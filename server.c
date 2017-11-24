#define _CRT_SECURE_NO_WARNINGS
#define BUF_SIZE 1024*10
#define USER_NUM 5000
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <Windows.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

unsigned WINAPI thread_func(void* para);
int check_userid(char* user_id);
void make_userlist(int user_idx);
void error_handle(char* message);
void broadcast(char* msg, int msg_len);

WSADATA wsa_data;
SOCKET serv_soc, clint_soc[USER_NUM];
SOCKADDR_IN serv_addr, clint_addr;

HANDLE hThread;
UINT threadID[USER_NUM];

int len_clint_addr, user_num;	// 클라이언트 수
char user_list[USER_NUM][50];
byte user_flag[USER_NUM];

int main(void)
{
	int port_num;

	printf("Enter Port Number >>");
	scanf("%d", &port_num);

	// 소켓 라이브러리 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		error_handle("WSAStartup() error!");

	// 소켓 생성
	serv_soc = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_soc == INVALID_SOCKET)
		error_handle("socket() error!");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port_num);

	// 바인드
	if (bind(serv_soc, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		error_handle("bind() error!");

	if (listen(serv_soc, USER_NUM) == SOCKET_ERROR)
		error_handle("listen() error!");

	while (1)                                                                             
	{
		len_clint_addr = sizeof(clint_addr);

		clint_soc[user_num] = accept(serv_soc, (SOCKADDR*)&clint_addr, &len_clint_addr);
		if (clint_soc[user_num] == INVALID_SOCKET)
			error_handle("accept() error!");

		// 쓰레드 생성
		hThread = (HANDLE)_beginthreadex(NULL, 0, thread_func, (void*)user_num, 0, &threadID[user_num]);
		if (0 == hThread)
			error_handle("_beginthreadex() error!");

		user_num = (user_num + 1) % USER_NUM;
	}

	closesocket(serv_soc);
	WSACleanup();
	return 0;
}

void error_handle(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


int check_userid(char* user_id)
{
	for (int i = 0; i < user_num; i++)
		if ((user_flag[i] == TRUE) && !strcmp(user_list[i], user_id))
			return i;
	return -1;
}

void broadcast(char* msg, int msg_len)
{
	for (int i = 0; i < user_num; i++)
		if (user_flag[i] == TRUE)
			send(clint_soc[i], msg, msg_len, 0);
}


void make_userlist(int user_idx)
{
	char list[BUF_SIZE] = "\nUser List ::";

	for (int i = 0; i < user_num; i++)
		if (user_flag[i] == TRUE)
		{
			strcat(list, "\n ");
			strcat(list, user_list[i]);
		}

	send(clint_soc[user_idx], list, sizeof(list), 0);
}


UINT WINAPI thread_func(void* para)
{
	int cur_idx = (int)para;
	char user_id[100];
	char msg[BUF_SIZE];
	int msg_len;

	// 클라이언트로부터 USER_ID를 받음
	msg_len = recv(clint_soc[cur_idx], user_id, sizeof(user_id) - 1, 0);
	user_flag[cur_idx] = TRUE;
	strcpy(user_list[cur_idx], user_id);

	// 클라이언트의 접속을 알림
	
	msg_len = sprintf(msg, "%s login..", user_id);
	broadcast(msg, msg_len + 1);
	memset(msg, 0, sizeof(msg));

	while (1)
	{
		msg_len = recv(clint_soc[cur_idx], msg, sizeof(msg) - 1, 0);
		if (msg_len > 1)
		{
			char* comm = strtok(msg, " ");

			if (!strcmp(comm, "/who"))
			{
				make_userlist(cur_idx);
			}

			else if (!strcmp(comm, "/m"))
			{
				int len_tmsg = 0;
				char* dest_id = strtok(NULL, " ");
				char tmsg[BUF_SIZE], *temp; 
				sprintf(tmsg, "[%s]", user_id);
				while (temp = strtok(NULL, " ")) strcat(tmsg, temp);
				while (tmsg[len_tmsg++] != 0);
				int dest_idx = check_userid(dest_id);
				if (dest_idx != -1)
					send(clint_soc[dest_idx], tmsg, len_tmsg, 0);
				else
				{
					msg_len = sprintf(msg, "%s dose not exist.", dest_id);
					send(clint_soc[cur_idx], msg, msg_len+1, 0);
				}

			}

			else if (!strcmp(comm, "/b"))
			{
				int len_tmsg = 0;
				char tmsg[BUF_SIZE] = "[broadcast]", *temp;
				while (temp = strtok(NULL, " ")) strcat(tmsg, temp);
				while (tmsg[len_tmsg++] != 0);
				broadcast(tmsg, len_tmsg);
			}

			else if (!strcmp(comm, "/q"))
			{
				char buf[BUF_SIZE];
				msg_len = sprintf(buf, "%s logout..", user_id);
				broadcast(buf, msg_len + 1);
				break;
			}
		}
	}
	user_flag[cur_idx] = FALSE;
	closesocket(clint_soc[cur_idx]);

	return 0;
}


