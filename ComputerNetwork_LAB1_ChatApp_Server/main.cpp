#include<iostream>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define LISTENIP "127.0.0.1"
int main() //Server端
{
	//库使用前初始化，协议使用版本
	WORD wVersionRequest = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(wVersionRequest, &wsadata);

	//服务器端初始化socket
	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, 0);

	//SOCKADDR_IN用来处理网络通信的地址
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;				//地址类型IPv4
	saddr.sin_port = htons(SPORT);			//htons():host to net short, 设置服务器端口？什么端口
	saddr.sin_addr.s_addr = INADDR_ANY;		//0代表接收所有IP
	//inet_pton(AF_INET, LISTENIP, &saddr);	//LISTENIP设置服务器监听IP，0为所有

	char IPv4[33];
	//bind将一个本地地址绑定到指定socket，成功返回0
	int bindrtv = bind(socketServer, (sockaddr*)&saddr, sizeof(saddr));	//注意bind中第3个参数namelen是指传入第二个指针所指sockaddr_in大小
	inet_ntop(AF_INET, &saddr, IPv4, 33);
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


	SOCKADDR_IN caddr;					//接收客户端的通信地址
	int len = sizeof(caddr);			//通信地址的长度，要以地址形式传给accept
	char sendBuf[1024], recvBuf[1024];	//接收与发送buffer
	int sendlen = 100, recvlen = 100;	//发送与接收buffer长度
	strcpy_s(sendBuf,1024,"Testing connection, send from server to client.");
	while (true)
	{
		printf("Checking connection...\n");
		SOCKET socketConn = accept(socketServer, (SOCKADDR*)&caddr, &len);	//accept连接请求会创建一个新的socket。用来单独和客户端通信
		send(socketConn,sendBuf,sendlen,0);
		recv(socketConn,recvBuf,recvlen,0);
		
		if (strcmp("shutdown", recvBuf) == 0)
		{
			printf("Server shutdown!\n");
			break;
		}
		if (!recvBuf[0])
		{
			printf("Received message: %s\n", recvBuf);
			system("pause");
		}
		strcpy_s(recvBuf, 1024, "");

		closesocket(socketConn);
		printf("Connection closed!\n");
	}
	closesocket(socketServer);	//服务器链接结束关闭socket
	WSACleanup();				//释放Socket dll资源
	return 0;
}