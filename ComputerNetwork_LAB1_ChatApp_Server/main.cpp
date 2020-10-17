#include<iostream>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define LISTENIP "127.0.0.1"
int main() //Server��
{
	//��ʹ��ǰ��ʼ����Э��ʹ�ð汾
	WORD wVersionRequest = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(wVersionRequest, &wsadata);

	//�������˳�ʼ��socket
	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, 0);

	//SOCKADDR_IN������������ͨ�ŵĵ�ַ
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;				//��ַ����IPv4
	saddr.sin_port = htons(SPORT);			//htons():host to net short, ���÷������˿ڣ�ʲô�˿�
	saddr.sin_addr.s_addr = INADDR_ANY;		//0�����������IP
	//inet_pton(AF_INET, LISTENIP, &saddr);	//LISTENIP���÷���������IP��0Ϊ����

	char IPv4[33];
	//bind��һ�����ص�ַ�󶨵�ָ��socket���ɹ�����0
	int bindrtv = bind(socketServer, (sockaddr*)&saddr, sizeof(saddr));	//ע��bind�е�3������namelen��ָ����ڶ���ָ����ָsockaddr_in��С
	inet_ntop(AF_INET, &saddr, IPv4, 33);
	if (bindrtv != 0)
	{
		//printf("Failed to bind to IP: %s:%d\n", inet_ntoa(saddr.sin_addr),SPORT);	//inet_ntoa()��һ��addr_in�ṹ�������IP�ַ���
		printf("Failed to bind to IP: %s:%d\n", IPv4, SPORT);	//���ı�������inet_ntop()
		printf("Error code %d\n", WSAGetLastError());
		return 0;
	}
	printf("Successfully bind to IP: %s:%d\n", IPv4, SPORT);

	//listenʹsocket��������˿ڵ�״̬������Զ�������Ƿ���
	if (listen(socketServer, 5) != 0)	//5�����ӵȴ�������󳤶ȣ���֪����ʲôӰ��
	{
		printf("Failed to set socket to listening state.\n");
		printf("Error code %d\n", WSAGetLastError());
		return 0;
	}
	printf("Listening...\n");


	SOCKADDR_IN caddr;					//���տͻ��˵�ͨ�ŵ�ַ
	int len = sizeof(caddr);			//ͨ�ŵ�ַ�ĳ��ȣ�Ҫ�Ե�ַ��ʽ����accept
	char sendBuf[1024], recvBuf[1024];	//�����뷢��buffer
	int sendlen = 100, recvlen = 100;	//���������buffer����
	strcpy_s(sendBuf,1024,"Testing connection, send from server to client.");
	while (true)
	{
		printf("Checking connection...\n");
		SOCKET socketConn = accept(socketServer, (SOCKADDR*)&caddr, &len);	//accept��������ᴴ��һ���µ�socket�����������Ϳͻ���ͨ��
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
	closesocket(socketServer);	//���������ӽ����ر�socket
	WSACleanup();				//�ͷ�Socket dll��Դ
	return 0;
}