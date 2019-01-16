#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

const char* TEXT = "Hello, CPP-WebUI!";

int main()
{
	int s = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr = { };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(8080);

	bind(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr_in));

	listen(s, 10);

	for (;;) {
		socklen_t len = sizeof(struct sockaddr_in);
		int cs = accept(s, reinterpret_cast<struct sockaddr*>(&addr), &len);
		if (cs == -1) break;

		char buf[4096];
		recv(cs, buf, sizeof(buf), 0);

		size_t size = snprintf(buf, sizeof(buf),
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: %zd\r\n"
				"Content-Type: text/html\r\n"
				"\r\n"
				"%s", strlen(TEXT), TEXT);
		send(cs, buf, size, 0);

		close(cs);
	}

	close(s);
	return 0;
}
