INCLUDE = -I /usr/local/include/

COMPILERFLAGS = -Wall -std=c++17
CC = g++
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)

All: project

project: test.cpp -laws-cpp-sdk-s3 -laws-cpp-sdk-core
	    $(CC) $(CFLAGS) -o tmp $^

clean:
	    rm *.o
