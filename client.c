#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define BUF_SIZE 1024*10
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <Windows.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

void error_handle(char* message);

HANDLE hThread;
UINT threadID;

WSADATA wsa_data;
SOCKET sock;
SOCKADDR_IN serv_addr;

UINT WINAPI thread_func(void* para);
int rec_on;

int main(void)
{
	char msg[BUF_SIZE];
	char c;
	char serv_ip[100];
	int port_num;
	char user_id[100];

	printf("Enter Server Ip Addr>>");
	scanf("%s", serv_ip);
	
	printf("Enter Server Port Number>>");
	scanf(" %d", &port_num);

	printf("Enter Your ID>>");
	scanf("%s", user_id);
	while ((c = getchar()) != '\n' && c != EOF);	// 입력버퍼 지우기

	// 소켓 라이브러리 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		error_handle("WSAStartup() errer!");


	// 소켓 생성
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		error_handle("socket() error!");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	serv_addr.sin_port = htons(port_num);

	// CONNECT
	if (connect(sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		error_handle("connect() error!");

	// 메시지 수신을 위한 쓰레드 호출
	rec_on = TRUE;
	hThread = (HANDLE)_beginthreadex(NULL, 0, thread_func, (void*)&rec_on, 0, &threadID);
	if (0 == hThread)
		error_handle("_beginthreadex() error!");

	// USER_ID를 서버로 전송
	send(sock, user_id, sizeof(user_id), 0);

	while (1)
	{
		int idx = 0;
		char c = 0;
		
		Sleep(500);
		printf(">>");

		while ((c = getchar()) != '\n') msg[idx++] = c;
		msg[idx] = 0;
		if (idx == 0) continue;

		send(sock, msg, sizeof(msg), 0);

		if (!strcmp(msg, "/q"))
		{
			break;
		}
	}

	rec_on = FALSE;
	closesocket(sock);
	WSACleanup();
	return 0;
}

void error_handle(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


UINT WINAPI thread_func(void* para)
{
	int* rec_on = (int*)para;
	char rec_msg[BUF_SIZE];

	while (*rec_on)
	{
		int rec_msg_len = recv(sock, rec_msg, sizeof(rec_msg) - 1, 0);
		if (rec_msg_len != -1)
		{
			puts("");
			puts(rec_msg);
		}
		
	}
	return 0;
}