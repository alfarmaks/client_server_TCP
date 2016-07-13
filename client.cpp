#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stropts.h>
#include <errno.h>
typedef int SOCKET;
typedef int INVALID_SOCKET;
#define INVALID_SOCKET ~0
#define SOCKET_ERROR -1
#define SD_SEND 0x01
#endif

using namespace std;


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "27015"
#define IP "localhost"
#define NO_ERROR 0L

int Close(SOCKET &sock){
#ifdef _WIN32
	return closesocket(sock);
#else
	return close(sock);
#endif
}

int sockInit(void)
{
#ifdef _WIN32
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(2, 2), &wsa_data);
#else
	return 0;
#endif
}

int sockQuit(void){
#ifdef _WIN32
	return WSACleanup();
#else
	return 0;
#endif
}

#ifndef _WIN32
int _kbhit() {
    static bool inited = false;
    int left;

    if (!inited) {
        termios t;
        tcgetattr(0, &t);
        t.c_lflag &= ~ICANON;
        tcsetattr(0, TCSANOW, &t);
        setbuf(stdin, NULL);
        inited = true;
    }

    ioctl(0, FIONREAD, &left);

    return left;
}
#endif

int ioctl(SOCKET soct, long cmd, u_long* argp){
#ifdef _WIN32
return ioctlsocket(soct, cmd, &argp);
#else
return ioctl(soct, cmd, &argp);
#endif
}


int main(int argc, char **argv)
{
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char sendbuf[BUFSIZ] = { 0 };
	char recvbuf[DEFAULT_BUFLEN] = { 0 };
	char Ip[BUFSIZ] = { 0 };
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	cout << "-------------------------------------------------------------------------------" << endl;
	cout << "\t[Client]\tWelcome to the Chatly" << endl;
	cout << "-------------------------------------------------------------------------------" << endl;
	cout << "Enter the host ip (\"l\" for localhost): " << endl;
	cin.getline(Ip, sizeof(Ip));
	if (!strcmp(Ip, "l"))
		strcpy(Ip, "localhost");
	cout << "Connecting...";
	// Initialize Winsock
	iResult = sockInit();
	if (iResult != 0) {
		//printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	//ZeroMemory(&hints, sizeof(hints));
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(Ip, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		sockQuit();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET){
                        //printf("socket failed with error: %d\n", WSAGetLastError());
			sockQuit();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR){
			Close(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}


	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		sockQuit();
		return 1;
	}
	cout << "Done!" << endl;

	//-------------------------
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the 
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled; 
	// If iMode != 0, non-blocking mode is enabled.
	u_long iMode = 1;
        iResult = ioctl(ConnectSocket, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
                printf("ioctlsocket failed with error: %d\n", iResult);

	// Receive until the peer closes the connection
	do {
		char str[BUFSIZ] = { 0 };
		memset(recvbuf, 0, recvbuflen);
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0){
			cout << "[Server]: " << recvbuf << endl;
		}
		else if (iResult == 0){
			printf("Connection closed\n");
			Close(ConnectSocket);
			sockQuit();
			return 1;
		}
		// Send an initial buffer
		if (_kbhit()){
			int chars = 0;
			do{
				cout << "[Client]: ";
				cin.getline(str, sizeof(str));
				chars = strlen(str);
				if (chars > 0){
					iResult = send(ConnectSocket, str, (int)strlen(str), 0);
					if (iResult == SOCKET_ERROR) {
						Close(ConnectSocket);
						sockQuit();
						return 1;
					}
				}
			} while (chars <= 0);
		}

	} while (true);
	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {

		Close(ConnectSocket);
		sockQuit();
		return 1;
	}
	// cleanup
	Close(ConnectSocket);
	sockQuit();
	return 0;
}
