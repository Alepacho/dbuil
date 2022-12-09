CC     = clang++
CFLAGS = -std=c++17 -Wall -g
IPATH  = ./inc

all: clean
	$(CC) $(CFLAGS) ./src/*.cpp $(IPATH)/md5.cpp -I$(IPATH) -o ./build

clean: 
	rm -rf build