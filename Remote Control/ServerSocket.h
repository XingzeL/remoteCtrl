#pragma once
#include "pch.h"
#include "framework.h"

//#define _CRT_SECURE_NO_WARNINGS
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket():sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//打包的重构
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear(); //这样就不会进入for循环
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	CPacket(const BYTE* pData, size_t& nSize) {
		//用于解析数据的构造函数
		size_t i;
		for (i = 0; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2; //如果只有包头后面没有东西的情况可以进入return的分支
				break;
			}
		}
		if (i + 4 + 2 + 2> nSize) { //有的包没有数据，就是一个命令，也可以进行解析
			//没有包头，或者只有包头(数据不全)
			nSize = 0;
			return;
		}
		//说明有包头且后面有数据
		nLength = *(DWORD*)(pData + i); i += 4; //取到包的长度
		if (nLength + i > nSize) {
			//说明包没有完全接收到，就返回，解析失败
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		//进行校验
		WORD sum = 0;
		for (int j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i; //head2 length4 data...  i就是实际从缓冲区读了的size
			return;
		}
		//解析失败的情况
		nSize = 0;
		return;
	}

	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) { //这样可以连等
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() { //包数据的大小
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		//填数据
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}
public:
	WORD sHead; //包头外部需要使用， 固定为FEFF
	DWORD nLength;
	WORD sCmd; //远控命令
	std::string strData; //包数据
	WORD sSum; //和校验
	std::string strOut; //整个包的数据
};
#pragma pack(pop)


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

	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr); //fault1: 是adr的size，写成了sock的size导致accept直接返回-1
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) return false;
		//char buffer[1024];
		//recv(m_sock, buffer, sizeof(buffer), 0);
		//send(m_sock, buffer, sizeof(buffer), 0);
		//closesocket(m_sock);
		return true;
	}

#define BUFFERSIZE 4096
	int DealCommand() {
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

	bool Send(const char* pData, int nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		//获取文件列表
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
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
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}

private: 

	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
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