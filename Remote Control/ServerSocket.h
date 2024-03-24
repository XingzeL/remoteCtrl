#pragma once
#include "pch.h"
#include "framework.h"


class CServerSocket
{
public:
	static CServerSocket* getInstance() { 
		//��̬������Ϊ�ⲿ���ʶ���Ľӿڣ�û��thisָ��(��Ϊȫ�ֵĳ�Ա����Ҫ��������)�����ܷ���static֮��ĳ�Ա
		if (m_instance == NULL) {
			m_instance = new CServerSocket(); //�����н��е�new����Ϊû���ⲿ���ͷ�ָ��delete�����¶��󲻻���������������������CHelper��
		}
		return m_instance;
	}

	bool InitSocket() {
		
		//TODO: ����socket��У��
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY; //���е�IP
		serv_adr.sin_port = htons(9527);
		//Ϊʲôserver�ж��IP: �е�ʱ���Ƕ��ڹ��������������ݿ⣬���������豸�������ӣ�
		// �����IP��ַ������ķ���¶�����棬�����Ĵ������Ū�úܿ�(�ù���) ��������ɱ���
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) return false; //ע��serv_adr��ת��Ҫȡ��ַ TODO:У��
		if (listen(m_sock, 1) == -1) return false;
		return true;
	}

	bool AcceptClient() {
		sockaddr_in client_adr;
		int cli_sz = sizeof(m_sock);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) return false;
		char buffer[1024];
		//recv(m_sock, buffer, sizeof(buffer), 0);
		//send(m_sock, buffer, sizeof(buffer), 0);
		//closesocket(m_sock);
		return true;
	}

	int DealCommand() {
		if (m_client == -1) return false;
		char buffer[1024] = "";
		while (true) {
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			//TODO: ����ͻ��˵�commands
		}
	}

	bool Send(const char* pData, int nSize) {
		return send(m_client, pData, nSize, 0) > 0;
	}


private: 

	SOCKET m_sock;
	SOCKET m_client;
	//����ģʽ�������������������Ϊprivate
	//��������
	CServerSocket(const CServerSocket&) {}
	CServerSocket& operator=(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() 
	{
		//m_sock = -1;
		m_client = INVALID_SOCKET; //-1

		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0); //��������
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0); //ѡ��Э���壺IPV4�� stream��TCPЭ��
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
		//���������ĺô���������ȫ�ֵ�ʱ��ֻ��main��������ã�Ҳ����Ӱ���κ��ж�
		//���˿�����ʱ�������main���ͷţ����ܱ�����ڲ�֪�����ں���д��Ҫ�õ�sock�Ĵ��룬���´���
		//ֻ��һ�εĲ����������ö���ȫ�ֵķ�ʽ
		//���ǵ���main���������������CServerSocket������һ���ֲ���������ʼ���ֻᱻ����һ�Ρ�������� ����ģʽ
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		} //�汾�ĳ�ʼ�� 
		return TRUE;
	}

	static void releaseInstance() { //������helper���õ��ͷ�socket�ĺ���
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp; //��ʾ�ͷŶ��ڴ�
		}
	}

	static CServerSocket* m_instance; //���óɾ�̬��Ա������̬��������
	//CServerSocket* m_instance;
	class CHelper {
		//helper����socket�ĳ�Ա��helper���ͷŵ�ʱ�������delete��������
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

extern CServerSocket server; //������.cpp�ļ���