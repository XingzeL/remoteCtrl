#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server; //������ȫ�ֱ���,����������֮ǰ���г�ʼ�� 
//�����ĺô�����main����֮ǰ�ǵ��̣߳������о����ķ��ղ���

CServerSocket* CServerSocket::m_instance = NULL; //�ÿգ�ȫ�ֳ�Ա��Ҫ��ʾ��ʼ��
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance(); //Ҳ��ȫ�ֵ�ָ�룬�����˹��еķ��ʷ������õ������󱻳�ʼ��������ָ��





