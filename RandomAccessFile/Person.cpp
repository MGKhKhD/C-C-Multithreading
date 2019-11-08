#include "include/Person.hpp"

Person::Person(): SINLen{9}, cityLen{20}, fnameLen{20}, lnameLen{20} {
    SIN = new char[SINLen+1];
    fname = new char[fnameLen+1];
    lname = new char[lnameLen+1];
    city = new char[cityLen+1];
}

Person::Person(const char* sin, const char* f, const char* l, const char* c, const int y, const long s):
Person() {
    strncpy(SIN, sin, SINLen);
    strncpy(fname, f, fnameLen);
    strncpy(lname, l, lnameLen);
    strncpy(city, c, cityLen);
    year=y;
    salary=s;
}

std::ostream& Person::writeToConsole(std::ostream& out) {
    SIN[SINLen]=fname[fnameLen]=lname[lnameLen]=city[cityLen]='\0';
    out << "SIN: " << SIN << ", fname: " << fname << ", lname: " << lname << ", city: " << city << ", year: " << year << ", salary: " << salary;
    return out;
     
}

std::istream& Person::readFromConsole(std::istream& in) {
    char str[90];
    using namespace std;

    cout << "SIN (9 digits): ";
    in.getline(str, 90);
    strncpy(SIN, str, SINLen);

    cout << "first name (20 characters max): ";
    in.getline(str, 90);
    strncpy(fname, str, fnameLen);

    cout << "last name: (20 characters max): ";
    in.getline(str, 90);
    strncpy(lname, str, lnameLen);

    cout << "city: (20 characters max): ";
    in.getline(str, 90);
    strncpy(city, str, cityLen);

    cout << "year: ";
    in >> year;

    cout << "salary: ";
    in >> salary;

    in.ignore();
    return in;
}

void Person::readInfoWithoutSIN(){
    char str[90];
    using namespace std;

    cout << "first name (20 characters max): ";
    cin.getline(str, 90);
    strncpy(fname, str, fnameLen);

    cout << "last name: (20 characters max): ";
    cin.getline(str, 90);
    strncpy(lname, str, lnameLen);

    cout << "city: (20 characters max): ";
    cin.getline(str, 90);
    strncpy(city, str, cityLen);

    cout << "year: ";
    cin >> year;

    cout << "salary: ";
    cin >> salary;

}

void Person::readFromFile(std::fstream& fin) {
    fin.read(SIN, SINLen);
    fin.read(fname, fnameLen);
    fin.read(lname, lnameLen);
    fin.read(city, cityLen);
    fin.read(reinterpret_cast<char*>(&year), sizeof(int));
    fin.read(reinterpret_cast<char*>(&salary), sizeof(long));
}

void Person::writeToFile(std::fstream& fout) {
    fout.write(SIN, SINLen);
    fout.write(fname, fnameLen);
    fout.write(lname, lnameLen);
    fout.write(city,cityLen);
    fout.write(reinterpret_cast<const char*>(&year), sizeof(int));
    fout.write(reinterpret_cast<const char*>(&salary), sizeof(long));
}