CXX = g++
CXXFLAGX = -std=c++17
LIBS = -lpthread

all: rafile

rafile: main.o Person.o 
	$(CXX) $(CXXFLAGX) -o rafile main.o Person.o $(LIBS) -I./include

main.o: main.cpp  
	$(CXX) $(CXXFLAGX) -c main.cpp $(LIBS) -I./include

Person.o: Person.cpp
	$(CXX) $(CXXFLAGX) -c Person.cpp

clean:
	-rm -f *.o