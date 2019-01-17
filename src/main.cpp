#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

std::string Load(const std::string& filename)
{
	std::cerr << "Loading '" << filename << "'" << std::endl;
	std::ifstream file(filename);
	std::string text, line;
	while (std::getline(file, line)) {
		text += line + "\r\n";
	}
	std::cerr << " " << text.size() << " byte(s) loaded" << std::endl;
	return text;
}

int main()
{
	int s = socket(AF_INET, SOCK_STREAM, 0);

	int enable = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

	struct sockaddr_in addr = { };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(8080);

	bind(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr_in));

	listen(s, 10);

	std::cerr << "Visit http://localhost:8080/ in web browser" << std::endl;

	for (;;) {
		socklen_t len = sizeof(struct sockaddr_in);
		int cs = accept(s, reinterpret_cast<struct sockaddr*>(&addr), &len);
		if (cs == -1) break;

		std::cerr << "=== client(" << inet_ntoa(addr.sin_addr) << ") connected" << std::endl;

		char buf[65536];
		recv(cs, buf, sizeof(buf), 0);
		//std::cerr << "[RECV] " << buf << std::endl;

		int status = 200;
		std::string text;
		std::string contentType;
		bool exists = true;
		if (strncmp(buf, "GET ", 4) == 0) {
			const char* s = buf + 4;
			const char* p = strchr(s, ' ');
			std::string request(s, p);
			std::cerr << "Request: '" << request << "'" << std::endl;
			if (request == "/css/bootstrap.min.css") {
				text = Load("ext/css/bootstrap.min.css");
				contentType = "text/css";
			} else if (request == "/css/bootstrap-theme.min.css") {
				text = Load("ext/css/bootstrap-theme.min.css");
				contentType = "text/css";
			} else if (request == "/js/bootstrap.min.js") {
				text = Load("ext/js/bootstrap.min.js");
				contentType = "text/javascript";
			} else if (request == "/js/html5shiv.min.js") {
				text = Load("ext/js/html5shiv.min.js");
				contentType = "text/javascript";
			} else if (request == "/js/jquery.min.js") {
				text = Load("ext/js/jquery.min.js");
				contentType = "text/javascript";
			} else if (request == "/js/respond.min.js") {
				text = Load("ext/js/respond.min.js");
				contentType = "text/javascript";
			} else if (request == "/" || request == "/index.html") {
				text = Load("ext/index.html");
				contentType = "text/html";
			} else if (request == "/hello.txt") {
				text = "Hello, CPP-WebUI!";
				contentType = "text/text";
			} else {
				status = 404;
				exists = false;
			}
		}

		std::ostringstream ss;
		if (exists) {
			ss << "HTTP/1.1 " << status << " OK\r\n"
				"Content-Length: " << text.size() << "\r\n"
				"Content-Type: " << contentType << "\r\n"
				"\r\n" << text;
		} else {
			ss << "HTTP/1.1 " << status << " Page does not exist\r\n\r\n";
		}
		text = ss.str();

		for (size_t pos = 0; pos < text.size(); ) {
			int n = send(cs, text.c_str() + pos, text.size() - pos, 0);
			if (n <= 0) break;
			pos += n;
		}

		close(cs);
	}

	close(s);
	return 0;
}
