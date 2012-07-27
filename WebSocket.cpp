#include "WebSocket.h"

using namespace WebSocket;

WServer::WServer(u_short port) : _port(port)
{
#ifdef _WIN32
	if(WSAStartup(MAKEWORD(2,2),&wsData) != 0) {
		Error();
	}
#endif
	if((webSock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET) {
		Error();
	}
}

WServer::~WServer()
{
#ifdef _WIN32
	closesocket(webSock);
	WSACleanup();
#endif

#ifdef __unix__
	shutdown(webSock,SHUT_RDWR);
	close(webSock);
#endif
}

void WServer::Error()
{
#ifdef _WIN32
	std::cout << "FATAL ERROR! Code: " << WSAGetLastError() << std::endl;
#endif

#ifdef __unix__
	std::cout << "FATAL ERROR!!!! " << std::endl;
#endif
}

void WServer::Listen()
{
	const int MAX_BUF_LEN = 1024;
	
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);

	if(bind(webSock,(struct sockaddr *)&addr,sizeof(addr)) == SOCKET_ERROR) {
		Error();
	}

	if(listen(webSock,SOMAXCONN ) == SOCKET_ERROR) {
		Error();
	}
	std::cout << "Listening for connections on port " << _port << "\n\n\n\n";

	while(webSock) {
		WSClient *webClient = AcceptClient(); // Allocate off the heap

		if(isValidClient(webClient,false))
			CreateNewClient(webClient);
	}

	Error();
}

bool WServer::isValidClient(WSClient *client, bool checkVer)
{
	if(client->sock != INVALID_SOCKET) {
		if(checkVer) {
			if((WSClientVersion)client->version == 0)
				return false;
		} return true;
	}
	return false;
}

WSClient *WServer::AcceptClient()
{
	struct sockaddr_in addr;

#ifdef _WIN32
	int length = sizeof(addr);
	SOCKET client = accept(webSock,(sockaddr*)&addr,&length);
#endif

#ifdef __unix__
	socklen_t length = sizeof(addr);
	SOCKET client = accept(webSock,(sockaddr*)&addr,&length);
#endif

	const char* ipAddr = inet_ntoa(addr.sin_addr);

	if(client == SOCKET_ERROR) {
		std::cout << "Invalid Client Socket...\n";
		Error();
		return new WSClient(this,-1,NULL);
	}

	return new WSClient(this,client,ipAddr);
}

void WServer::CreateNewClient(WSClient *client)
{
#ifdef _WIN32
	client->handle = (HANDLE)_beginthreadex(NULL,0,&WServer::clientThread,client,NULL,NULL);
#endif

#ifdef __unix__
	pthread_create(&client->handle,NULL,WServer::clientThread,client);
#endif

	clients.push_back(client);
	std::cout << "Client Connected with id of " << client->id << " {" << client->ip << "} ..." << std::endl;
}

#ifdef _WIN32
unsigned __stdcall WServer::clientThread(void* param)
#elif defined(__unix__)
void *WServer::clientThread(void* param)
#endif
{
	WSClient *client = static_cast<WSClient*>(param);
	WServer *self = client->self;

	int recvBytes;
	char handshakeBuff[1024];
	memset(handshakeBuff,0,sizeof(handshakeBuff));

	int handshake = recv(client->sock,handshakeBuff,sizeof(handshakeBuff),0);

	if(!self->HandShakeClient(client,handshakeBuff)) {
		self->CloseClient(client);
		return NULL;
	}

	char buff[1024];
	memset(buff,0,sizeof(buff));

	do {
		recvBytes = recv(client->sock,buff,sizeof(buff),0);
		
		if(recvBytes > 0) {
			std::cout << "Received " << recvBytes << " bytes from client[" << client->id << "] {" << client->ip << "}.." << std::endl;
		}
		
	} while(recvBytes > 0); 

	std::cout << "Client " << client->id << " {" << client->ip << "}  Disconnected...\n";

	self->CloseClient(client);

	return NULL;
	
}

void WServer::CloseClient(WSClient *client)
{
#ifdef _WIN32
	closesocket(client->sock);
	CloseHandle(client->handle);
#endif

#ifdef __unix__
	close(client->sock);
#endif

	std::vector<WSClient*>::iterator it;
	for(it = clients.begin(); it != clients.end(); ++it) {
		if(*it == client) {
			clients.erase(it);
			break;
		}
	}

	delete client;
}

handshakeMap WServer::parseClientHandshake(std::string clientHandShake)
{
	handshakeMap toReturn;

	int found = clientHandShake.find_first_of(":");
	int position = clientHandShake.find_first_of("\n"); // Ignore the First HTTP GET Request

	while(found != std::string::npos) {
		std::string key = clientHandShake.substr(position+1,(found-position)-1);

		position = clientHandShake.find_first_of("\n",found);
		std::string value = clientHandShake.substr(found+2,((position-2)-found)-1); // found + 2 = theres a whitespace after :

		toReturn[key] = value;

		found = clientHandShake.find_first_of(":",found+1);
		std::string port = static_cast<std::ostringstream*>( &(std::ostringstream() << _port) )->str(); // If the Client is connecting via a URI such as http://google.com:8181 do this check otherwise the key => value pairs will be corrupt
		if(clientHandShake.substr(found+1,port.length()) == port)
			found = clientHandShake.find_first_of(":",found+1);
	}

	return toReturn;
}

bool WServer::HandShakeClient(WSClient *client, std::string clientHandshake)
{
	handshakeMap clientMap = parseClientHandshake(clientHandshake);

	client->version = atoi(clientMap["Sec-WebSocket-Version"].c_str());

	if(!isValidClient(client,true)) {
		std::cout << "Error: Invalid Client LN196\n";
		return false;
	}

	std::string handshake = "HTTP/1.1 101 Switching Protocols\r\n";
	handshake += "Upgrade: WebSocket\r\n";
	handshake += "Connection: Upgrade\r\n";

	std::string magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	std::string clientKey = clientMap["Sec-WebSocket-Key"] + magicString;

	unsigned char hash[20];
	sha1::calc(clientKey.c_str(),clientKey.length(),hash);

	std::string base64Encoded = base64_encode(hash,20);

	handshake += "Sec-WebSocket-Accept: " + base64Encoded + "\r\n";
	
	handshake += "\r\n";

	int sent = send(client->sock,handshake.c_str(),handshake.length(),0);

	if(sent > 0)
		return true;

	return false;
}
