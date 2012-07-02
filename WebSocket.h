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
#include <utility>
#include "sha1.h"
#include "Base64Encoder.h"
#include "WSFrame.h"

#if defined __APPLE__ // Stupid Apple...
#define __unix__
#endif

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
	#include <stdlib.h> // atoi
#endif

namespace WebSocket
{
		typedef std::map<std::string, std::string> handshakeMap;
		struct WSClient;
		struct CBWrapper;

		enum WSCallbackType
		{
			OPEN,
			ON_OPEN_BLOCKING,
			ON_OPEN,
			CLOSE,
			ON_CLOSE_BLOCKING,
			ON_CLOSE,
			RECV,
			ON_RECV_BLOCKING,
			ON_RECV
		};

		typedef void (*CallBack)(void*);
		typedef std::map<WSCallbackType,CallBack> callbackTemplate;

		enum WSClientVersion
		{
			WS_VER8 = 8,
			WS_VER13 = 13,
		};

		class WServer
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
			WServer(u_short port);
			~WServer();

			void registerCallback(WSCallbackType type, void (*func)(void*));

			void Listen();
			void CloseClient(WSClient *client);
			inline int getClientSize() { return clients.size(); }
		private:

			WSClient *AcceptClient();
			bool HandShakeClient(WSClient *client, std::string clientHandshake);
			handshakeMap parseClientHandshake(std::string clientHandShake);
			void Error();


			bool callbackExists(WSCallbackType type);
			bool unregisterCallback(WSCallbackType type);
			void parseCallback(WSCallbackType type,void* p);
#ifdef _WIN32
			static unsigned __stdcall callbackWrapper(void* param);
#endif

#ifdef __unix__
			static void *callbackWrapper(void* param);
#endif

			bool isValidClient(WSClient *client,bool checkVer);

			void CreateNewClient(WSClient *client);

			u_short _port;

			std::vector<WSClient*> clients;
			callbackTemplate callbackMap;
		};

		struct WSClient {
				WSClient(WServer *_self,SOCKET _sock, const char* _ip) : 
						self(_self), sock(_sock), ip(_ip) { id = self->getClientSize(); };
				SOCKET sock;
				int version;
				int id;
				WServer* self;
				const char* ip;
#if defined(_WIN32)
				HANDLE handle;
#elif defined(__unix__)
				pthread_t handle;
#endif
		};

		struct CBWrapper {
				CBWrapper(WSCallbackType _type, void* _param, WServer *_self) : type(_type),
					param(_param), self(_self) { };

				void* param;
				WSCallbackType type;
				WServer *self;
		};

}; // namespace WebSocket

#endif // __WEBSOCKET_H