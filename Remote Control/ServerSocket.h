#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

typedef struct file_info {
	file_info() { //构造
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

	BOOL IsInvalid; //是否是无效的文件比如链接，快捷方式
	char szFileName[256]; //名字
	BOOL IsDirectory; //是否是目录
	BOOL HasNext;
}FILEINFO, * PFILEINFO;

typedef struct MouseEvent{
	MouseEvent() {
		nAction = 0;
		nButton = -1; //先是没有效果
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //点击，移动，双击
	WORD nButton; //左键，右键，中键
	POINT ptXY; //坐标
}MOUSEEV, *PMOUSEEV;

typedef void(*SOCK_CALLBACK)(void* arg, int status, std::list<CPacket>&, CPacket& inPacket); //定义一个callback

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

	int Run(SOCK_CALLBACK callback, void* arg, short port = 9527) { 
		/*
			主函数注册了callback，在网络模块中运行这个函数
			callback函数让功能和网络模块分离开来，只有传参的联系
		*/
		
		bool ret = InitSocket(port);
		if (ret == false) return -1; //错误码返回到上层，让外面进行错误处理

		std::list<CPacket> lstPackets; //package队列，用于收集处理函数要发送的包

		m_callback = callback;
		m_arg = arg;

		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets, m_packet); //lstPackets将要传送的数据收集到网络模块
				while (lstPackets.size() > 0) { //全部发送，网络类只管发送数据，是什么不重要
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}

		return 0;

	}

protected:  //除了启动，都保护起来
	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr); //fault1: 是adr的size，写成了sock的size导致accept直接返回-1
		TRACE("Start to accept \r\n");
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz); //会卡在这里
		TRACE("End Accept\r\n");
		if (m_client == -1) return false;
		//char buffer[1024];
		//recv(m_sock, buffer, sizeof(buffer), 0);
		//send(m_sock, buffer, sizeof(buffer), 0);
		//closesocket(m_sock);
		TRACE("quit AcceptClient\r\n");
		return true;
	}

	bool InitSocket(short port = 9527) {

		//TODO: 进行socket的校验
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY; //所有的IP
		serv_adr.sin_port = htons(port);
		char errMsg[256]; // 定义用于存储错误信息的缓冲区
		//为什么server有多个IP: 有的时候是对内工作，比如是数据库，由内网的设备进行连接；
		// 对外的IP地址将对外的服务暴露给外面，内网的带宽可以弄得很宽(用光纤) 外网带宽成本高
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		{
			strerror_s(errMsg, sizeof(errMsg), errno);
			fprintf(stderr, "Error in bind: %s\n", errMsg);
			return false;
		}
		if (listen(m_sock, 1) == -1) return false;


		return true;
	}

#define BUFFERSIZE 4096
	int DealCommand() {
		//TRACE("into Deal Command");
		if (m_client == -1) return false;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFERSIZE];
		//原来没有delete掉的原因：buffer不确定是长连接还是短链接
		TRACE("Server get command %d\r\n", m_packet.sCmd);
		if (buffer == NULL) {
			TRACE("内存不足！\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFERSIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFERSIZE - index, 0);
			if (len <= 0) {
				delete[]buffer; //短连接的情况：只处理一次cmd之后可以不用了
				return -1;
			}

			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //这里可能会改变len
			if (len > 0) {
				//解析成功，输出
				memmove(buffer, buffer + len, BUFFERSIZE - len); //将剩余的数据移动到缓冲的头步m_packet
				index -= len; //注意：上面的len和这里的len可能不同，因为读到了2000字节可能1000个字节是一个包，先解析一个包，把剩余数据移到前面，让index变化
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1; //意外情况
	}

	bool Send(const char* pData, int nSize) {  //直接将数据发送出去
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {  //将包发送出去
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		//获取文件列表
		if (((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) || m_packet.sCmd == 9) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			//限制情况
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true; //成功拿到mouseEvent
		}

		return false;
	}
	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}

	}

private: 

	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	void* m_arg;  //callback函数和参数
	SOCK_CALLBACK m_callback;

	//单例模式：将构造和析构函数作为private
	//拷贝构造
	CServerSocket(const CServerSocket& ss) {}
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

//extern CServerSocket server; //定义在.cpp文件中