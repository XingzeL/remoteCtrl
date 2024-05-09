#pragma once

#include "pch.h"
#include <string>
#include "framework.h"
#include <vector>
#include <list>
#include <map>
#include <mutex>

//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma pack(push)
#pragma pack(1)
#define WM_SEND_PACK (WM_USER + 1)  //发送包数据
#define WM_SEND_PACK_ACK (WM_USER+2) ///发送包数据应答
//void Dump(BYTE* pData, size_t nSize);
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

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

	CPacket(const BYTE* pData, size_t& nSize){
		//用于解析数据的构造函数
		size_t i;
		for (i = 0; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2; //如果只有包头后面没有东西的情况可以进入return的分支
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) { //有的包没有数据，就是一个命令，也可以进行解析
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

	/*
		原本Data函数是没有参数的，而是改变CPacket的成员变量，
		在进行发送的时候将数据打包到strOut并返回
	*/

	const char* Data(std::string& strOut) const { //3.加入const后缀，不会修改对象本身
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
	//std::string strOut; //整个包的数据
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

enum {
	CSM_AUTOCLOSE=1, //CSM = Client Socket Mode 自动关闭模式
};

typedef struct PacketData{ //消息中传输的Packet，有2个元素，data和mode
	std::string strData;
	UINT nMode;
	PacketData(const char* pData, size_t nLen, UINT mode) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
		}
		return *this;
	}
}PACKET_DATA;

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1; //先是没有效果
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //点击，移动，双击
	WORD nButton; //左键，右键，中键
	POINT ptXY; //坐标
}MOUSEEV, * PMOUSEEV;


//在头文件中定义并实现了它： head.h中定义并实现了，a.cpp, b.cpp包含了同一个head.h，就会出现同一个符号导致报错：
//1>RemoteClientDlg.obj : error LNK2005: "class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __cdecl GetErrorInfo(int)" (?GetErrorInfo@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@H@Z) 已经在 ClientSocket.obj 中定义
//1 > G:\Projets\remoteCtrl\x64\Debug\RemoteClient.exe : fatal error LNK1169 : 找到一个或多个多重定义的符号
std::string GetErrorInfo(int wsaErrCode);
class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		//静态函数作为外部访问对象的接口，没有this指针(因为全局的成员不需要依赖对象)，不能访问static之外的成员
		if (m_instance == NULL) {
			m_instance = new CClientSocket(); //在类中进行的new，因为没有外部的释放指令delete，导致对象不会调用析构函数——解决：CHelper类
		}
		return m_instance;
	}

	bool InitSocket();

#define BUFFERSIZE 4096000
	int DealCommand() { //接收服务器的响应，解析一个packet，返回值为响应的命令值
		if (m_sock == -1) return false;
		//char buffer[1024] = "";
		char* buffer = m_buffer.data();  //TODO:多线程发送命令时可能会出现冲突
		//原来没有delete掉的原因：buffer不确定是长连接还是短链接

		if (buffer == NULL) {
			TRACE("内存不足！\r\n");
			return -2;
		}
		/*memset(buffer, 0, BUFFERSIZE);*/ //这里如果清零的话会将m_buffer的data清掉
		static size_t index = 0; //index设为static，下次进入DealCommand的时候继续接收而不是清空
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFERSIZE - index, 0);
			if (((int)len <= 0) && ((int)index == 0)) { //10.3.3(解决文件接收bug)加入index == 0的判断：index代表缓冲区中还剩的数据是否读完；len代表本次recv读取失败
				//delete[]buffer; //短连接的情况：只处理一次cmd之后可以不用了
				TRACE("出现读包的问题！");
				return -1;
			}
			//Dump((BYTE*)buffer, index);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //这里可能会改变len,len是实际上用掉的size
			if (len > 0) {
				//解析成功，输出
				//memmove(buffer, buffer + len, BUFFERSIZE - len); //将剩余的数据移动到缓冲的头步m_packet
				memmove(buffer, buffer + len, index - len);
				index -= len; //注意：上面的len和这里的len可能不同，因为读到了2000字节可能1000个字节是一个包，先解析一个包，把剩余数据移到前面，让index变化
				//delete[]buffer;
				return m_packet.sCmd; //处理掉一个packet就返回，外面的循环会不断调用DealCommand
			}
		}
		//delete[]buffer;
		return -1; //意外情况
	}

	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,
	//	bool isAutoClose = true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClose = true);

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

	CPacket& GetPack() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET; //-1
	}

	void UpdateAddress(int nIP, int nPort) {
		if (m_nIP != nIP || m_nPort != nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

private:
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::map<HANDLE, bool> m_mapAutoClosed;
	std::list<CPacket> m_lstSend; //要发送的数据
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; //认为对方会应答一系列的包；改成list的引用，就不用手动把包插进去
	//如果用vector，数量不稳定时候开销很大：随着空间的增大，push_back消耗的时间指数增加
	
	//M层：加入IP和Port的信息
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer; //可以直接用名字取地址
	SOCKET m_sock;
	CPacket m_packet;
	//单例模式：将构造和析构函数作为private
	//拷贝构造
	CClientSocket(const CClientSocket& ss);


	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket();
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
		//析构函数的好处：对象是全局的时候，只在main结束后调用，也不会影响任何行动
		//多人开发的时候：如果在main中释放，可能别的人在不知情下在后面写了要用到sock的代码，导致错误
		//只用一次的操作，可以用定义全局的方式
		//但是当在main函数中如果有人用CServerSocket声明了一个局部变量，初始化又会被调用一次——解决： 单例模式
	}

	static unsigned __stdcall threadEntry(void* arg); //传给带线程号的beginthreadex的话就需要函数为unsigned __stdcall
	void threadFunc();
	void threadFunc2(); //线程接收消息，线程中调用注册好的回调函数

	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		} //版本的初始化 
		return TRUE;
	}

	static void releaseInstance() { //用于让helper调用的释放socket的函数
		if (m_instance != NULL) {
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			TRACE("Socket has been released!\r\n");
			delete tmp; //显示释放堆内存
		}
	}

	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	/*
	  原本Send传入的不是const pack，在进行网络的send的时候会用Data()改变pack中的内容
	  在C层重构中的发送接口传入的是const(为了低耦合性), 这个改变pack自身的函数需要修改：
	  从调用原版Data()改变pack的strOut成员改成一个局部变量作为参数传给新的Data(string&)
	  这样改变的就是这个参数而不是pack，满足了C层的函数要求
	 */
	bool Send(const CPacket& pack) { //1.将参数改变成const
		if (m_sock == -1) return false;
		std::string strOut;
		pack.Data(strOut); //2.const参数只能调用有const后缀的函数,不会改变pack，Data函数会改变strOut
		return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
	}

	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);

	static CClientSocket* m_instance; //设置成静态成员，供静态函数访问
	//CServerSocket* m_instance;
	class CHelper {
		//helper访问socket的成员，helper被释放的时候会连带delete单例对象
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};