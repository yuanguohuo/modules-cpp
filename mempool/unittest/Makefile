all: clean build run

build:
	g++ -L/usr/local/lib64       \
	    -I/usr/local/include/    \
	    --std=c++11              \
	    ../mempool.cpp           \
	    test1.cpp                \
	    -lgtest                  \
	    -lpthread                \
		-o test1

clean:
	rm -f test1

run:
	./test1
