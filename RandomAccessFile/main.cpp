
#include <vector>
#include <thread>

#include "include/Person.hpp"
#include "include/FileMang.hpp"

void testSkipSet(){
    SkipSet<int, int> tss;

    std::vector<std::thread> workers;
    for(int i=0; i<5; ++i){
        auto payload{[](SkipSet<int, int>& tss){
            for(int j=0; j<4; ++j){
                tss.add(std::pair<int, int>(std::rand()%200,std::rand()%200));
            }
        }};
        std::thread th{std::thread(payload, std::ref(tss))};
        workers.push_back(std::move(th));
    }

    for(auto& w : workers){
        if(w.joinable()) w.join();
    }
    
    std::cout << tss;
}

void testPerson() {
    Person pr1("123123123", "Mari", "Tanberland", "LA", 1994, 20000);
    std::cout << pr1 << "\n";
    Person pr2;
    std::cin >> pr2;
    std::cout << pr2 << "\n";
}

int main() {
    //testPerson();
    //testSkipSet();

    FileMang<Person> db;
    db.run();
}