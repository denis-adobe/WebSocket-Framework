#include "WebSocket.h"

int main(int argc, char**argv)
{
	WebSocket* webSocket = new WebSocket(8181);
	webSocket->Listen();

	return 0;
}