#ifndef _SKIPSET_H_
#define _SKIPSET_H_

#include <iostream>
#include <vector>
#include <string>
#include <shared_mutex>

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
    mutable std::shared_mutex mtx;

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
    
};

template<typename K, typename V>
bool SkipSet<K, V>::update(const T& old, const T& t){

    if(old.first != t.first) {
        return false;
    }else{
        if(old.second == t.second) return false;
    }

    
    Node* curr=root, *prev=nullptr;
    std::unique_lock<std::shared_mutex> lock{mtx};
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
    std::unique_lock<std::shared_mutex> lock{mtx};
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
    
    std::shared_lock<std::shared_mutex> lock{mtx};

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
    std::shared_lock<std::shared_mutex> lock{mtx};
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
        std::unique_lock<std::shared_mutex> lock{mtx};
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
    std::unique_lock<std::shared_mutex> lock{mtx};
    Node* curr=root;
    while(curr){
        Node* tmp=curr->next[0];
        delete curr;
        curr=tmp;
        n--;
    }
    h=0;
}
 
#endif