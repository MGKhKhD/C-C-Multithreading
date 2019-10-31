#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <exception>
#include <chrono>

class EmptyStackException: public std::exception{
    std::string msg;
public:
    EmptyStackException(std::string str="Stack is empty"): msg{std::move(str)} {}
    virtual const char* what() const noexcept override{
        return msg.c_str();
    }
};

template<typename T>
class ThreadSafeStack{
    struct Node{
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
        std::mutex mtx;

        Node(): next() {}
        explicit Node(T val): data{std::make_shared<T>(std::move(val))} {}
    };

    mutable std::mutex stackMtx;
    Node head;
    std::size_t len;

    friend std::ostream& operator<<(std::ostream& out, ThreadSafeStack& stack){
        return stack.printStack(out);
    }

    std::ostream& printStack(std::ostream& out){
        Node* curr=&head;
        std::unique_lock<std::mutex> lock{stackMtx};
        while(Node* node=curr->next.get()){
            out << *node->data << "  ";
            curr=node;
        }
        out << "\n";
        return out;
    }

    std::shared_ptr<T> popItem(){
        Node* curr=&head;
        std::unique_lock<std::mutex> lock{head.mtx};
        if(!len) throw EmptyStackException();
        Node* node=curr->next.get();
        std::shared_ptr<T> res{node->data};
        std::unique_ptr<Node> old{std::move(curr->next)};
        head.next=std::move(node->next);
        --len;
        return res;
    }

public:
    ThreadSafeStack(): len{0} {}
    ThreadSafeStack(const ThreadSafeStack& tss) = delete;
    ThreadSafeStack& operator=(const ThreadSafeStack& tss) = delete;

    void push(T val){
        std::unique_ptr<Node> node{new Node(val)};
        std::lock_guard<std::mutex> lock{head.mtx};
        node->next=std::move(head.next);
        head.next=std::move(node);
        ++len;
    }

    std::shared_ptr<T> pop(){
        return popItem();
    }

    void pop(T& val){
        std::shared_ptr<T> res{popItem()};
        val=*res;
    }

    bool isEmpty() const{
        std::lock_guard<std::mutex> lock{head.mtx};
        return len==0;
    }
    
};

template<typename T>
class ThreadSafeVector{
    std::vector<T> vec;
    mutable std::mutex mtx;

    std::ostream& print(std::ostream& out) const{
        for(auto& e : vec) out << e << "  ";
        out << "\n";
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, const ThreadSafeVector& tsv){
        return tsv.print(out);
    }

public:
    void push_back(T val){
        std::lock_guard<std::mutex> lock{mtx};
        vec.push_back(std::move(val));
    }
};

int main(){
    ThreadSafeStack<int> tss;
    std::vector<std::thread> workers;
    ThreadSafeVector<int> vals;
    for(int i=0; i<10; ++i){
        if(i<=8){
            auto payload{[i](ThreadSafeStack<int>& tss){
                for(int j=0; j<=i; ++j) tss.push(i);
            }};
            std::thread th{std::thread(payload, std::ref(tss))};
            workers.push_back(std::move(th));
        }else{
            auto payload{[](ThreadSafeStack<int>& tss, ThreadSafeVector<int>& vec){
                try{
                    // std::shared_ptr<int> ptr=tss.pop();
                    // vec.push_back(*ptr);
                    int val;
                    tss.pop(val);
                    vec.push_back(val);
                }catch(std::exception& ex){
                    std::cout << "Exception: " << ex.what() << "\n";
                }
            }};
            std::thread th{std::thread(payload, std::ref(tss), std::ref(vals))};
            workers.push_back(std::move(th));
        } 
    }
    for(auto& w: workers){
        if(w.joinable()) w.join();
    }

    std::cout << tss;
    std::cout << vals;
}