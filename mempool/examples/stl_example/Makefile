OUT=example
SRC=                \
  ../../mempool.cpp \
  example.cpp

all: clean build run

clean:
	rm -f ${OUT} 

build:
	g++ --std=c++11 ${SRC} -o ${OUT}

run:
	./${OUT}
