#include "ClientStruct.h"
#include<iostream>
#include<conio.h>
using std::cout;
using std::endl;
bool get_line(char * buffer, int len)
{
	for (int i = 0; i < len; i++)
	{
		buffer[i] = _getch();
		switch (buffer[i])
		{
		case '\r':
			buffer[i] = '\0';
			cout << endl;
			return true;
		case 0X1B:
			buffer[i] = '\0';
			cout << endl;
			return false;
		case '\b':
			if (i > 0)
			{
				i--;
				buffer[i] = '\0';
				cout << "\b \b";
				i--;
			}
			else
				i--;
			break;
		default:
			if (i == len - 1)
			{
				buffer[i - 1] = buffer[i];
				cout << "\b \b";
				i--;
			}
			cout << buffer[i];
			break;
		}
	}
	buffer[len - 1] = '\0';
	return true;
}
char get_cmd()
{
	char cmd[2] = { '\0','\0' };
	for (int i = 0; i < 2; i++)
	{
		cmd[i] = _getch();
		if (cmd[i] == 0x1b)
			return 0x1b;

		if (cmd[i] == '\b')
			if (i)
			{
				cmd[--i] = '\0';
				cout << "\b \b";
			}
			else
				cmd[i] = '\0';

		if (cmd[i] == '\r')
			if (i)
				break;
			else
				cmd[i] = '\0';

		if (cmd[i])
			if (i)
			{
				cmd[0] = cmd[1];
				cout << '\b' << cmd[0];
				i--;
			}
			else
				cout << cmd[0];
		else
			i--;
	}
	cout << endl;
	return cmd[0];
}

bool get_YN()
{
	char cmd;
	while (true)
	{
		cout << "Command (Y/N):";
		cmd = get_cmd();
		switch (cmd)
		{
		case 0x1b: cout << endl;
		case 'n':
		case 'N':
			return false;
		case 'y':
		case 'Y':
			return true;
		default:
			cout << "No such option!" << endl;
		}
	}
	return false;
}

bool chk_exit(char * const str, const int num)
{
	for (int i = 0; i < num; i++)
	{
		str[i] = _getch();
		if (str[i] == '\r')
		{
			str[i] = '\0';
			cout << endl;
			break;
		}
		if (str[i] == char(0x1B))
			return true;
		if (str[i] == '\b')
			if (i > 0)
			{
				i--;
				str[i] = '\0';
				cout << "\b \b";
				i--;
			}
			else
				i--;
		else
			cout << str[i];
	}
	return false;
}
