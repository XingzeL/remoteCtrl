#include "pch.h"
#include "ClientSocket.h"

//CServerSocket server; //声明的全局变量,会在主函数之前进行初始化 
//这样的好处是在main函数之前是单线程，不会有竞争的风险产生

CClientSocket* CClientSocket::m_instance = NULL; //置空，全局成员需要显示初始化
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance(); //也是全局的指针，调用了共有的访问方法，让单例对象被初始化并赋给指针

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);

	return ret;
}