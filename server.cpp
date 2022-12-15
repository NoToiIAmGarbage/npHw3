#include <bits/stdc++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <fstream>

using namespace std;

#define max_clients 30

int opt = 1, PORT;
struct sockaddr_in addr;
int addrlen;

struct room {
	room() {
		member.clear();
	};
	room(bool x, int y, int z, int w) : pub(x), id(y), started(false), inv(z), manager(w) {
		member.clear();
	}; 
	bool pub;
	bool started;
	long long id;
	long long manager;
	long long inv;
	string ans;
	int cur, round;
	vector<int> member;
};

struct invitation {
	invitation() : valid(false) {};
	invitation(int x, string y) : roomId(x), inviter(y), valid(true) {};
	bool valid;
	long long roomId;
	string inviter; // email
	bool operator == (const invitation& x) {
		if(valid != x.valid) {
			return false;
		}
		if(roomId != x.roomId) {
			return false;
		}
		if(inviter != x.inviter) {
			return false;
		}
		return true;
	}
};

map<string, pair<string, string> > user2data;
map<string, pair<string, string> > email2data;
map<string, vector<invitation> > user2invitation;
map<string, int> user2state;
map<long long, room> gameRoom;

long long inRoom[max_clients + 5];

string state[max_clients + 5];

// string game[max_clients + 5];

vector<string> process_string(string str) {
	vector<string> res;
	string tmp = "";
	for(char x : str) {
		if(x == ' ') {
			res.push_back(tmp);
			tmp = "";
		}
		else {
			if((int)x <= 31) {
				continue;
			}
			tmp += x;
		}
	}
	res.push_back(tmp);
	return res;
}

bool is4digit(string str) {
	if(str.size() != 4) {
		return false;
	}
	for(int i = 0; i < 4; i ++) {
		if(str[i] > '9' || str[i] < '0') {
			return false;
		}
	}
	return true;
}

pair<int, int> comp(string one, string two) {
	bool vis[2][4];
	memset(vis, 0, sizeof(vis));
	pair<int, int> res = {0, 0};
	for(int i = 0; i < 4; i ++) {
		if(one[i] == two[i]) {
			res.first ++;
			vis[0][i] = vis[1][i] = true;
		}
	}
	for(int i = 0; i < 4; i ++) {
		for(int j = 0; j < 4; j ++) {
			if(!vis[0][i] && !vis[1][j] && one[i] == two[j]) {
				vis[0][i] = vis[1][j] = true;
				res.second ++;
			}
		}
	}
	return res;
}

class Connection {
public:
	void updateLoginNumber() {
		Aws::Client::ClientConfiguration c;
		Aws::S3::S3Client s3_client(c);
		Aws::S3::Model::GetObjectRequest request;
		request.SetBucket("nphw3bucket");
		request.SetKey("loginNumber");

		Aws::S3::Model::GetObjectOutcome outcome =
		            s3_client.GetObject(request);

		Aws::IOStream& body = outcome.GetResult().GetBody();

		for(int i = 0; i < 3; i ++) {
			body >> loginNum[i];
		}

	}

	void writeLoginNumber() {
		Aws::Client::ClientConfiguration c;
		Aws::S3::S3Client s3_client(c);
		Aws::S3::Model::PutObjectRequest request;
		request.SetBucket("nphw3bucket");
		request.SetKey("loginNumber"); 
		
		const shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::StringStream>("");
	
		string objectContent = to_string(loginNum[0]) + ' ' + to_string(loginNum[1]) + ' ' + to_string(loginNum[2]);
		cout << objectContent << '\n';
		*inputData << objectContent.c_str();
		request.SetBody(inputData);
		Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);	
	}
	void process_register(vector<string> comm) {
		if(comm.size() != 4) {
			sendMes("Usage: register <username> <email> <password>");
			return;
		}
		if(user2data.find(comm[1]) != user2data.end()) {
			sendMes("Username or Email is already used");
			return;
		}
		if(email2data.find(comm[2]) != email2data.end()) {
			sendMes("Username or Email is already used");
			return;
		}
		email2data[comm[2]] = {comm[1], comm[3]};
		user2data[comm[1]] = {comm[2], comm[3]};
		user2state[comm[1]] = -1;
		sendMes("Register Successfully");
	}

	void process_login(vector<string> comm, int ind) {
		if(comm.size() != 3) {
			sendMes("Usage: login <username> <password>", ind);
			return;
		}
		if(user2data.find(comm[1]) == user2data.end()) {
			sendMes("Username does not exist", ind);
			return;
		}
		if(state[ind] != "") {
			sendMes("You already logged in as " + state[ind], ind);
			return;	
		}
		if(user2state[comm[1]] != -1) {
			sendMes("Someone already logged in as " + comm[1], ind);
			return;		
		}
		if(user2data[comm[1]].second != comm[2]) {
			sendMes("Wrong password", ind);
			return;
		}
		sendMes("Welcome, " + comm[1], ind);
		state[ind] = comm[1];
		user2state[comm[1]] = ind;
		updateLoginNumber();
		loginNum[0] ++;
		writeLoginNumber();
	}

	void process_logout(int ind) {
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}

		if(inRoom[ind] != -1) {
			sendMes("You are already in game room " + to_string(inRoom[ind]) + ", please leave game room", ind);
			return;
		}
		sendMes("Goodbye, " + state[ind], ind);
		user2state[state[ind]] = -1;
		state[ind] = "";
		updateLoginNumber();
		loginNum[0] --;
		writeLoginNumber();
	}

	void process_create_public_room(vector<string> comm, int ind) {
		int id = stoi(comm[3]);
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] != -1) {
			sendMes("You are already in game room " + to_string(inRoom[ind]) + ", please leave game room", ind);
			return;
		}
		if(gameRoom.find(id) != gameRoom.end()) {
			sendMes("Game room ID is used, choose another one", ind);
			return;
		}
		gameRoom[id] = room(0, id, -1, ind);
		sendMes("You create public game room " + comm[3], ind);
		inRoom[ind] = id;
		gameRoom[id].member.push_back(ind);
	}

	void process_create_private_room(vector<string> comm, int ind) {
		int id = stoi(comm[3]), inv = stoi(comm[4]);
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] != -1) {
			sendMes("You are already in game room " + to_string(inRoom[ind]) + ", please leave game room", ind);
			return;
		}
		if(gameRoom.find(id) != gameRoom.end()) {
			sendMes("Game room ID is used, choose another one", ind);
			return;
		}
		gameRoom[id] = room(1, id, inv, ind);
		sendMes("You create private game room " + comm[3], ind);
		inRoom[ind] = id;
		gameRoom[id].member.push_back(ind);
	}

	void process_list_room(vector<string> comm) {
		if(gameRoom.empty()) {
			sendMes("List Game Rooms\nNo Rooms");
		}
		else {
			string res = "List Game Rooms\n";
			int now = 1;
			for(auto& [x, y] : gameRoom) {
				string tmp;
				res += to_string(now ++) + ". ";
				if(y.pub) {
					res += "(Private)";
				}
				else {
					tmp += "(Public)";
				}
				tmp += " Game Room " + to_string(x);
				if(y.started) {
					tmp += " has started";
				}
				else {
					tmp += " is open for players";
				}
				res += tmp + '\n';
			}
			sendMes(res);
		}
	}

	void process_list_user(vector<string> comm) {
		if(user2data.empty()) {
			sendMes("List Users\nNo Users");
		}
		else {
			string res = "List Users\n";
			int now = 1;
			for(auto& [x, y]: user2data) {
				string tmp = to_string(now++) + ". " + x + '<' + y.first + '>';
				if(user2state[x] != -1) {
					tmp += " Online";
				}
				else {
					tmp += " Offline";
				}
				res += tmp + '\n';
			}
			sendMes(res);
		}
	}

	void process_join_room(vector<string> comm, int ind) {
		int id = stoi(comm[2]);
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] != -1) {
			sendMes("You are already in game room " + to_string(inRoom[ind]) + ", please leave game room", ind);
			return;
		}
		if(gameRoom.find(id) == gameRoom.end()) {
			sendMes("Game room " + to_string(id) + " is not exist", ind);
			return;
		}
		if(gameRoom[id].pub) {
			sendMes("Game room is private, please join game by invitation code", ind);
			return;
		}
		if(gameRoom[id].started) {
			sendMes("Game has started, you can't join now", ind);
			return;
		}
		inRoom[ind] = id;
		sendMes("You join game room " + comm[2], ind);
		for(int x : gameRoom[id].member) {
			sendMes("Welcome, " + state[ind] + " to game!", x);
		}
		gameRoom[id].member.push_back(ind);
	}

	void process_invite(vector<string> comm, int ind) {
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] == -1) {
			sendMes("You did not join any game room", ind);
			return;
		}
		if(!gameRoom[inRoom[ind]].pub || gameRoom[inRoom[ind]].manager != ind ) {
			sendMes("You are not private game room manager", ind);
			return;
		}
		if(user2state[ email2data[comm[1]].first ] == -1) {
			sendMes("Invitee not logged in", ind);
			return;
		}
		sendMes("You receive invitation from " + state[ind] + "<" + user2data[state[ind]].first + ">", user2state[ email2data[comm[1]].first ]);
		sendMes("You send invitation to " + email2data[comm[1]].first + "<" + comm[1] + ">", ind);
		vector<invitation> &v = user2invitation[ email2data[comm[1]].first ];
		v.push_back(invitation(inRoom[ind], user2data[state[ind]].first));
		sort(v.begin(), v.end(), [](auto x, auto y) {
			return x.roomId < y.roomId;
		});
		v.resize(unique(v.begin(), v.end()) - v.begin());
	}

	void process_list_invitations(int ind) {
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		string res = "List invitations\n";
		int now = 1;
		for(auto x : user2invitation[state[ind]]) {
			if(!x.valid) {
				continue;
			}
			res += to_string(now ++) + ". ";
			res += email2data[x.inviter].first + "<" + x.inviter + "> invite you to join game room ";
			res += to_string(x.roomId) + ", invitation code is " + to_string(gameRoom[x.roomId].inv) + '\n';  
		}
		if(res == "List invitations\n") {
			res += "No Invitations";
		}
		sendMes(res, ind);
	}

	void process_accept(vector<string> comm, int ind) {
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] != -1) {
			sendMes("You are already in game room " + to_string(inRoom[ind]) + ", please leave game room", ind);
			return;
		}
		int id = -1; 
		for(auto &x : user2invitation[state[ind]]) {
			if(x.valid && x.inviter == comm[1]) {
				id = x.roomId;
				break;
			}
		}
		if(id == -1) {
			sendMes("Invitation not exist", ind);
			return;
		}

		auto r = gameRoom[id];

		if(r.inv != stoi(comm[2])) {
			sendMes("Your invitation code is incorrect", ind);
			return;
		}

		if(r.started) {
			sendMes("Game has started, you can't join now", ind);
			return;
		}

		inRoom[ind] = id;
		sendMes("You join game room " + to_string(id), ind);
		for(int x : gameRoom[id].member) {
			sendMes("Welcome, " + state[ind] + " to game!", x);
		}
		gameRoom[inRoom[ind]].member.push_back(ind);
	}

	void process_leave_room(int ind) {
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] == -1) {
			sendMes("You did not join any game room", ind);
			return;
		}

		auto &r = gameRoom[inRoom[ind]];

		if(ind == r.manager) {
			for(auto &[_, v] : user2invitation) {
				for(auto& x : v) {
					if(x.roomId == inRoom[ind]) {
						x.valid = false;
					}
				}
			}

			sendMes("You leave game room " + to_string(inRoom[ind]), ind);
			for(int x : r.member) {
				if(x == ind) {
					continue;
				}
				sendMes("Game room manager leave game room " + to_string(inRoom[x]) + ", you are forced to leave too", x);
				inRoom[x] = -1;
			}
			gameRoom.erase(inRoom[ind]);
			inRoom[ind] = -1;
		}
		else if(r.started) {
			sendMes("You leave game room " + to_string(inRoom[ind]) + ", game ends", ind);
			r.member.erase(find(r.member.begin(), r.member.end(), ind));
			inRoom[ind] = -1;
			r.started = false;
			for(int x : r.member) {
				sendMes(state[ind] + " leave game room " + to_string(inRoom[x]) + ", game ends", x);
			}
		}
		else {
			sendMes("You leave game room " + to_string(inRoom[ind]), ind);
			r.member.erase(find(r.member.begin(), r.member.end(), ind));
			inRoom[ind] = -1;
			for(int x : r.member) {
				sendMes(state[ind] + " leave game room " + to_string(inRoom[x]), x);
			}
		}
	}

	void process_start_game(vector<string> comm, int ind) {
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] == -1) {
			sendMes("You did not join any game room", ind);
			return;
		}
		auto &r = gameRoom[inRoom[ind]];
		if(r.manager != ind) {
			sendMes("You are not game room manager, you can't start game", ind);
			return;
		}
		if(r.started) {
			sendMes("Game has started, you can't start again", ind);
			return;
		}
		if(comm.size() == 4 && !is4digit(comm[3])) {
			sendMes("Please enter 4 digit number with leading zero", ind);
			return;
		}


		r.started = true;
		for(int x : r.member) {
			sendMes("Game start! Current player is " + state[r.member[0]], x);
		}
		r.round = stoi(comm[2]);
		if(comm.size() == 4) {
			r.ans = comm[3];
		}
		else {
			r.ans = "0000";
		}
		r.cur = 0;
	}

	void process_guess(vector<string> comm, int ind) {
		if(state[ind] == "") {
			sendMes("You are not logged in", ind);
			return;
		}
		if(inRoom[ind] == -1) {
			sendMes("You did not join any game room", ind);
			return;
		}
		auto &r = gameRoom[inRoom[ind]];
		if(!r.started) {
			if(ind == r.manager) {
				sendMes("You are game room manager, please start game first", ind);
			}
			else {
				sendMes("Game has not started yet\n", ind);
			}
			return;
		}
		if(ind != r.member[r.cur]) {
			sendMes("Please wait..., current player is " + state[r.member[r.cur]], ind);
			return;
		}
		if(!is4digit(comm[1])) {
			sendMes("Please enter 4 digit number with leading zero", ind);
			return;
		}

		auto res = comp(comm[1], r.ans);
		string name = state[r.member[r.cur]];

		if(res.first == 4) {
			string m = name + " guess \'" + comm[1] + "\' and got Bingo!!! " + name + " wins the game, game ends";
			for(int x : r.member) {
				sendMes(m, x);
			}
			r.started = false;
		}
		else {
			string m = name + " guess \'" + comm[1] + "\' and got " + "\'" + to_string(res.first) + "A" + to_string(res.second) + "B\'";
			r.cur ++;
			if(r.cur == r.member.size()) {
				r.cur = 0; r.round --;
				if(r.round) {
					for(int x : r.member) {
						sendMes(m, x);
					}
				}
				else {
					for(int x : r.member) {
						sendMes(m + '\n' + "Game ends, no one wins", x);
					}
					r.started = false;
				}
			}
			else {
				for(int x : r.member) {
					sendMes(m, x);
				}
			}
		}
	}

	void process_status(int ind) {
		updateLoginNumber();
		string res;
		for(int i = 0; i < 3; i ++) {
			res += "Server" + to_string(i + 1) + ": " + to_string(loginNum[i]) + '\n';
		}
		sendMes(res, ind);
	}

	void process_command(string str, int ind = -1) {
		auto comm = process_string(str);
		if(ind == -1) {
			if(comm[0] == "register") {
				process_register(comm);
			}
			else if(comm[0] == "list" && comm[1] == "users") {
				process_list_user(comm);
			}
			else if(comm[0] == "list" && comm[1] == "rooms") {
				process_list_room(comm);
			}
			else {
				sendMes("Not a valid command");
			}
			return;
		}

		if(comm[0] == "login") {
			process_login(comm, ind);
		}
		else if(comm[0] == "logout") {
			process_logout(ind);
		}
		else if(comm[0] == "create" && comm[1] == "public" && comm[2] == "room") {
			process_create_public_room(comm, ind);
		}
		else if(comm[0] == "create" && comm[1] == "private" && comm[2] == "room") {
			process_create_private_room(comm, ind);
		}
		else if(comm[0] == "join" && comm[1] == "room") {
			process_join_room(comm, ind);
		}
		else if(comm[0] == "invite") {
			process_invite(comm, ind);
		}
		else if(comm[0] == "list" && comm[1] == "invitations") {
			process_list_invitations(ind);
		}
		else if(comm[0] == "accept") {
			process_accept(comm, ind);
		}
		else if(comm[0] == "leave" && comm[1] == "room") {
			process_leave_room(ind);
		}
		else if(comm[0] == "start" && comm[1] == "game") {
			process_start_game(comm, ind);
		}
		else if(comm[0] == "guess") {
			process_guess(comm, ind);
		}
		else if(comm[0] == "exit") {
			process_exit(ind);
		}
		else if(comm[0] == "status") {
			process_status(ind);
		}
		else {
			sendMes("Not a valid command", ind);
		}
	}

	void process_exit(int ind) {
		close(clients[ind]);
		clients[ind] = -1;
		// cerr << "Clients" << ind << "Exit" << '\n';
		if(state[ind] != "") {
			if(inRoom[ind] != -1) {
				auto &r = gameRoom[inRoom[ind]];


				if(ind == r.manager) {
					for(auto &[_, v] : user2invitation) {
						for(auto& x : v) {
							if(x.roomId == inRoom[ind]) {
								x.valid = false;
							}
						}
					}
					for(int x : r.member) {
						if(x == ind) {
							continue;
						}
						inRoom[x] = -1;
					}
					gameRoom.erase(inRoom[ind]);
					inRoom[ind] = -1;
				}
				else {
					r.member.erase(find(r.member.begin(), r.member.end(), ind));
					inRoom[ind] = -1;
					r.started = false;
				}
			}
			user2state[state[ind]] = -1;
			state[ind] = "";
			updateLoginNumber();
			loginNum[0] --;
			writeLoginNumber();
		}
	}

	Connection() {
		memset(loginNum, 0, sizeof(loginNum));
		writeLoginNumber();
		
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(PORT);
		addrlen = sizeof(addr);
		// setup address

		for(int i = 0; i < max_clients; i ++) {
			inRoom[i] = -1;
		}
	}

	void UDP() {
		udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

		if( bind(udp_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			cerr << "Udp socket fail to bind\n";
		}

		// cerr << "UDP server is running\n";

	}

	void TCP() {
		for(int i = 0; i < max_clients; i ++) {
			clients[i] = -1;
		}
		master_socket = socket(AF_INET, SOCK_STREAM, 0);
		// file descriptor of master socket

		if(!master_socket) {
			cerr << "Master socket failed to establish.\n";
			exit(-1);
		} // handle error event


		if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
			cerr << "Master socket fail to be configured.\n";
			exit(0);
		}
		// configure master socket

		if( bind(master_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			cerr << "Master socket fail to bind\n";
		}
		// bind the master socket to the address struct

		if( listen(master_socket, max_clients) < 0) {
			cerr << "Failed to start listening.\n";
			exit(0);
		}
		// starts listening at PORT

		// cerr << "TCP server is running\n";
	}

	void handle_event() {
		while(true) {
			FD_ZERO(&readfds); // clear out

			FD_SET(master_socket, &readfds);
			FD_SET(udp_socket, &readfds);

			max_sd = max(udp_socket, master_socket) + 1; // for select function

			for(int i = 0; i < max_clients; i ++) {
				int sd = clients[i];

				if(sd > 0) {
					FD_SET(sd, &readfds);
				}

				max_sd = max(max_sd, sd);
			}

			activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

			if ( (activity < 0) && (errno != EINTR) ) {  
	            printf("select error");  
	        }

			if(FD_ISSET(master_socket, &readfds)) {
				new_socket = accept(master_socket, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
				if(new_socket < 0) {
					cerr << "New socket fail to accept\n";
					exit(0);
				}

				// cerr << "New connection.\n";
				// cerr << "FD : " << new_socket << '\n';
				// cerr << "IP : " << inet_ntoa(addr.sin_addr) << '\n';
				// cerr << "Port : " << ntohs(addr.sin_port) << '\n';

				// string welcome_mes = "Fuck you";

				// if(send(new_socket, welcome_mes.c_str(), welcome_mes.size(), 0) != welcome_mes.size()) {
				// 	cerr << "Failed to send welcome meassage\n";
				// }

				for(int i = 0; i < max_clients; i ++) {
					if(clients[i] == -1) {
						clients[i] = new_socket;
						break;
					}
				}

			}

			if(FD_ISSET(udp_socket, &readfds)) {
				int n = recvfrom(udp_socket, buffer, 1024, 0, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
				buffer[n] = '\0';
				process_command(buffer);
			}


			for(int i = 0; i < max_clients; i ++) {
				int sd = clients[i];
				if(FD_ISSET(sd, &readfds)) {
					int val = read(sd, buffer, 1024);
					buffer[val] = '\0';
					// cerr << buffer << '\n';
					if(!val) { // closing
						// cerr << "tcp get msg: exit\n";
						process_exit(i);
						// close(sd);
					}
					else {
						process_command(buffer, i);
					}
				}
			}

		}
	}

	void sendMes(string str, int ind = -1) {
		if(str.back() != '\n') {
			str += '\n';
		}
		if(ind != -1) {
			send(clients[ind], str.c_str(), str.size(), 0);
		}
		else {
			sendto(udp_socket, str.c_str(), str.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
		}
	}
private:
	int master_socket, max_sd;

	int loginNum[3];

	int udp_socket;

	int activity, new_socket;

	fd_set readfds;
	
	int clients[max_clients];
	char buffer[1024];
};

int main(int argc, char** argv) {

	PORT = 8888;
	Aws::SDKOptions options;
	Aws::InitAPI(options);
	
	Connection lol;

	lol.UDP(); lol.TCP(); 

	lol.handle_event();
}
