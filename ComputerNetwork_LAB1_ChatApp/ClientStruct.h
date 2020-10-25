#pragma once
enum FUNCODE
{
	Login, LoginSuccess, LoginFailed,
	Logout,
	Message, MessageToClient, MessageToServer,
	OnlineList
};

bool get_line(char *buffer, int len);
char get_cmd();
bool get_YN();
bool chk_exit(char * const str, const int num);