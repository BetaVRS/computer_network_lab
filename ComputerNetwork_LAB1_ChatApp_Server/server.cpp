#include<iostream>
#include<stdio.h>
#include<map>
#include<queue>
#include<winsock2.h>
#include<WS2tcpip.h>
#include"ServerStruct.h"
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define BUFFSIZE 4096
#define MAXONLINE 32
#define LISTENIP ""

HANDLE mutex = CreateMutex(NULL, FALSE, NULL);	//利用互斥量实现线程的同步
//以下为线程需要同步读写的数据
map<unsigned short, DWORD> onlineUser;		//在线用户，UID为key，线程ID为value
unsigned short currUID = 1;					//暂存uid
unsigned int onlineUserNum = 0;				//当前在线人数
map<unsigned short, queue<char*>*> msgTrans;	//服务器转发消息列表

bool loginAccept(SOCKET sock,unsigned short* uid);
bool messageAccept(SOCKET sock, unsigned short uid, char* buffer);
int checkOnlineList(SOCKET sock);
bool logout(SOCKET sock, u_short uid);
DWORD WINAPI threadProc(LPVOID lpPrama);

int main() //Server端
{
	//库使用前初始化，协议使用版本
	WORD wVersionRequest = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(wVersionRequest, &wsadata);

	//服务器端初始化socket
	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, 0); //SOCKET类型本身是 UINT_PTR 是指针

	//SOCKADDR_IN用来处理网络通信的地址
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;				//地址类型IPv4
	saddr.sin_port = htons(SPORT);			//htons():host to net short, 设置服务器端口？什么端口
	saddr.sin_addr.s_addr = INADDR_ANY;		//0代表接收所有IP
	//inet_pton(AF_INET, LISTENIP, &saddr);	//LISTENIP设置服务器监听IP，0为所有

	char IPv4[16]= { 0 };
	//bind将一个本地地址绑定到指定socket，成功返回0
	int bindrtv = bind(socketServer, (sockaddr*)&saddr, sizeof(saddr));	//注意bind中第3个参数namelen是指传入第二个指针所指sockaddr_in大小
	inet_ntop(AF_INET, &saddr.sin_addr.s_addr, IPv4, 16);
	if (bindrtv != 0)
	{
		//printf("Failed to bind to IP: %s:%d\n", inet_ntoa(saddr.sin_addr),SPORT);	//inet_ntoa()将一个addr_in结构体输出成IP字符串
		printf("Failed to bind to IP: %s:%d\n", IPv4, SPORT);	//上文报错，改用inet_ntop()
		printf("Error code %d\n", WSAGetLastError());
		return 0;
	}
	printf("Successfully bind to IP: %s:%d\n", IPv4, SPORT);

	//listen使socket进入监听端口的状态，监听远程连接是否到来
	if (listen(socketServer, 5) != 0)	//5是连接等待队列最大长度，不知到有什么影响
	{
		printf("Failed to set socket to listening state.\n");
		printf("Error code %d\n", WSAGetLastError());
		return 0;
	}
	printf("Listening...\n");


	SOCKADDR_IN caddr;						//接收客户端的通信地址
	int len = sizeof(caddr);				//通信地址的长度，要以地址形式传给accept

	char sendBuf[1024], recvBuf[1024];		//接收与发送buffer
	int sendlen = 100, recvlen = 100;		//发送与接收buffer长度
	//strcpy_s(sendBuf,1024,"Testing connection, send from server to client.");

	//Server主循环
	while (true)
	{
		if (onlineUserNum < MAXONLINE)	//控制最大在线人数
		{
			printf("Checking connection...\n");
			SOCKET socketConn = accept(socketServer, (SOCKADDR*)&caddr, &len);	//accept连接请求会创建一个新的socket。用来单独和客户端通信
			inet_ntop(AF_INET, &caddr.sin_addr.s_addr, IPv4, 16);
			printf("Connect to %s:%d. Initiating new thread...\n", IPv4, ntohs(caddr.sin_port));

			HANDLE hThread = CreateThread(NULL, 0, threadProc, (LPVOID)socketConn, 0, NULL); //SOCKET本身是指针 UINT_PTR -> LPVOID (void *)
			if (hThread)
			{
				//onlineUser.emplace(uid, GetThreadId(hThread));	//记录上线用户
				CloseHandle(hThread);	//关闭线程句柄，表示不再对句柄对应的线程做任何干预。并没有结束线程。
				printf("New thread created, handle closed.\n");				
			}
			else
			{
				printf("Failed to create new thread.\n");
				printf("Error code: %d\n", GetLastError());
			}
		}
		else
		{
			printf("The Number of online users has reach the limit. Stop accepting new connection.\n");
			while (onlineUserNum >= MAXONLINE)
				Sleep(1000);
			printf("The Number of online users has decreased. Restore connection.\n");
		}
	}
	closesocket(socketServer);	//服务器链接结束关闭socket
	WSACleanup();				//释放Socket dll资源
	return 0;
}

bool loginAccept(SOCKET sock, unsigned short* uid)
{
	BYTE func;
	char recvBuf[16] = { 0 };
	char sendBuf[16] = { 0 };
	printf("Waiting for login...\n");
	recv(sock, recvBuf, 16, 0);
	printf("Login info received.\n");
	func = recvBuf[2];
	if (func != FUNCODE::Login)
		return false;
	func = FUNCODE::LoginSuccess;

	WaitForSingleObject(mutex, INFINITE);	//申请加锁，无限等待
	//对共享资源的操作
	*uid = currUID;
	onlineUser.emplace(currUID, GetCurrentThreadId());
	currUID++;
	onlineUserNum++;
	ReleaseMutex(mutex);	//解锁

	//前几字节要手动复制，可能会含有空字符
	sendBuf[0] = ((char *)uid)[0];
	sendBuf[1] = ((char *)uid)[1];
	sendBuf[2] = func;
	//strncat_s(sendBuf, 16, (char*)uid, sizeof(unsigned short));
	//strncat_s(sendBuf, 16, (char*)(&func), sizeof(BYTE));
	printf("Sending back login accept...\n");
	send(sock, sendBuf, 3, 0);
	printf("Login accept sent.\n");
	return true;
}

bool messageAccept(SOCKET sock, unsigned short uid, char* buffer)
{
	char temp[2] = { buffer[3],buffer[4] };
	u_short target = *((u_short*)temp);
	printf("Accepting message from %u to %u.\n", uid, target);
	WaitForSingleObject(mutex, INFINITE);	//加锁
	auto itor = msgTrans.find(target);	
	if (itor == msgTrans.end()||msgTrans.size()==0)	//若map类型没有查找到，则返回end()迭代器
	{
		//*******************************************************************************
		printf("Currently %d user(s) online\n", msgTrans.size());
		printf("User %u not found, message abort.\n",target);
		ReleaseMutex(mutex);	//解锁
		return false;
	}
	char* msg = new char[BUFFSIZE];//strlen(buffer + 5) + 6];	//在线程消息的转发中释放资源
	for (int i = 0; i < 5; i++)	//手动复制前5个字节，可能包含空字节
		msg[i] = buffer[i];
	strcpy_s(msg + 5, strlen(buffer + 5) + 6, buffer + 5);
	itor->second->push(msg);
	printf("Message from %u to %u stored.\n", uid, target);
	ReleaseMutex(mutex);	//解锁
	return true;
}

int checkOnlineList(SOCKET sock)
{
	BYTE func = FUNCODE::OnlineList;
	char sendBuf[BUFFSIZE] = { 0 };
	sendBuf[2] = func;
	//strncat_s(sendBuf, BUFFSIZE, (char*)(&uid), sizeof(u_short));
	//strncat_s(sendBuf, BUFFSIZE, (char*)(&func), sizeof(BYTE));
	WaitForSingleObject(mutex, INFINITE);	//加锁
	printf("Checking online users' data.\n");
	u_short usernum = onlineUser.size();
	sendBuf[3] = ((char*)&usernum)[0];
	sendBuf[4] = ((char*)&usernum)[1];
	//strncat_s(sendBuf, BUFFSIZE, (char*)(&usernum), sizeof(u_short));
	u_short uid = 0;
	int i = 0;
	if(usernum>0)
		for (auto itor = onlineUser.begin(); itor != onlineUser.end(); itor++,i+=2)
		{
			uid = itor->first;
			sendBuf[i + 5] = ((char*)&uid)[0];
			sendBuf[i + 6] = ((char*)&uid)[1];
			//strncat_s(sendBuf, BUFFSIZE, (char*)(&uid), sizeof(u_short));
		}
	printf("Sending back online users' data.\n");
	send(sock, sendBuf, 5 + usernum * 2, 0);
	ReleaseMutex(mutex);	//解锁
	printf("Online users' data sent.\n");
	return 0;
}

bool logout(SOCKET sock,u_short uid)
{
	BYTE func = FUNCODE::Logout;
	char sendBuf[32] = { 0 };
	const char msg[] = "Logged out from server!";
	sendBuf[2] = func;
	//strncat_s(sendBuf, BUFFSIZE, (char*)(&uid), sizeof(u_short));
	//strncat_s(sendBuf, BUFFSIZE, (char*)(&func), sizeof(BYTE));
	strncat_s(sendBuf + 3, 29, msg, strlen(msg) + 1);
	send(sock, sendBuf, 32, 0);
	WaitForSingleObject(mutex, INFINITE);	//加锁
	onlineUser.erase(uid);
	msgTrans.erase(uid);
	onlineUserNum--;
	ReleaseMutex(mutex);	//解锁
	printf("User %u logged out from server.\n", uid);
	return true;
}

DWORD __stdcall threadProc(LPVOID lpPrama)
{
	SOCKET socketClient = (SOCKET)lpPrama;
	DWORD threadID = GetCurrentThreadId();
	printf("Thread: %u has been created.\n",threadID);
	unsigned short uid = 0;
	bool loged = false;
	queue<char *> msgQueue;	//等待发送到client的消息队列，来自其它client

	if (loginAccept(socketClient,&uid))
		loged = true;
	else
		printf("Login not accepted. Closing socket.\n");
	WaitForSingleObject(mutex, INFINITE);
	msgTrans.emplace(uid, &msgQueue);
	ReleaseMutex(mutex);
	//设置socket为非阻塞模式
	u_long is_non_block = 1;
	int ret = ioctlsocket(socketClient, FIONBIO, &is_non_block);
	if (ret != 0)
	{
		printf("Failed to set socket to non block mode.\n");
		closesocket(socketClient);
		return 0;
	}

	char recvBuf[BUFFSIZE];	//线程与Client通信的buffer
	BYTE func;	//通信协议中的功能码
	int retv = 0;
	int wsaError;
	char *msg;
	while (loged)
	{
		WaitForSingleObject(mutex, INFINITE);	//加锁
		while (!msgQueue.empty())	//消息转发给client
		{
			printf("Transferring message to client...\n");
			msg = msgQueue.front();
			msgQueue.pop();
			send(socketClient, msg, strlen(msg + 5) + 6, 0);
			printf("Message transferred.\n");
			for (int i = 0; i < 5; i++)
				msg[i] = (char)255;	
			delete msg;		//释放消息暂存的空间，造成Heap corruption???
							//在msgAcpt()中将new分配大小改为固定值BUFFSIZIE问题不再出现
							//原理不明
			printf("Memory deleted.\n");
		}
		ReleaseMutex(mutex);	//解锁

		retv = recv(socketClient, recvBuf, BUFFSIZE, 0);
		if (retv > 0)
		{
			func = recvBuf[2];
			printf("Received data form uid: %d with func: %u\n", uid, func);
		}
		else
			func = 255;
		switch(func)
		{
		case FUNCODE::Message:		messageAccept(socketClient, uid, recvBuf);break;
		case FUNCODE::OnlineList:	checkOnlineList(socketClient);break;
		case FUNCODE::Logout:		loged = !logout(socketClient,uid);break;
		default: break;
		}
	}

	while (!msgQueue.empty())	//释放new分配的未转发资源
	{
		delete msgQueue.front();
		msgQueue.pop();
	}

	closesocket(socketClient);
	printf("Socket closed!\n");
	printf("Thread: %u terminated.\n", threadID);
	return 0;
}