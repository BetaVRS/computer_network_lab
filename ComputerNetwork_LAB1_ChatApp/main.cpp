#include<iostream>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define SERVERIP "192.168.0.102"
int main()	//Client端
{
	//库使用前初始化，协议使用版本
	WORD wVersionRequest = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(wVersionRequest, &wsadata);

	//客户端初始化socket
	SOCKET socketClient = socket(AF_INET, SOCK_STREAM, 0);

	//服务器地址信息
	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;					//IPv4
	addrSrv.sin_port = htons(SPORT);				//服务器的监听端口
	//addrSrv.sin_addr.s_addr = inet_addr(SERVERIP);	//inet_addr()将字符串形式的ip地址转换为网络字节序
	//inet_pton(AF_INET, SERVERIP, &addrSrv.sin_addr);	//上文报错，改用新版本函数inet_pton()
	InetPton(AF_INET, SERVERIP, &addrSrv.sin_addr);

	char IPv4[16] = { 0 };
	//connect用于与服务器建立连接
	if (connect(socketClient, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) != 0) //注意connect第3个参数namelen是指传入第二个指针所指sockaddr_in大小
	{
		inet_ntop(AF_INET, &addrSrv.sin_addr.s_addr, IPv4, SPORT);
		printf("Failed to connect to %s:%d\n", IPv4, SPORT);
		printf("Error code %d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
	inet_ntop(AF_INET, &addrSrv.sin_addr.s_addr, IPv4, SPORT);
	printf("Successfully connect to %s:%d\n", IPv4, SPORT);

	int sendlen = 100, recvlen = 100;
	char sendBuf[1024] = { 0 };
	char recvBuf[1024] = { 0 };
	int iResult = 0;
	while (true)
	{
		printf("Sending message to server: ");
		scanf_s("%s", &sendBuf,1024);
		//strcpy_s(sendBuf, 1024, "Testing connection, send from client to server.");

		send(socketClient, sendBuf, sendlen, 0);
		printf("Message sent.\n");

		printf("Receiving...\n");
		iResult = recv(socketClient, recvBuf, recvlen, 0);
		
		if (iResult == 0)
		{
			printf("Connection closed\n");
			break;
		}
		else if (iResult > 0)
			printf("Receiving %d bytes from server.\n", iResult);
		else
		{
			printf("A failure for recv() in receiving message from server.\n");
			printf("Error code %d\n", WSAGetLastError());
			break;
		}
		printf("Receiving message from server: ");
		printf("%s\n", recvBuf);
		strcpy_s(recvBuf, 1024, "");
	}

	closesocket(socketClient);	//链接结束关闭socket
	WSACleanup();				//释放Socket dll资源
	system("pause");
	return 0;
}