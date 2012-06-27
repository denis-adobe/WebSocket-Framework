#ifndef __WEBSOCKET_H
#define __WEBSOCKET_H

/**
	SHA1: http://code.google.com/p/smallsha1/
	Base64: http://www.adp-gmbh.ch/cpp/common/base64.html
**/

#include <iostream>
#include <string.h>
#include <vector>
#include <map>
#include <sstream>
#include "sha1.h"
#include "Base64Encoder.h"


#if defined _WIN32
	#include <WinSock2.h>
	#include <Windows.h>
	#include <process.h>
	#pragma comment(lib, "Ws2_32.lib")
#elif defined __unix__
	typedef int SOCKET;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR -1
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
#endif

typedef std::map<std::string, std::string> handshakeMap;
struct WebSocketClient;


class WebSocket
{
protected:

	SOCKET webSock;
	struct sockaddr_in addr;

#ifdef _WIN32
	WSAData wsData;
	static unsigned __stdcall clientThread(void* param);
#endif

#ifdef __unix__
	static void *clientThread(void* param);
#endif
public:
	WebSocket(u_short port);
	~WebSocket();

	void Listen();
	void CloseClient(WebSocketClient *client);
	inline int getClientSize() { return clients.size(); }
private:
	enum ClientVersion
	{
		WS_VER8 = 8,
		WS_VER13 = 13,
	};

	WebSocketClient *AcceptClient();
	bool HandShakeClient(SOCKET *webclient, std::string clientHandshake);
	handshakeMap parseClientHandshake(std::string clientHandShake);
	void Error();

	bool isValidClient(WebSocketClient *client);

	void CreateNewClient(WebSocketClient *client);

	u_short _port;

	std::vector<WebSocketClient*> clients;
};

struct WebSocketClient {
		WebSocketClient(WebSocket *_self,SOCKET _sock,int _version) : 
				self(_self), sock(_sock), version(_version) { id = self->getClientSize(); };
		SOCKET sock;
		int version;
		int id;
		WebSocket* self;
#if defined(_WIN32)
		HANDLE handle;
#elif defined(__unix__)
		pthread_t handle;
#endif
};

#endif // __WEBSOCKET_H