#include <iostream>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <queue>


template<typename T>
class ThreadSafeQueue{
private:
    struct Node{
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
    };
    Node* tail;
    std::unique_ptr<Node> head;
    mutable std::mutex mtxH;
    mutable std::mutex mtxT;
    std::condition_variable cv;
    std::size_t n;

    Node* getTail() const{
        std::lock_guard<std::mutex> lock{mtxT};
        return tail;
    }

    std::unique_ptr<Node> popHead(){
        std::unique_ptr<Node> oldHead{std::move(head)};
        head=std::move(oldHead->next);
        return oldHead;
    }

    std::unique_lock<std::mutex> waitForData(){
        std::unique_lock<std::mutex> lock{mtxH};
        cv.wait(lock, [&]{
            return (head.get() != getTail());
        });
        return std::move(lock);
    }

    std::unique_ptr<Node> waitDequeue(){
        std::unique_lock<std::mutex> lock{waitForData()};
        return popHead();
    }

    std::unique_ptr<Node> waitDequeue(T& val){
        std::unique_lock<std::mutex> lock{waitForData()};
        val=std::move(*(head->data));
        return popHead();
    }

    std::unique_ptr<Node> tryPopHead(){
        std::unique_lock<std::mutex> lock{mtxH};
        if(head.get()==getTail()) return std::unique_ptr<Node>();
        return popHead();
    }

    std::unique_ptr<Node> tryPopHead(T& val){
        std::unique_lock<std::mutex> lock{mtxH};
        if(head.get()==getTail()) return std::unique_ptr<Node>();
        val=std::move(*(head->data));
        return popHead();
    }

public:
    ThreadSafeQueue(): head{new Node()}{
        tail=head.get();
        n=0;
    }
    ThreadSafeQueue(const ThreadSafeQueue& tsq)=delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue& tsq)=delete;

    bool isEmpty() const{
        std::lock_guard<std::mutex> lock{mtxH};
        return (head.get() == getTail());
    }

    void push(T newVal){
        std::shared_ptr<T> val{std::make_shared<T>(std::move(newVal))};
        std::unique_ptr<Node> p{new Node()};
        {
            std::lock_guard<std::mutex> lock{mtxT};
            tail->data=val;
            Node* newTail=p.get();
            tail->next=std::move(p);
            tail=newTail;
            ++n;
        }
        cv.notify_one();
    }

    const size_t size() const{
        std::lock_guard<std::mutex> lock{mtxH};
        std::lock_guard<std::mutex> lockT{mtxT};
        return n;
    }

    std::shared_ptr<T> waitAndDequeue(){
        std::unique_ptr<Node> oldHead{waitDequeue()};
        return oldHead->data;
    }

    void waitAndDequeue(T& val){
        std::unique_ptr<Node> oldHead{waitDequeue(val)};
    }

    std::shared_ptr<T> tryDequeue(){
        std::unique_ptr<Node> oldHead{tryPopHead()};
        return oldHead? oldHead->data : std::make_shared<T>();
    }

    bool tryDequeue(T& val){
        std::unique_ptr<Node> oldHead{tryPopHead(val)};
        return oldHead? true : false;
    }

};

int main(){
    ThreadSafeQueue<int> tsq;
    std::vector<std::thread> workers1;
    std::size_t count{0};
    for(int i=0; i<10; ++i){
        auto payload{[i, &count](ThreadSafeQueue<int>& q){
        for(int j=0; j<=i; ++j) {
            q.push(i);
            ++count;
        }
        }};
        std::thread th{std::thread(payload, std::ref(tsq))};
        workers1.push_back(std::move(th));
    }

    ThreadSafeQueue<int> tsq2;
    std::vector<std::thread> workers2;
    for(int i=0; i<4; ++i){
        auto payload2{[](ThreadSafeQueue<int>& src, ThreadSafeQueue<int>& des){
            while(!src.isEmpty()){
                std::shared_ptr<int> ptr=src.waitAndDequeue();
                if(ptr) des.push(*ptr);
            }
        }};
        std::thread th{std::thread(payload2, std::ref(tsq), std::ref(tsq2))};
        workers2.push_back(std::move(th));
    }

    for(int i=0; i<10; ++i){
        if(workers1[i].joinable()) workers1[i].join();
    }
    for(auto& w: workers2){
        if(w.joinable()) w.join();
    }

    std::cout <<"the first one: \n";
    while(!tsq.isEmpty()){
        int val;
        tsq.waitAndDequeue(val);
        std::cout << val << " ";
    }

    std::cout << "\nthe second one:\n";

    while(!tsq2.isEmpty()){
        std::shared_ptr<int> ptr=tsq2.tryDequeue();
        if(ptr){
            std::cout << *ptr << "  ";
        }
    }

    std::cout << "\n";

    return 0;

}