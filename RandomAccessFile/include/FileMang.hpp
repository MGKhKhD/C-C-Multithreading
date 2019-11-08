#ifndef _FILEMANAGEMENT_H_
#define _FILEMANAGEMENT_H_

#include <iostream>
#include <fstream>
#include <mutex>
#include <string.h>
#include <exception>
#include <string>

#include "Person.hpp"
#include "SkipSet.hpp"

class UnableToOpenFileException: public std::exception{
    std::string msg;
public:
    UnableToOpenFileException(std::string message="Unable to open file"): msg(std::move(message)) {}
    virtual const char* what() const noexcept override { return msg.c_str();}
};

template<typename F>
class FileMang{
private:
    char fileName[30];
    std::fstream fmang;
    SkipSet<long, long> activePos;
    std::vector<long> inactivePos;
    mutable std::mutex mtx;

    void init();
    void fillPos();
    void add( F& f);
    bool find(F& f);
    void modify(F& f);
    void applyModification(F& f, long pos);
    bool remove(F& f);

    std::ostream& print(std::ostream& out) {
        if(fmang.is_open()) fmang.close();

        fmang.open(fileName, std::ios::in | std::ios::binary);
        if(!fmang.is_open()) throw UnableToOpenFileException();

        F rec;
        std::cout << "-------------------------------RECORDS FORM FILE-------------------------\n";
        while(true){
            rec.readFromFile(fmang);
            if(fmang.eof()) break;
            if(!rec.isRemoved()) std::cout << rec << "\n";
        }
        fmang.close();
        std::cout << "-------------------------------------END-----------------------------------\n";
    }

    friend std::ostream& operator<<(std::ostream& out, FileMang& file) {
        return file.print(out);
    }

public:
    FileMang(){
        init();
    }

    void run();

};

template<typename F>
bool FileMang<F>::remove(F& rec){
    long pos;
    if(activePos.find(rec.getSIN(), pos) && pos!=-1){
        if(fmang.is_open()) fmang.close();

        fmang.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
        if(!fmang.is_open()) throw UnableToOpenFileException();

        
        fmang.seekp(pos, std::ios::beg);
        rec.readFromFile(fmang);
        rec.setRemoved();
        fmang.seekp(pos, std::ios::beg);
        rec.writeToFile(fmang);

        activePos.remove(rec.getSIN());
        inactivePos.push_back(pos);

        return true;
    }
    return false;
}

template<typename F>
void FileMang<F>::applyModification(F& rec, long pos) {
        rec.readInfoWithoutSIN();
        if(fmang.is_open()) fmang.close();

        fmang.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
        if(!fmang.is_open()) throw UnableToOpenFileException();

        fmang.seekp(pos, std::ios::beg);
        rec.writeToFile(fmang);

        fmang.close();
}

template<typename F>
void FileMang<F>::modify(F& rec) {
    long pos = -1;
    if(activePos.find(rec.getSIN(), pos) && pos != -1){
        std::cout << "Enter new info: \n";
        F newRec;
        
        newRec.readSIN();

        if(newRec.getSIN() == rec.getSIN()){
            try{
                applyModification(newRec, pos);
            }catch(std::exception& ex){
                throw UnableToOpenFileException();
            }
        }else{
            long pos1=-1;
            if( activePos.find(newRec.getSIN(), pos1) && pos1 != -1 ){
                std::cout << "The SIN is taken, try new one\n";
                return;
            }else {
                try{
                    applyModification(newRec, pos);
                }catch(std::exception& ex){
                    throw UnableToOpenFileException();
                }
            }
        }
    }else{
        std::cout << "Record does not exist in the file\n";
        return;
    }
}

template<typename F>
bool FileMang<F>::find(F& rec){
    long pos = -1;
    if(activePos.find(rec.getSIN(), pos) && pos != -1){
        if(fmang.is_open()) fmang.close();

        fmang.open(fileName, std::ios::in | std::ios::binary);
        if(!fmang.is_open()) throw UnableToOpenFileException();

        fmang.seekp(pos, std::ios::beg);
        rec.readFromFile(fmang);
        std::cout << rec << "\n";

        fmang.close();
        return true;
    }
    return false;
}


template<typename F>
void FileMang<F>::add( F& rec){
    if(fmang.is_open()) fmang.close();

    fmang.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
    if(!fmang.is_open()) throw UnableToOpenFileException();

    if(inactivePos.empty()){
        std::cout << rec << "\n";
        fmang.seekp(0,std::ios::end);
        rec.writeToFile(fmang);
        long pos=fmang.tellp();
        activePos.add(std::pair<long, long>(rec.getSIN(), pos-rec.size()));
    }else{
        long pos=inactivePos.back();
        inactivePos.pop_back();
        fmang.seekp(pos, std::ios::beg);
        rec.writeToFile(fmang);
        activePos.add(std::pair<long, long>(rec.getSIN(), pos));
    }

    fmang.close();
}

template<typename F>
void FileMang<F>::init(){
    char str[80];
    std::cout << "Enter file name: ";
    std::cin.getline(str, 80);
    strncpy(fileName, str, 30);
    try{
        fillPos();
    }catch(std::exception& ex){
        std::cout << "Exception in sonstruction: " << ex.what() << "\n";
    }
}

template<typename F>
void FileMang<F>::run() {
    char options[5];
    std::cout << "Choose your option: \n";
    std::cout << "1) Add, 2) Find, 3) Modify, 4) Remove, 5) Show, 6) Exit\n";
    std::cin.getline(options, 4);
    F rec;
    while(std::cin.getline(options,5)){
        if(*options=='1'){

            rec.readSIN();
            try{
                if(find(rec)){
                    std::cout<< "Record already exists in the file\n";
                }else{
                    rec.readInfoWithoutSIN();
                    try{
                        add(rec);
                    }catch(std::exception& ex){
                        std::cerr << "Exception: " << ex.what() << "\n";
                    }
                }
            }catch(std::exception& ex){
                std::cerr << "Exception: " << ex.what() << "\n";
            }

        }else if(*options=='2'){

            rec.readSIN();
            std::cout << "Record ";
            try{
                if(find(rec)){
                    std::cout << "exists in the file \n";
                }else{
                    std::cout << "does not exist in the file\n";
                }
            }catch(std::exception& ex){
                std::cerr << "Exception: " << ex.what() << "\n";
            }

        }else if(*options=='3'){
            
            rec.readSIN();
            try{
                modify(rec);
            }catch(std::exception& ex){
                std::cerr << "Exception: " << ex.what() << "\n";
            }

        }else if(*options=='4'){
            rec.readSIN();
            try{
                if(remove(rec)){
                    std::cout << "the record successfully deleted\n";
                }else{
                    std::cout << "reocrd deos not exit in the file\n";
                }
            }catch(std::exception& ex){
                std::cerr << "Exception: " << ex.what() << "\n";
            }
        }else if(*options=='5'){

            try{
                std::cout << *this;
            }catch(std::exception& ex) {
                std::cerr << "Exception: " <<ex.what() << "\n";
            }

        }else if(*options=='6'){
            return;
        }else{
            std::cout << "wrong answer\n";
        }
        std::cout << "Choose your option: \n";
        std::cout << "1) Add, 2) Find, 3) Modify, 4) Remove, 5) Show, 6) Exit\n";
    }
}

template<typename F>
void FileMang<F>::fillPos() {
    std::unique_lock<std::mutex> lock{mtx};
    fmang.open(fileName, std::ios::in | std::ios::out | std::ios:: binary);
    if(!fmang.is_open()) {
        throw UnableToOpenFileException();
    }

    F rec;
    while(true){
        rec.readFromFile(fmang);
        if(fmang.eof()) break;
        long pos=fmang.tellp();
        
        if(rec.isRemoved()){
            inactivePos.push_back(pos-rec.size());
        }else{
            activePos.add(std::pair<long, long>(rec.getSIN(), pos-rec.size()));
        }
    }
    fmang.close();
}

#endif