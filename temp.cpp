#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;

int opt = 1, PORT;
const int mx_q = 100;
struct sockaddr_in addr;
int addrlen;

char buffer[1024];

class TCP {
public:
	TCP() {
		TCP_fd = socket(AF_INET, SOCK_STREAM, 0);
		// file descriptor of TCP

		client_fd = connect(TCP_fd, (struct sockaddr*)&addr, sizeof(addr));
		if(client_fd < 0) {
			cout << "connection failed.\n";
			exit(0);
		}
		// connect to server

	}
	void readMes() {
		int val = read(TCP_fd, buffer, 1024);
		buffer[val] = '\0';
		cout << buffer;
	}

	void close_connection() {
		close(client_fd);
	}

	void sendMes(string str) {
		send(TCP_fd, str.c_str(), str.size(), 0);

	}
private:
	int TCP_fd, client_fd;
};

class UDP {
public:
	UDP() {
		UDP_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(UDP_fd < 0) {
			cout << "UDP fd failed creating\n";
			exit(0);
		}
	}

	void close_connection() {
		close(UDP_fd);
	}

	void sendMes(string str) {
		sendto(UDP_fd, str.c_str(), str.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
	}

	void readMes() {
		int val = recvfrom(UDP_fd, buffer, 1024, 0, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
		buffer[val] = '\0';
		cout << buffer;
	}
private:
	int UDP_fd;
};

int main(int argc, char** argv) {

	PORT = 8888; // extract the port

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
		cout << "\nInvalid address/ Address not supported\n";
		return 0;
	}
	addrlen = sizeof(addr);
	// setup address
	
	TCP tcp; UDP udp;
	string str;
	set<string> udp_comm({"game-rule", "register", "list"});
	while(getline(cin, str)) {
		string tmp = "";
		for(int i = 0; i < str.size(); i ++) {
			if(str[i] == ' ') {
				break;
			}
			tmp += str[i];
		}
		if(tmp == "exit") {
			udp.close_connection();
			tcp.close_connection();
			return 0;
		}
		else if(str == "list invitations") {
			tcp.sendMes(str);
			tcp.readMes();
		}
		else if(udp_comm.count(tmp)) {
			udp.sendMes(str);
			udp.readMes();
		}
		else {
			tcp.sendMes(str);
			tcp.readMes();
		}
	}
	return 0;
}