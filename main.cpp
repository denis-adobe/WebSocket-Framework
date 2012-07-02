#include "WebSocket.h"

void on_open(void* param)
{
	std::cout << "Hello World testing my callback Wrappers\n\n";
	return;
}

void on_close(void* param)
{
	std::cout << "Closed connection with client\n\n";
	return;
}

void on_recv(void* param)
{
	char* data = (char*)param;

	std::cout << "Received: " << data << std::endl;

	return;
}

int main(int argc, char**argv)
{
	WebSocket::WServer* server = new WebSocket::WServer(8181);
	server->registerCallback(WebSocket::ON_OPEN,&on_open);
	server->registerCallback(WebSocket::ON_CLOSE,&on_close);
	server->registerCallback(WebSocket::ON_RECV,&on_recv);

	server->Listen();

	return 0;
}