if [ -d "./build" ]
then
	rm -r build
fi

mkdir build

g++ -std=c++17 server.cpp -o ./build/server