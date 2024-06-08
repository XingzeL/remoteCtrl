#pragma once
#include "USocket.h"
#include "CThread.h"

/*
 ���������Ĺ��ܣ�
*/
class CNetwork
{
public:
	CNetwork() {}
	~CNetwork() { }
private:
};

/*
	���ûص�����
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
	
	/* ͨ����������ؽ��в������� */
	//��������
	CServerParameter& operator<<(AcceptFunc func);
	CServerParameter& operator<<(RecvFunc func);
	CServerParameter& operator<<(SendFunc func);
	CServerParameter& operator<<(RecvFromFunc func);
	CServerParameter& operator<<(SendToFunc func);
	CServerParameter& operator<<(std::string& ip);
	CServerParameter& operator<<(short port);
	CServerParameter& operator<<(PTYPE type);

	//��������������������
	CServerParameter& operator>>(AcceptFunc& func);
	CServerParameter& operator>>(RecvFunc& func);
	CServerParameter& operator>>(SendFunc& func);
	CServerParameter& operator>>(RecvFromFunc& func);
	CServerParameter& operator>>(SendToFunc& func);
	CServerParameter& operator>>(std::string& ip);
	CServerParameter& operator>>(short& port);
	CServerParameter& operator>>(PTYPE& type);

	//�������죬���ں����أ�����ͬ���͸�ֵ
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

