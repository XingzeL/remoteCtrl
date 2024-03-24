#pragma once
#include "pch.h"
#include "framework.h"


class CServerSocket
{
public:
	CServerSocket() 
	{
		
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0); //��������
		}
	}
	~CServerSocket() {
		WSACleanup();
		//���������ĺô���������ȫ�ֵ�ʱ��ֻ��main��������ã�Ҳ����Ӱ���κ��ж�
		//���˿�����ʱ�������main���ͷţ����ܱ�����ڲ�֪�����ں���д��Ҫ�õ�sock�Ĵ��룬���´���
		//ֻ��һ�εĲ����������ö���ȫ�ֵķ�ʽ
		//���ǵ���main���������������CServerSocket������һ���ֲ���������ʼ���ֻᱻ����һ��
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		} //�汾�ĳ�ʼ�� 
		return TRUE;
	}
};

extern CServerSocket server; //������.cpp�ļ���