#include "pch.h"
#include "ClientSocket.h"

//CServerSocket server; //������ȫ�ֱ���,����������֮ǰ���г�ʼ�� 
//�����ĺô�����main����֮ǰ�ǵ��̣߳������о����ķ��ղ���

CClientSocket* CClientSocket::m_instance = NULL; //�ÿգ�ȫ�ֳ�Ա��Ҫ��ʾ��ʼ��
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance(); //Ҳ��ȫ�ֵ�ָ�룬�����˹��еķ��ʷ������õ������󱻳�ʼ��������ָ��

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