if [ -d "./build" ]
then
	rm -r build
fi

mkdir build

g++ -std=c++17 -I /usr/local/include/ server.cpp /usr/local/lib/libaws-cpp-sdk-s3.so /usr/local/lib/libaws-cpp-sdk-core.so -o ./build/server
