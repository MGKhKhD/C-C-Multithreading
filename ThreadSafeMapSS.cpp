#include <iostream>
#include <vector>
#include <thread>
#include <shared_mutex>
#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <map>

template<typename K, typename V>
class SkipSet;

template<typename K, typename V, typename H=std::hash<K>>
class ThreadSafeMapSS{
private:
    struct Bucket{
        SkipSet<K, V> bucket_data;

        std::shared_ptr<V> find(const K& k) const{
            return bucket_data.find(k);
        }

        void add_or_update(const K& k, const V& v){
            V old;
            if(bucket_data.find(k, old)){
                bucket_data.update(std::pair<K, V>(k, old), std::pair<K, V>(k, v));
            }else{
                bucket_data.add(std::pair<K, V>(k, v));
            }
        }

        std::shared_ptr<V> remove(const K& k){
            return bucket_data.remove(k);
        }

        std::ostream& print(std::ostream& out) const{
            out << bucket_data;
            return out;
        }

        friend std::ostream& operator<<(std::ostream& out, const Bucket& bucket){
            return bucket.print(out);
        }
    };
    std::vector<std::unique_ptr<Bucket>> buckets;
    H hasher;

    Bucket& getBucket(const K& k) const{
        std::size_t ind{hasher(k)%buckets.size()};
        return *buckets[ind];
    }

    std::ostream& print(std::ostream& out) const{
        for(int i=0; i<buckets.size(); out << *buckets[i], ++i);
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, const ThreadSafeMapSS& tsm){
        return tsm.print(out);
    }

public:
    ThreadSafeMapSS(unsigned int numBuckets = 7, const H& hasher_=H()): buckets(numBuckets), hasher{hasher_} {
        for(int i{0}; i<numBuckets; ++i){
            buckets[i].reset(new Bucket);
        }
    }

    void add_or_update(const K& k, const V& v){
        getBucket(k).add_or_update(k, v);
    }

    std::shared_ptr<V> remove(const K& k){
        return getBucket(k).remove(k);
    }

    std::shared_ptr<V> find(const K& k) const{
        return getBucket(k).find(k);
    }
    
};

template<typename K, typename V>
class SkipSet{
    typedef typename std::pair<K, V> T;
    struct Node{
        T info;
        std::vector<Node*> next;
        int height;

        Node(const T& t, const int hv): info{t}, height{hv} {
            for(unsigned int i=0; i<=hv; next.push_back(nullptr), ++i);
        }

    };
    Node *root;
    int h;
    int n;
    mutable std::shared_timed_mutex mtx;

    void destroySet();

    const int setHeight() const{
        const int z=std::rand();
        int m=1;
        int k=0;
        while((m & z)){
            k++;
            m<<=1;
        }
        return k;
    }

    void updateParams(Node* &curr, Node* &prev, const int r, const T& t) const;
    bool findEntry(const K& k, V& v) const;
    bool removeEntry(const K& k, V& v);

    std::ostream& printSS(std::ostream& out) const;
    friend std::ostream& operator<<(std::ostream& out, const SkipSet& ss){
        return ss.printSS(out);
    }


public:
    SkipSet(): n{0}{
        if constexpr (std::is_same_v<K,std::string> && std::is_same_v<V,std::string>){
            root=new Node(std::pair<K,V>("", ""), sizeof(int)*8);
        }else if constexpr (!std::is_same_v<K, std::string> && std::is_same_v<V, std::string>){
            root=new Node(std::pair<K,V>(static_cast<K>(NULL), ""), sizeof(int)*8);
        }else if constexpr (std::is_same_v<K, std::string> && !std::is_same_v<V, std::string>){
            root=new Node(std::pair<K,V>( "", static_cast<V>(NULL)), sizeof(int)*8);
        }else{
            root=new Node(std::pair<K,V>(static_cast<K>(NULL), static_cast<V>(NULL)), sizeof(int)*8);
        }
        h=root->height;
    }

    ~SkipSet(){
        destroySet();
    }

    void add(const T& t);
    bool update(const T& old, const T& t);

    std::shared_ptr<V> find(const K& k) const{
        V v;
        return findEntry(k, v)? std::make_shared<V>(std::move(v)) : std::make_shared<V>();
    }

    bool find(const K& k, V& v) const{
        return findEntry(k, v);
    }

    bool remove(const K& k, V& v){
        return removeEntry(k, v);
    }

    std::shared_ptr<V> remove(const K& k){
        V v;
        return removeEntry(k, v)? std::make_shared<V>(std::move(v)) : std::make_shared<V>();
    }

    std::map<K, V> getMap() const{
        std::unique_lock<std::shared_timed_mutex> lock{mtx};
        std::map<K, V> res;
        for(Node* node=root->next[0]; node; res.insert(node->info.first, node->info.second), node=node->next[0]);
        return res;
    }
    
};

template<typename K, typename V>
bool SkipSet<K, V>::update(const T& old, const T& t){

    if(old.first != t.first) {
        return false;
    }else{
        if(old.second == t.second) return false;
    }

    
    Node* curr=root, *prev=nullptr;
    std::unique_lock<std::shared_timed_mutex> lock{mtx};
    int r=h;
    while(r>=0){
        updateParams(curr, prev, r, old);
        if(curr->next[r] && curr->next[r]->info.first==old.first){
            lock.unlock();
            V v;
            remove(old.first, v);
            add(t);
            return true;
        }
        r--;
    }
    return false;
}

template<typename K, typename V>
bool SkipSet<K, V>::removeEntry(const K& k, V& v){
    Node* curr=root, *prev=nullptr, *del=nullptr;
    std::unique_lock<std::shared_timed_mutex> lock{mtx};
    int r=h;
    while(r>=0){
        updateParams(curr, prev, r, std::pair<K, V>(k, v));
        if(curr->next[r] && curr->next[r]->info.first==k){
            del=curr->next[r];
            curr->next[r]=del->next[r];
            if(curr==root) h--;
        }
        r--;
    }
    if(del){
        v=del->info.second;
        delete del;
        n--;
        return true;
    }
    return false;

}

template<typename K, typename V>
bool SkipSet<K, V>:: findEntry(const K& k, V& v) const{
    Node* curr=root, *prev=nullptr;
    
    std::lock_guard<std::shared_timed_mutex> lock{mtx};

    int r=h;
    while(r>=0){
        updateParams(curr, prev, r, std::pair<K, V>(k, v));
        if(curr->next[r] && curr->next[r]->info.first==k){
            v=curr->next[r]->info.second;
            return true;
        }
        r--;
    }
    return false;
}

template<typename K, typename V>
std::ostream& SkipSet<K, V>::printSS(std::ostream& out) const{
    std::lock_guard<std::shared_timed_mutex> lock{mtx};
    for(Node* node=root->next[0]; node; node=node->next[0]){
        out << "(" << node->info.first << ", " << node->info.second << ")" << "    ";
    }
    out << "\n";
    return out;
}

template<typename K, typename V>
void SkipSet<K, V>::updateParams(Node* &curr, Node* &prev, const int r, const T& t) const{
    while(curr->next[r] && (!prev || (prev && prev!=curr)) && curr->next[r]->info.first < t.first){
        prev=curr;
        curr=curr->next[r];
    }
}

template<typename K, typename V>
void SkipSet<K, V>::add(const T& t){
    Node* curr=root, *prev=nullptr;
    Node* node=new Node(std::pair<K, V>(t.first, t.second), setHeight());
    std::vector<Node*> finger;

    {
        std::unique_lock<std::shared_timed_mutex> lock{mtx};
        for(unsigned int i=0; i<=h; finger.push_back(nullptr), ++i);
        int r=h;
        while(r>=0){
            updateParams(curr, prev, r, t);
            if(curr->next[r] && curr->next[r]->info.first == t.first) return;
            finger[r--]=curr;
        }
        while(node->height >= h++){
            finger.push_back(root);
        }

        for(unsigned int i=0; i<=node->height; ++i){
            node->next[i]=finger[i]->next[i];
            finger[i]->next[i]=node;
        }
        n++;
    }
}

template<typename K, typename V>
void SkipSet<K, V>::destroySet(){
    std::unique_lock<std::shared_timed_mutex> lock{mtx};
    Node* curr=root;
    while(curr){
        Node* tmp=curr->next[0];
        delete curr;
        curr=tmp;
        n--;
    }
    h=0;
}

void testSS(){
    SkipSet<int, int> ss1;
    std::size_t nums{10};
    std::vector<std::thread> workers;
    workers.reserve(nums);
    std::vector<std::pair<int,int>> vec;
    int count{5};
    for(std::size_t i{0}; i<nums; ++i){
        auto payload{[&vec, &count](SkipSet<int, int>& ss){
            int k=std::rand()%200;
            int v=std::rand()%200;
            ss.add(std::pair<int, int>(k,v));
            if(count--) vec.push_back(std::pair<int, int>(k,v));
        }};
        std::thread th{std::thread(payload, std::ref(ss1))};
        workers.push_back(std::move(th));
    }

    for(auto& w : workers){
        if(w.joinable()) w.join();
    }

    std::cout << ss1;

    //find 
    // auto paylod1{[&vec](SkipSet<int,int>& ss){
    //     for(auto v=vec.begin(); v!=vec.end();){
    //         std::shared_ptr<int> ptr{ss.find((*v).first)};
    //         if(ptr){
    //             std::cout << "worker1: " << "(" << (*v).first << ", " << *ptr << ")" << "\n";
    //             v=vec.erase(v);
    //             break;
    //         }
    //     }
    // }};
    // std::thread th1{std::thread(paylod1, std::ref(ss1))};
    // auto paylod2{[&vec](SkipSet<int,int>& ss){
    //     for(auto v=vec.begin(); v!=vec.end();){
    //         int res;
           
    //         if( ss.find((*v).first, res)){
    //             std::cout << "worker2: " << "(" << (*v).first << ", " << res << ")" << "\n";
    //             v=vec.erase(v);
    //             break;
    //         }
    //     }
    // }};
    // std::thread th2{std::thread(paylod2, std::ref(ss1))};

    
    // if(th1.joinable()) th1.join();
    // if(th2.joinable()) th2.join();
    

    // std::cout << "the size of vec: " << vec.size() << "\n";

    // remove
    auto paylod3{[&vec](SkipSet<int,int>& ss){
        for(auto v=vec.begin(); v!=vec.end();){
            std::shared_ptr<int> ptr{ss.remove((*v).first)};
            if(ptr){
                std::cout << "worker3: " << "(" << (*v).first << ", " << *ptr << ")" << "\n";
                v=vec.erase(v);
                break;
            }
        }
    }};
    std::thread th3{std::thread(paylod3, std::ref(ss1))};
    auto paylod4{[&vec](SkipSet<int,int>& ss){
        for(auto v=vec.begin(); v!=vec.end();){
            int res;
           
            if( ss.remove((*v).first, res)){
                std::cout << "worker4: " << "(" << (*v).first << ", " << res << ")" << "\n";
                v=vec.erase(v);
                break;
            }
        }
    }};
    std::thread th4{std::thread(paylod4, std::ref(ss1))};

    
    if(th3.joinable()) th3.join();
    if(th4.joinable()) th4.join();
    

    std::cout << "the size of vec: " << vec.size() << "\n";
    std::cout << ss1;

    //update
    auto paylod5{[&vec](SkipSet<int,int>& ss){
        for(auto v=vec.begin(); v!=vec.end();){
            std::shared_ptr<int> ptr{ss.find((*v).first)};
            if(ptr){
                std::cout << "worker5: " << "(" << (*v).first << ", " << *ptr << ")" << "\n";
                ss.update(*v, std::pair<int, int>((*v).first, (*v).second+2));
                v=vec.erase(v);
                break;
            }
        }
    }};
    std::thread th5{std::thread(paylod5, std::ref(ss1))};
    auto paylod6{[&vec](SkipSet<int,int>& ss){
        for(auto v=vec.begin(); v!=vec.end();){
            std::shared_ptr<int> ptr{ss.find((*v).first)};
            if(ptr){
                std::cout << "worker6: " << "(" << (*v).first << ", " << *ptr << ")" << "\n";
                ss.update(*v, std::pair<int, int>((*v).first, (*v).second+40));
                v=vec.erase(v);
                break;
            }
        }
    }};
    std::thread th6{std::thread(paylod6, std::ref(ss1))};
     auto paylod7{[&vec](SkipSet<int,int>& ss){
        for(auto v=vec.begin(); v!=vec.end();){
            std::shared_ptr<int> ptr{ss.find((*v).first)};
            if(ptr){
                std::cout << "worker7: " << "(" << (*v).first << ", " << *ptr << ")" << "\n";
                ss.update(*v, std::pair<int, int>((*v).first, (*v).second-40));
                v=vec.erase(v);
                break;
            }
        }
    }};
    std::thread th7{std::thread(paylod7, std::ref(ss1))};

    
    if(th5.joinable()) th5.join();
    if(th6.joinable()) th6.join();
    if(th7.joinable()) th7.join();
    

    std::cout << "the size of vec: " << vec.size() << "\n";
    std::cout << ss1;


}

void testMap(){
    ThreadSafeMapSS<int, int> tsm;
    std::size_t nums{10};
    std::vector<std::thread> workers;
    workers.reserve(nums);

    for(std::size_t i{0}; i<nums; ++i){
        auto payload{[](ThreadSafeMapSS<int, int>& ss){
            for(int i=0; i<4; ++i){
                int k=std::rand()%200;
                int v=std::rand()%200;
                ss.add_or_update(k, v);
            }
        }};
        std::thread th{std::thread(payload, std::ref(tsm))};
        workers.push_back(std::move(th));
    }

    for(auto& w : workers){
        if(w.joinable()) w.join();
    }

    std::cout << tsm;
}

int main(){
    // SkipSet<int, int> ss;
    // SkipSet<int, std::string> ss1;
    // SkipSet<std::string, int> ss2;
    // SkipSet<std::string, std::string> ss3;
    // SkipSet<const char*, std::string> ss4;

    // testSS();

    testMap();

    return 0;
}