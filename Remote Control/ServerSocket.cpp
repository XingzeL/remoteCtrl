#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server; //声明的全局变量,会在主函数之前进行初始化 
//这样的好处是在main函数之前是单线程，不会有竞争的风险产生

CServerSocket* CServerSocket::m_instance = NULL; //置空，全局成员需要显示初始化
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance(); //也是全局的指针，调用了共有的访问方法，让单例对象被初始化并赋给指针





