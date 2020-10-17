#include<iostream>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define SERVERIP "127.0.0.1"
int main()	//Client��
{
	//��ʹ��ǰ��ʼ����Э��ʹ�ð汾
	WORD wVersionRequest = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(wVersionRequest, &wsadata);

	//�ͻ��˳�ʼ��socket
	SOCKET socketClient = socket(AF_INET, SOCK_STREAM, 0);

	//��������ַ��Ϣ
	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;					//IPv4
	addrSrv.sin_port = htons(SPORT);				//�������ļ����˿�
	//addrSrv.sin_addr.s_addr = inet_addr(SERVERIP);	//inet_addr()���ַ�����ʽ��ip��ַת��Ϊ�����ֽ���
	inet_pton(AF_INET, SERVERIP, &addrSrv);			//���ı��������°汾����inet_pton()
	

	//connect�������������������
	if (connect(socketClient, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) != 0) //ע��connect��3������namelen��ָ����ڶ���ָ����ָsockaddr_in��С
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

	closesocket(socketClient);	//���ӽ����ر�socket
	WSACleanup();				//�ͷ�Socket dll��Դ
	system("pause");
	return 0;
}