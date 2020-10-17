#include<iostream>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define SERVERIP "127.0.0.1"
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
	inet_pton(AF_INET, SERVERIP, &addrSrv);			//上文报错，改用新版本函数inet_pton()
	

	//connect用于与服务器建立连接
	if (connect(socketClient, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) != 0) //注意connect第3个参数namelen是指传入第二个指针所指sockaddr_in大小
	{
		printf("Failed to connect to %s:%d\n", SERVERIP, SPORT);
		printf("Error code %d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
	printf("Successfully connect to %s:%d\n", SERVERIP, SPORT);

	int sendlen = 100, recvlen = 100;
	char sendBuf[1024] = { 0 };
	char recvBuf[1024] = { 0 };
	
	printf("Sending message to server.\n");
	//scanf("%s", &sendBuf);
	strcpy_s(sendBuf, 1024, "Testing connection, send from client to server.");
	recv(socketClient, sendBuf, recvlen, 0);
	send(socketClient, recvBuf, sendlen, 0);
	printf("Message sent.\n");
	printf("%s\n", recvBuf);

	closesocket(socketClient);	//链接结束关闭socket
	WSACleanup();				//释放Socket dll资源
	system("pause");
	return 0;
}