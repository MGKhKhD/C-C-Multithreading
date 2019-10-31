#include <iostream>
#include <mutex>
#include <memory>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <exception>
#include <string>

class EmptyListException: public std::exception{
    std::string msg;
public:
    EmptyListException(std::string message="List is empy"): msg{std::move(message)} {}
    virtual const char* what() const noexcept override{
        return msg.c_str();
    }
};

template<typename T>
class ThreadSafeSList{
    struct Node{
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
        mutable std::mutex mtx;

        Node(): next() {}
        Node(T val): data{std::make_shared<T>(std::move(val))} {}
    };

    mutable Node head;
    std::size_t len;
    mutable std::mutex mtx;

    std::ostream& print(std::ostream& out) const{
        std::lock_guard<std::mutex> lock{mtx};
        forEach([&out](const T& t){
            out << t << "  ";
        });
        out << "\n";
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, const ThreadSafeSList& tsl){
        return tsl.print(out);
    }

public:
    ThreadSafeSList(): len{0} {}
    ThreadSafeSList(const ThreadSafeSList& tsl)=delete;
    ThreadSafeSList& operator=(const ThreadSafeSList& tsl)=delete;
    ~ThreadSafeSList(){
        remove_if([](const T& t){
            return true;
        });
    }

    void push_front(T val);

    template<typename Func>
    void forEach(Func func) const;

    template<typename Func>
    bool find_first_if(Func func) const;

    template<typename Func>
    void remove_if(Func func);

    const std::size_t countElem(const T& t) const{
        std::lock_guard<std::mutex> lock{mtx};
        std::size_t res{0};
        forEach([&res, t](const T& val){
            if(t==val) res++;
        });
        return res;
    }

};

template<typename T>
template<typename Func>
void ThreadSafeSList<T>::remove_if(Func func){
    Node* curr=&head;
    std::unique_lock<std::mutex> lock{head.mtx};
    if(!len) throw EmptyListException();
    while(Node* node=curr->next.get()){
        std::unique_lock<std::mutex> nxtLock{node->mtx};
        
        if(func(*node->data)){
            std::unique_ptr<Node> old{std::move(curr->next)};
            curr->next=std::move(node->next);
            --len;
            nxtLock.unlock();
        }else{
            lock.unlock();
            curr=node;
            lock=std::move(nxtLock);
        }
    }
}

template<typename T>
template<typename Func>
bool ThreadSafeSList<T>::find_first_if(Func func) const{
    Node* curr=&head;
    std::unique_lock<std::mutex> lock{head.mtx};
    while(Node* node=curr->next.get()){
        std::unique_lock<std::mutex> nxtLock{node->mtx};
        lock.unlock();
        if(func(*node->data)){          
            return true;
        }else{
            curr=node;
            lock=std::move(nxtLock);
        }
    }
    return false;
}

template<typename T>
template<typename Func>
void ThreadSafeSList<T>::forEach(Func func) const {
    Node* curr=&head;
    std::unique_lock<std::mutex> lock{head.mtx};
    while(Node* node=curr->next.get()){
        std::unique_lock<std::mutex> nxtLock{node->mtx};
        lock.unlock();
        func(*node->data);
        curr=node;
        lock=std::move(nxtLock);
    }
}

template<typename T>
void ThreadSafeSList<T>::push_front(T val){
    std::unique_ptr<Node> node{new Node(val)};
    std::lock_guard<std::mutex> lock{head.mtx};
    node->next=std::move(head.next);
    head.next=std::move(node);
    ++len;
}


int main(){
    std::vector<std::thread> workers;
    ThreadSafeSList<int> tsl;
    for(int i=0; i<15; ++i){
        auto payload{[i](ThreadSafeSList<int>& tsl){
            for(int j=0; j<=i; ++j){
                if(tsl.countElem(i)<4){
                    tsl.push_front(i);
                    std::this_thread::sleep_for(std::chrono::microseconds(std::rand()%500));
                }
                try
                {
                    tsl.remove_if([](const int num){
                        std::this_thread::sleep_for(std::chrono::microseconds(std::rand()%500));
                        return num>=10;
                    });
                }
                catch(const std::exception& e)
                {
                    std::cerr <<"Exception: " << e.what() << '\n';
                }
                
            }
        }};
        std::thread th{std::thread(payload, std::ref(tsl))};
        workers.push_back(std::move(th));
    }

    for(auto& w : workers){
        if(w.joinable()) w.join();
    }

    std::cout << tsl;

}