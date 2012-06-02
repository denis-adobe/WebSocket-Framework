#include "WebSocket.h"

WebSocket::WebSocket(u_short port) : _port(port)
{
	if(WSAStartup(MAKEWORD(2,2),&wsData) != 0) {
		WebSocket::Error(WSAGetLastError());
	}
	if((webSock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET) {
		WebSocket::Error(WSAGetLastError());
	}
	Listening = false;
}

WebSocket::~WebSocket()
{
	WSACleanup();
}

void WebSocket::Error(int code)
{
	std::cout << "Error\nCode: " << code << std::endl;
}

void WebSocket::Listen()
{
	const int MAX_BUF_LEN = 1024;

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);

	if(bind(webSock,(sockaddr*)&addr,sizeof(addr)) == SOCKET_ERROR) {
		WebSocket::Error(WSAGetLastError());
	}

	if(listen(webSock,SOMAXCONN) == SOCKET_ERROR) {
		WebSocket::Error(WSAGetLastError());
	}
	Listening = true;
	std::cout << "Listening for connections on port " << _port << "\n\n\n\n";

	while(webSock != SOCKET_ERROR) {
		SOCKET webClient = accept(webSock,NULL,NULL);
		std::cout << "Client Connected..." << std::endl;

		std::cout << "Handshaking..." << std::endl;
		char buff[MAX_BUF_LEN];
		memset(buff,0,sizeof(buff));

		int received = recv(webClient,buff,sizeof(buff),0);
		std::cout << "Recived Client Handshake of " << received << " bytes\n";

		if(HandShakeClient(&webClient,std::string(buff)))
			clients.push_back(webClient);

		// TODO: Handle Clients
	}

	WebSocket::Error(WSAGetLastError());
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
		closesocket(*webclient);
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

	if(sent > 0) {
		std::cout << "Sent Handshake Total of " << sent << " bytes\n\n\n";
		return true;
	}

	std::cout << "Failed to send data to client. Closing socket.\n";
	closesocket(*webclient);
	return false;
}

void WebSocket::CloseConnection() {
	closesocket(webSock);
}

bool WebSocket::getListenState()
{
	return Listening;
}