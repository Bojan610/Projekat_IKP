#pragma warning(disable:4996) 
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_PORT 27016

bool InitializeWindowsSockets();

int __cdecl main(int argc, char **argv)
{
	SOCKET connectSocket = INVALID_SOCKET;
	int iResult;

	if (InitializeWindowsSockets() == false)
		return 1;

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(DEFAULT_PORT);

	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
	}

	int data = 0;
	do {
		// slanje podatka
		char temp[10];
		itoa(data, temp, 10);

		iResult = send(connectSocket, temp, (int)sizeof(data), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		data++;
		Sleep(1500);
	} while (true);

	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

bool InitializeWindowsSockets()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}
