#pragma once
#include "pch.h"
#include "framework.h"


class CServerSocket
{
public:
	static CServerSocket* getInstance() { 
		//静态函数作为外部访问对象的接口，没有this指针(因为全局的成员不需要依赖对象)，不能访问static之外的成员
		if (m_instance == NULL) {
			m_instance = new CServerSocket(); //在类中进行的new，因为没有外部的释放指令delete，导致对象不会调用析构函数——解决：CHelper类
		}
		return m_instance;
	}

	bool InitSocket() {
		
		//TODO: 进行socket的校验
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY; //所有的IP
		serv_adr.sin_port = htons(9527);
		//为什么server有多个IP: 有的时候是对内工作，比如是数据库，由内网的设备进行连接；
		// 对外的IP地址将对外的服务暴露给外面，内网的带宽可以弄得很宽(用光纤) 外网带宽成本高
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) return false; //注意serv_adr的转换要取地址 TODO:校验
		if (listen(m_sock, 1) == -1) return false;
		return true;
	}

	bool AcceptClient() {
		sockaddr_in client_adr;
		int cli_sz = sizeof(m_sock);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) return false;
		char buffer[1024];
		//recv(m_sock, buffer, sizeof(buffer), 0);
		//send(m_sock, buffer, sizeof(buffer), 0);
		//closesocket(m_sock);
		return true;
	}

	int DealCommand() {
		if (m_client == -1) return false;
		char buffer[1024] = "";
		while (true) {
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			//TODO: 处理客户端的commands
		}
	}

	bool Send(const char* pData, int nSize) {
		return send(m_client, pData, nSize, 0) > 0;
	}


private: 

	SOCKET m_sock;
	SOCKET m_client;
	//单例模式：将构造和析构函数作为private
	//拷贝构造
	CServerSocket(const CServerSocket&) {}
	CServerSocket& operator=(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() 
	{
		//m_sock = -1;
		m_client = INVALID_SOCKET; //-1

		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0); //结束进程
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0); //选择协议族：IPV4， stream：TCP协议
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
		//析构函数的好处：对象是全局的时候，只在main结束后调用，也不会影响任何行动
		//多人开发的时候：如果在main中释放，可能别的人在不知情下在后面写了要用到sock的代码，导致错误
		//只用一次的操作，可以用定义全局的方式
		//但是当在main函数中如果有人用CServerSocket声明了一个局部变量，初始化又会被调用一次——解决： 单例模式
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		} //版本的初始化 
		return TRUE;
	}

	static void releaseInstance() { //用于让helper调用的释放socket的函数
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp; //显示释放堆内存
		}
	}

	static CServerSocket* m_instance; //设置成静态成员，供静态函数访问
	//CServerSocket* m_instance;
	class CHelper {
		//helper访问socket的成员，helper被释放的时候会连带delete单例对象
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

extern CServerSocket server; //定义在.cpp文件中