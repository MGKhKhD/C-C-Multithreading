#ifndef _PERSONAL_H_
#define _PERSONAL_H_

#include <iostream>
#include <fstream>
#include <string.h>

class Person{
private:
    const size_t SINLen, fnameLen, lnameLen, cityLen;
    char* SIN, *fname, *lname, *city;
    int year;
    long salary;

    std::istream& readFromConsole(std::istream& in);
    std::ostream& writeToConsole(std::ostream& out);

    friend std::ostream& operator<<(std::ostream& out, Person& pr){
        return pr.writeToConsole(out);
    }

    friend std::istream& operator>>(std::istream& in, Person& pr){
        return pr.readFromConsole(in);
    }


public:
    Person();
    Person(const char*, const char*, const char*, const char*, const int, const long);

    void readSIN(){
        char str[80];
        std::cout << "Enter SIN (9 ditigs): ";
        std::cin.getline(str, 80);
        strncpy(SIN, str, SINLen);
    }

    void readFromFile(std::fstream&);
    void writeToFile(std::fstream&);

    bool isRemoved() const{
        return fname[0]=='#';
    }

    void setRemoved() {
        fname[0]='#';
    }

    const long size() const {
        return SINLen + fnameLen + lnameLen + cityLen + sizeof(int) + sizeof(long);
    }

    const long getSIN() const {
        return strtol(SIN, NULL, 10);
    }

    void setSIN(const char* sin) {
        strncpy(SIN, sin, SINLen);
    }

    void readInfoWithoutSIN();
};

#endif