#ifndef __WEBSOCKET_H
#define __WEBSOCKET_H

/**
	SHA1: http://code.google.com/p/smallsha1/
	Base64: http://www.adp-gmbh.ch/cpp/common/base64.html
**/

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "sha1.h"
#include "Base64Encoder.h"


#if defined _WIN32
	#include <WinSock2.h>
	#include <Windows.h>
	#pragma comment(lib, "Ws2_32.lib")
#elif defined __unix__
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

typedef std::map<std::string, std::string> handshakeMap;


class WebSocket
{
protected:
	WSAData wsData;
	SOCKET webSock;
	sockaddr_in addr;

public:
	WebSocket(u_short port);
	~WebSocket();
	void CloseConnection();
	void Listen();

	bool getListenState();
private:
	bool HandShakeClient(SOCKET *webclient, std::string clientHandshake);
	handshakeMap parseClientHandshake(std::string clientHandShake);
	void Error(int code);
		
	bool Listening;
	u_short _port;
	std::vector<SOCKET> clients;
};


#endif // __WEBSOCKET_H