#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../Common/QueueHeader.h"
#include "../Common/TaskHeader.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define MAX_CLIENT 10
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);} 
#define THREAD_POOL_SIZE 5

HANDLE hAddToQueueSemaphore;
HANDLE hGetQueueDataSemaphore;
HANDLE hSendToThreadPoolSemaphore;
HANDLE hSendToWRSemaphore;

CRITICAL_SECTION addToQueueCS;
CRITICAL_SECTION getQueueDataCS;
CRITICAL_SECTION tasksUpdateCS;

struct Queue* queue;
struct Task* task;
int tasksCount = 0;
int counter = 0;

bool InitializeWindowsSockets();
void Decomposition(SOCKET* acceptedSockets, int indexDeleted, int indexCurrentSize);
void WriteInFile(int data, int workerRole);

DWORD WINAPI addToQueue(LPVOID lpParam);
DWORD WINAPI sendToThreadPool(LPVOID lpParam);
DWORD WINAPI sendToWR(LPVOID lpParam);

int main(void)
{
	int iResult;
	int currentConnections = 0;
	char recvbuf[DEFAULT_BUFLEN];
	char recvData[10];
	queue = createQueue(12);
	task = createTasks(10);

	SOCKET listenSocket = INVALID_SOCKET;

	DWORD addToQueueThreadID;
	DWORD sendToThreadPoolThreadID;
	DWORD threadPoolThreadID[THREAD_POOL_SIZE];

	HANDLE hAddToQueueThread;
	HANDLE hSendToThreadPoolThread;
	HANDLE hThreadPoolThread[THREAD_POOL_SIZE];

	hAddToQueueSemaphore = CreateSemaphore(0, 0, 1, NULL);
	if (hAddToQueueSemaphore) {
		hAddToQueueThread = CreateThread(NULL, 0, &addToQueue, recvData, 0, &addToQueueThreadID);
	}

	hSendToThreadPoolSemaphore = CreateSemaphore(0, 0, 1, NULL);
	if (hSendToThreadPoolSemaphore) {
		hSendToThreadPoolThread = CreateThread(NULL, 0, &sendToThreadPool, NULL, 0, &sendToThreadPoolThreadID);
	}

	hSendToWRSemaphore = CreateSemaphore(0, 0, 1, NULL);
	if (hSendToWRSemaphore) {
		for (int i = 0; i < THREAD_POOL_SIZE; i++) {
			char temp[10];
			itoa(i, temp, 10);
			hThreadPoolThread[i] = CreateThread(NULL, 0, &sendToWR, temp, 0, &threadPoolThreadID[i]);
		}
	}

	InitializeCriticalSection(&addToQueueCS);
	InitializeCriticalSection(&getQueueDataCS);
	InitializeCriticalSection(&tasksUpdateCS);

	SOCKET acceptedSocket[MAX_CLIENT];
	for (int i = 0; i < MAX_CLIENT; i++) {
		acceptedSocket[i] = INVALID_SOCKET;
	}

	if (InitializeWindowsSockets() == false)
		return 1;

	addrinfo *resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(resultingAddress);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	unsigned long mode = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &mode);

	printf("Server initialized, waiting for clients.\n");

	fd_set readfds;

	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;

	while (true) {
		FD_ZERO(&readfds);

		if (currentConnections < MAX_CLIENT)
			FD_SET(listenSocket, &readfds);

		for (int i = 0; i < currentConnections; i++) {
			FD_SET(acceptedSocket[i], &readfds);
		}

		int result = select(0, &readfds, NULL, NULL, &timeVal);
		if (result == 0)
			continue;
		else if (result == SOCKET_ERROR)
			break;
		else {
			if (FD_ISSET(listenSocket, &readfds)) {
				acceptedSocket[currentConnections] = accept(listenSocket, NULL, NULL);
				if (acceptedSocket[currentConnections] == INVALID_SOCKET) {
					printf("accept failed with error: %d\n", WSAGetLastError());
					closesocket(listenSocket);
					WSACleanup();
					return 1;
				}

				unsigned long mode = 1;
				iResult = ioctlsocket(acceptedSocket[currentConnections], FIONBIO, &mode);
				currentConnections++;
			}

			for (int i = 0; i < currentConnections; i++) {
				if (acceptedSocket[i] != INVALID_SOCKET) {
					iResult = recv(acceptedSocket[i], recvbuf, DEFAULT_BUFLEN, 0);

					if (iResult > 0) {
							memcpy(recvData, recvbuf, sizeof(recvbuf));
							printf("Client sent: %s\n", recvData);
							
							ReleaseSemaphore(hAddToQueueSemaphore, 1, NULL);

					}
					else if (iResult == 0) {
						printf("Connection with client closed.\n");
						closesocket(acceptedSocket[currentConnections]);
						Decomposition(acceptedSocket, i, currentConnections);
					}
					else {
						printf("recv failed with error: %d\n", WSAGetLastError());
						closesocket(acceptedSocket[currentConnections]);
					}
				}
			}
		}
	}

	for (int i = 0; i < MAX_CLIENT; i++) {
		iResult = shutdown(acceptedSocket[i], SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket[i]);
			WSACleanup();
			return 1;
		}

		closesocket(acceptedSocket[i]);
	}

	SAFE_DELETE_HANDLE(hAddToQueueSemaphore);
	SAFE_DELETE_HANDLE(hAddToQueueThread);
	
	SAFE_DELETE_HANDLE(hSendToThreadPoolSemaphore);
	SAFE_DELETE_HANDLE(hSendToThreadPoolThread);
	
	SAFE_DELETE_HANDLE(hSendToWRSemaphore);
	SAFE_DELETE_HANDLE(hThreadPoolThread);
		
	closesocket(listenSocket);

	DeleteCriticalSection(&addToQueueCS);
	DeleteCriticalSection(&getQueueDataCS);
	DeleteCriticalSection(&tasksUpdateCS);
	
	WSACleanup();

	return 0;
}

bool InitializeWindowsSockets() {
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void Decomposition(SOCKET* acceptedSockets, int indexDeleted, int indexCurrentSize) {
	for (int i = indexDeleted; i < indexCurrentSize - 1; i++) {
		acceptedSockets[i] = acceptedSockets[i + 1];
	}
}

void WriteInFile(int data, int workerRole) {
	const char* filename = "temp.txt";

	FILE *fp = fopen(filename, "a");
	if (fp == NULL) {
		printf("Error opening the file %s", filename);
		return;
	}
	
	fprintf(fp, "Worker role %d: %d\n", workerRole, data);
	fclose(fp);
}

DWORD WINAPI addToQueue(LPVOID lpParam) {
	while (true) {
		WaitForSingleObject(hAddToQueueSemaphore, INFINITE);
		EnterCriticalSection(&addToQueueCS);
			char* temp = (char*)lpParam;
			enqueue(queue, atoi(temp));
			printf("Enqueued data: %s\n", temp);
			counter++;
		LeaveCriticalSection(&addToQueueCS);

		if (counter > 3)
			ReleaseSemaphore(hSendToThreadPoolSemaphore, 1, NULL);
	}
}

DWORD WINAPI sendToThreadPool(LPVOID lpParam) {
	int tempData;
	while (true) {
		WaitForSingleObject(hSendToThreadPoolSemaphore, INFINITE);

		EnterCriticalSection(&getQueueDataCS);
			//preuzmi podatak iz reda
			tempData = dequeue(queue);
		LeaveCriticalSection(&getQueueDataCS);
		
		EnterCriticalSection(&tasksUpdateCS);
			//kreiranje novog taska
			addTask(task, tempData);
			tasksCount++;
		LeaveCriticalSection(&tasksUpdateCS);

		if (tasksCount > 3)
			ReleaseSemaphore(hSendToWRSemaphore, 1, NULL);
	}
}

DWORD WINAPI sendToWR(LPVOID lpParam) {
	while (true) {
		WaitForSingleObject(hSendToWRSemaphore, INFINITE);

		EnterCriticalSection(&tasksUpdateCS);
			//preuzimanje taska i prosledjivanje metodi writeInFile		
			WriteInFile(getTask(task), atoi((char*)lpParam));
			tasksCount--;
		LeaveCriticalSection(&tasksUpdateCS);

		if (tasksCount > 0)
			ReleaseSemaphore(hSendToWRSemaphore, 1, NULL);
	}
}