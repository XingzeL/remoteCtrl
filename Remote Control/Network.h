#pragma once
#include "USocket.h"
#include "CThread.h"

/*
 服务器核心功能：
*/
class CNetwork
{
public:
	CNetwork() {}
	~CNetwork() { }
private:
};

/*
	设置回调函数
*/
typedef int (*AcceptFunc)(void* arg, CUSOCKT& client); 
typedef int (*RecvFunc)(void* arg, const CBuffer& buffer);
typedef int (*SendFunc)(void* arg);
typedef int (*RecvFromFunc)(void* arg, const CBuffer& buffer, CUSockaddrIn& addr);
typedef int (*SendToFunc)(void* arg);

class CServerParameter {
public:
	CServerParameter(const std::string& ip, short port, PTYPE type);
	CServerParameter(
		const std::string& ip = "0.0.0.0",
		short port=9527, PTYPE type = PTYPE::UTypeTCP,
		AcceptFunc acceptf = NULL,
		RecvFunc recvf = NULL,
		SendFunc sendf = NULL,
		RecvFromFunc recvfromf = NULL,
		SendToFunc sendtof = NULL
	);
	
	/* 通过运算符重载进行参数设置 */
	//参数输入
	CServerParameter& operator<<(AcceptFunc func);
	CServerParameter& operator<<(RecvFunc func);
	CServerParameter& operator<<(SendFunc func);
	CServerParameter& operator<<(RecvFromFunc func);
	CServerParameter& operator<<(SendToFunc func);
	CServerParameter& operator<<(std::string& ip);
	CServerParameter& operator<<(short port);
	CServerParameter& operator<<(PTYPE type);

	//参数输出，传入的是引用
	CServerParameter& operator>>(AcceptFunc& func);
	CServerParameter& operator>>(RecvFunc& func);
	CServerParameter& operator>>(SendFunc& func);
	CServerParameter& operator>>(RecvFromFunc& func);
	CServerParameter& operator>>(SendToFunc& func);
	CServerParameter& operator>>(std::string& ip);
	CServerParameter& operator>>(short& port);
	CServerParameter& operator>>(PTYPE& type);

	//拷贝构造，等于号重载，用于同类型赋值
	CServerParameter(const CServerParameter& param);
	CServerParameter& operator=(const CServerParameter& param);
	
	std::string m_ip;
	short m_port;
	PTYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;
};

class CUServer : public ThreadFuncBase {
public:
	CUServer(CServerParameter& param);
	int Invoke(void* arg); 
	int Send(CUSOCKT& client, CBuffer& buffer);
	int Sendto(CUSOCKT& client, CUSockaddrIn& addr);
private:
	int threadFunc();
private:
	CServerParameter m_param;
	CThread m_thread;
};

