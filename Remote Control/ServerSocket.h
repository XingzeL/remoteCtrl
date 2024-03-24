#pragma once
#include "pch.h"
#include "framework.h"


class CServerSocket
{
public:
	CServerSocket() 
	{
		
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0); //结束进程
		}
	}
	~CServerSocket() {
		WSACleanup();
		//析构函数的好处：对象是全局的时候，只在main结束后调用，也不会影响任何行动
		//多人开发的时候：如果在main中释放，可能别的人在不知情下在后面写了要用到sock的代码，导致错误
		//只用一次的操作，可以用定义全局的方式
		//但是当在main函数中如果有人用CServerSocket声明了一个局部变量，初始化又会被调用一次
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		} //版本的初始化 
		return TRUE;
	}
};

extern CServerSocket server; //定义在.cpp文件中