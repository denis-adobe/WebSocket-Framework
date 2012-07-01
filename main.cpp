#include "WebSocket.h"

int main(int argc, char**argv)
{
	WebSocket::WServer* server = new WebSocket::WServer(8181);
	server->Listen();

	return 0;
}