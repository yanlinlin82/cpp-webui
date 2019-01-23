#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

bool LoadContent(const std::string& filename, std::string& text)
{
	std::cerr << "Loading '" << filename << "'" << std::endl;
	std::ifstream file(filename);
	if (!file.is_open()) {
		return false;
	}
	text.clear();
	std::string line;
	while (std::getline(file, line)) {
		text += line + "\r\n";
	}
	std::cerr << " " << text.size() << " byte(s) loaded" << std::endl;
	return true;
}

std::string ParseRequest(const char* buf)
{
	std::string request;
	if (strncmp(buf, "GET ", 4) == 0) {
		const char* s = buf + 4;
		const char* p = strchr(s, ' ');
		request = std::string(s, p);
	}
	std::cerr << "Request: '" << request << "'" << std::endl;
	return request;
}

std::string GetExtName(const std::string& filename)
{
	auto pos = filename.find_last_of('.');
	return (pos == std::string::npos ? "" : filename.substr(pos));
}

std::string Process(const std::string& request)
{
	int status = 200;
	std::string text;
	std::string contentType;
	bool exists = true;
	if (!request.empty()) {
		auto ext = GetExtName(request);
		if (ext == ".css") {
			contentType = "text/css";
		} else if (ext == ".js") {
			contentType = "text/javascript";
		} else {
			contentType = "text/html";
		}

		if (request == "/hello.txt") {
			text = "Hello, CPP-WebUI!";
			contentType = "text/text";
		} else {
			if (!LoadContent("ext" + (request == "/" ? "/index.html" : request), text)) {
				status = 404;
				exists = false;
			}
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

		std::string request = ParseRequest(buf);
		std::string response = Process(request);

		for (size_t pos = 0; pos < response.size(); ) {
			int n = send(cs, response.c_str() + pos, response.size() - pos, 0);
			if (n <= 0) break;
			pos += n;
		}

		close(cs);
	}

	close(s);
	return 0;
}
