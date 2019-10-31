#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <deque>
#include <condition_variable>
#include <vector>
#include <chrono>

template<typename T>
class ThreadSafeQueue1{
    std::deque<T> data;
    std::condition_variable cv;
    mutable std::mutex mtx;

    std::ostream& print(std::ostream& out) const {
        std::lock_guard<std::mutex> lock{mtx};
        for(auto& e : data) out << e << "  ";
        out << "\n";
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, const ThreadSafeQueue1& tsq){
        return tsq.print(out);
    }

public:
    ThreadSafeQueue1(){}
    ThreadSafeQueue1(const ThreadSafeQueue1& tsq){
        std::lock_guard<std::mutex> lock{tsq.mtx};
        data=tsq.data;
    }

    ThreadSafeQueue1& operator=(const ThreadSafeQueue1& tsq)=delete;

    void push(T val){
        std::lock_guard<std::mutex> lock{mtx};
        data.push_front(std::move(val));
        cv.notify_all();
    }

    std::shared_ptr<T> wait_and_pop(){
        std::unique_lock<std::mutex> lock{mtx};
        cv.wait(lock, [this]{
            return !data.empty();
        });
        std::shared_ptr<T> res{std::make_shared<T>(std::move(data.back()))};
        data.pop_back();
        return res;
    }

    void wait_and_pop(T& val){
        std::unique_lock<std::mutex> lock{mtx};
        cv.wait(lock, [this]{
            return !data.empty();
        });
        val=std::move(data.back());
        data.pop_back();
    }

    std::shared_ptr<T> tryPop() {
        std::lock_guard<std::mutex> lock{mtx};
        if(data.empty()) return std::make_shared<T>();
        std::shared_ptr<T> res{std::make_shared<T>(std::move(data.back()))};
        data.pop_back();
        return res;
    }

    bool tryPop(T& val) {
        std::lock_guard<std::mutex> lock{mtx};
        if(data.empty()) return false;
        val=std::move(data.back());
        data.pop_back();
        return true;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock{mtx};
        return data.empty();
    }
};

int main(){
    ThreadSafeQueue1<int> tsq;
    std::vector<std::thread> workers;
    std::vector<int> res;
    std::vector<int> res1;
    for (int i=0; i<15; ++i){
        if(i<=5){
            auto payload{[i](ThreadSafeQueue1<int>& tsq){
                for(int j=0; j<=i; ++j) tsq.push(i);
            }};
            std::thread th{std::thread(payload, std::ref(tsq))};
            workers.push_back(std::move(th));
        }else if( i> 5 && i<=10){
            auto payload{[](ThreadSafeQueue1<int>& tsq, std::vector<int>& vec){
                while(!tsq.isEmpty()){
                    std::shared_ptr<int> ptr=tsq.wait_and_pop();
                    vec.push_back(*ptr);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }};
            std::thread th{std::thread(payload, std::ref(tsq), std::ref(res))};
            workers.push_back(std::move(th));
        }else{
            auto payload{[](ThreadSafeQueue1<int>& tsq, std::vector<int>& vec){
                int j=0;
                while(j<=5){
                    int val;
                    if(tsq.tryPop(val)){
                        vec.push_back(val);
                    }else{
                        ++j;
                    }
                }
            }};
            std::thread th{std::thread(payload, std::ref(tsq), std::ref(res1))};
            workers.push_back(std::move(th));
        }
    }

    for(auto& w : workers){
        if(w.joinable()) w.join();
    }

    std::cout << tsq;
    for( auto& e : res) std::cout << e << "  ";
    std::cout << "\n";

    for( auto& e : res1) std::cout << e << "  ";
    std::cout << "\n";

}