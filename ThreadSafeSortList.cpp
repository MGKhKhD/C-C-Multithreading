#include<iostream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <memory>
#include <vector>

template<typename T>
class ThreadSafeSorteList{
    struct Node{
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
        std::mutex mtx;

        Node(): next() {}
        Node(T t): data{std::make_shared<T>(std::move(t))} {}
    };
    Node head;
    int n;

    std::ostream& print(std::ostream& out){
        forEach([&out](const T& t){
            out << t << "  ";
        });
        out << "\n";
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, ThreadSafeSorteList& tss){
        return tss.print(out);
    }

public:
    ThreadSafeSorteList(): n{0} {}

    void add(T t){
        std::unique_ptr<Node> newNode{new Node(t)};
        std::unique_lock<std::mutex> lock{head.mtx};
        if(!n++){
            head.next=std::move(newNode);
        }

        Node* curr=&head;
        while(Node* node=curr->next.get()){
            if(*(node->data) > t){
                std::unique_lock<std::mutex> nxtLock{node->mtx};
                lock.unlock();
                Node* p=newNode.get();
                p->next=std::move(curr->next);
                curr->next=std::move(newNode);
                n++;
                break;
            }
            curr=node;
        }
    }

    bool remove(const T& t){
        std::unique_lock<std::mutex> lock{head.mtx};
        if(!n) return false;
        Node* curr=&head;
        bool removed{false};
        while(Node* node=curr->next.get()){
            std::unique_lock<std::mutex> nxtLock{node->mtx};
            if(*node->data < t){
                lock.unlock();
                curr=node;
                lock=std::move(nxtLock);
            }else if(*node->data == t){
                std::unique_ptr<Node> oldNode{std::move(curr->next)};
                curr->next=std::move(node->next);
                nxtLock.unlock();
                removed=true;
            }else{
                break;
            }
        }
        return removed;
    }

    bool find(const T& t){
        std::unique_lock<std::mutex> lock{head.mtx};
        if(!n) return false;
        Node* curr=&head;
        while(Node* node=curr->next.get()){
            std::unique_lock<std::mutex> nxtLock{node->mtx};
            lock.unlock();
            if(*node->data < t){
               curr=node;
               lock=std::move(nxtLock); 
            }else if(*node->data==t){
                return true;
            }else {
                break;
            }
        }
        return false;
    }

    template<typename Func>
    void forEach(Func func){
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
};

int main(){
    ThreadSafeSorteList<int> tsl;
    std::vector<std::thread> workers;
    for(int i=0; i<20; ++i){
        if(i<=15){
            auto payload{[](ThreadSafeSorteList<int>& tsl){
                for(int j=0; j<=2; ++j) {
                    tsl.add(std::rand()%200);
                }
            }};
            std::thread th{std::thread(payload, std::ref(tsl))};
            workers.push_back(std::move(th));
        }else{
            auto payload{[](ThreadSafeSorteList<int>& tsl){
                int z=std::rand()%200;
                if(tsl.find(z)){
                    std::cout << "found: " << z << "\n";
                }else{
                    std::cout << "Not found: " << z << "\n";
                }
            }};
            std::thread th{std::thread(payload, std::ref(tsl))};
            workers.push_back(std::move(th));
        }
    }

    for(auto& w: workers){
        if(w.joinable()){
            w.join();
        }
    }

    std::cout << tsl;

    std::vector<std::thread> workers1;
    for(int i=0; i<=20; ++i){
        auto payload{[](ThreadSafeSorteList<int>& tsl){
            int z=std::rand()%200;
            if(tsl.remove(z)){
                std::cout << "removed: " << z << "\n";
            }else{
                std::cout << "Not removed: " << z << "\n";
            }
        }};
        std::thread th{std::thread(payload, std::ref(tsl))};
        workers1.push_back(std::move(th));
    }

    for(auto& w: workers1){
        if(w.joinable()){
            w.join();
        }
    }

    std::cout << tsl;
    
}