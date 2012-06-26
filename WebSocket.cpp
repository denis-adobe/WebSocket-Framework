#include "WebSocket.h"

WebSocket::WebSocket(u_short port) : _port(port)
{
#ifdef _WIN32
	if(WSAStartup(MAKEWORD(2,2),&wsData) != 0) {
		WebSocket::Error();
	}
#endif
	if((webSock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET) {
		WebSocket::Error();
	}

}

WebSocket::~WebSocket()
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

void WebSocket::Error()
{
#ifdef _WIN32
	std::cout << "FATAL ERROR! Code: " << WSAGetLastError() << std::endl;
#endif

#ifdef __unix__
	std::cout << "FATAL ERROR!!!! " << std::endl;
#endif
}

void WebSocket::Listen()
{
	const int MAX_BUF_LEN = 1024;
	
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);

	if(bind(webSock,(struct sockaddr *)&addr,sizeof(addr)) == SOCKET_ERROR) {
		WebSocket::Error();
	}

	if(listen(webSock,SOMAXCONN ) == SOCKET_ERROR) {
		WebSocket::Error();
	}
	std::cout << "Listening for connections on port " << _port << "\n\n\n\n";

	while(webSock) {
		WebSocketClient *webClient = AcceptClient(); // Allocate off the heap

		if(isValidClient(webClient))
			CreateNewClient(webClient);
	}

	WebSocket::Error();
}

bool WebSocket::isValidClient(WebSocketClient *client)
{
	if(client->sock != INVALID_SOCKET && (ClientVersion)client->version != 0)
		return true;
	return false;
}
WebSocketClient *WebSocket::AcceptClient()
{
	struct sockaddr info;
	SOCKET client = accept(webSock,&info,NULL);

	if(client == SOCKET_ERROR) {
		std::cout << "Invalid Client Socket...\n";
		WebSocket::Error();
		return new WebSocketClient(this,-1,13);
	}

	return new WebSocketClient(this,client,13);
	
}

void WebSocket::CreateNewClient(WebSocketClient *client)
{
#ifdef _WIN32
	client->handle = (HANDLE)_beginthreadex(NULL,0,&WebSocket::clientThread,client,NULL,NULL);
#endif
	clients.push_back(client);
	std::cout << "Client Connected..." << std::endl;
}

#ifdef _WIN32
unsigned __stdcall WebSocket::clientThread(void* param)
#elif defined(__unix__)
unsigned int WebSocket::clientThread(void* param)
#endif
{
	WebSocketClient *client = static_cast<WebSocketClient*>(param);
	WebSocket *self = client->self;

	int recvBytes;
	char buff[1024];
	memset(buff,0,sizeof(buff));

	int handshake = recv(client->sock,buff,sizeof(buff),0);

	if(!self->HandShakeClient(&client->sock,buff)) {
		self->CloseClient(client);
		return 0;
	}

	do {
		recvBytes = recv(client->sock,buff,sizeof(buff),NULL);
		
		if(recvBytes > 0) {
			std::cout << "Received " << recvBytes << " from client.." << std::endl;
		}
		
	} while(recvBytes > 0); 
	
	self->CloseClient(client);

	std::cout << "Client Disconnected...\n";

	return 0;
	
}

void WebSocket::CloseClient(WebSocketClient *client)
{
#ifdef _WIN32
	closesocket(client->sock);
	CloseHandle(client->handle);
#endif

	clients.erase(clients.begin() + (client->id -1));

	delete client;
}

handshakeMap WebSocket::parseClientHandshake(std::string clientHandShake)
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

bool WebSocket::HandShakeClient(SOCKET *webclient, std::string clientHandshake)
{
	handshakeMap clientMap = parseClientHandshake(clientHandshake);

	if(clientMap["Sec-WebSocket-Version"] != "13") {
		std::cout << "Client attempting to connect with a WebSocket version of " << clientMap["Sec-WebSocket-Version"] << ". This version is unsupported...\n";
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

	int sent = send(*webclient,handshake.c_str(),handshake.length(),NULL);

	if(sent > 0)
		return true;

	return false;
}