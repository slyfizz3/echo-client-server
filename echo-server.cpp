#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <set>


using namespace std;

set<int> sock_arr;

void usage() {
	cout << "syntax : echo-server <port> [-e[-b]]\n";
	cout << "  -e : echo\n";
	cout << "  -b : broadcast\n";
	cout << "sample : echo-server 1234 -e -b\n";
}

struct Param {
	bool echo{false};
	bool broadcast{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {

		if(argc < 2){
			return 0;
		}

		port = atoi(argv[1]);

		for (int i = 2; i < argc; i++) {

			if (strcmp(argv[i], "-b") == 0) {
				broadcast = true;
				cout << "broadcast on" << endl;
				continue;
			}
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				cout << "echo on" << endl;
				continue;
			}
		}
		if ((echo|broadcast) != true){
			cout << "no option" << endl;
		}
		return port != 0;
	}
} param;

void recvThread(int sd) {
	cout << "connected\n";
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			cerr << "recv return " << res << endl;
			perror("recv");
			sock_arr.erase(sd);
			break;
		}
		buf[res] = '\0';
		cout << buf << endl;
		if (param.broadcast) {
			set<int>::iterator sock;
			for (sock=sock_arr.begin();sock !=sock_arr.end();sock++){
				res = send(*sock, buf, res, 0);
				if (res == 0 || res == -1) {
					cerr << "send return " << res << endl;
					perror("send");
					sock_arr.erase(*sock);
					break;
				}

			}
		}
		else if (param.echo) {
			res = send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				cerr << "send return " << res << endl;
				perror("send");
				sock_arr.erase(sd);
				break;
			}
		}

	}
	cout << "disconnected\n";
	close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int res;
	int optval = 1;
	res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		sock_arr.insert(cli_sd);
		thread* t = new thread(recvThread, cli_sd);
		t->detach();
	}
	close(sd);
}
