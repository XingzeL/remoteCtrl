#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

typedef struct file_info {
	file_info() { //����
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

	BOOL IsInvalid; //�Ƿ�����Ч���ļ��������ӣ���ݷ�ʽ
	char szFileName[256]; //����
	BOOL IsDirectory; //�Ƿ���Ŀ¼
	BOOL HasNext;
}FILEINFO, * PFILEINFO;

typedef struct MouseEvent{
	MouseEvent() {
		nAction = 0;
		nButton = -1; //����û��Ч��
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //������ƶ���˫��
	WORD nButton; //������Ҽ����м�
	POINT ptXY; //����
}MOUSEEV, *PMOUSEEV;

typedef void(*SOCK_CALLBACK)(void* arg, int status, std::list<CPacket>&, CPacket& inPacket); //����һ��callback

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

	int Run(SOCK_CALLBACK callback, void* arg, short port = 9527) { 
		/*
			������ע����callback��������ģ���������������
			callback�����ù��ܺ�����ģ����뿪����ֻ�д��ε���ϵ
		*/
		
		bool ret = InitSocket(port);
		if (ret == false) return -1; //�����뷵�ص��ϲ㣬��������д�����

		std::list<CPacket> lstPackets; //package���У������ռ�������Ҫ���͵İ�

		m_callback = callback;
		m_arg = arg;

		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets, m_packet); //lstPackets��Ҫ���͵������ռ�������ģ��
				while (lstPackets.size() > 0) { //ȫ�����ͣ�������ֻ�ܷ������ݣ���ʲô����Ҫ
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}

		return 0;

	}

protected:  //��������������������
	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr); //fault1: ��adr��size��д����sock��size����acceptֱ�ӷ���-1
		TRACE("Start to accept \r\n");
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz); 
		TRACE("End Accept\r\n");
		if (m_client == -1) return false;
		//char buffer[1024];
		//recv(m_sock, buffer, sizeof(buffer), 0);
		//send(m_sock, buffer, sizeof(buffer), 0);
		//closesocket(m_sock);
		TRACE("quit AcceptClient\r\n");
		return true;
	}

	bool InitSocket(short port = 9527) {

		//TODO: ����socket��У��
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY; //���е�IP
		serv_adr.sin_port = htons(port);
		char errMsg[256]; // �������ڴ洢������Ϣ�Ļ�����
		//Ϊʲôserver�ж��IP: �е�ʱ���Ƕ��ڹ��������������ݿ⣬���������豸�������ӣ�
		// �����IP��ַ������ķ���¶�����棬�����Ĵ������Ū�úܿ�(�ù���) ��������ɱ���
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		{
			strerror_s(errMsg, sizeof(errMsg), errno);
			fprintf(stderr, "Error in bind: %s\n", errMsg);
			return false;
		}
		if (listen(m_sock, 1) == -1) return false;


		return true;
	}

#define BUFFERSIZE 4096
	int DealCommand() {
		//TRACE("into Deal Command");
		if (m_client == -1) return false;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFERSIZE];
		//ԭ��û��delete����ԭ��buffer��ȷ���ǳ����ӻ��Ƕ�����
		TRACE("Server get command %d\r\n", m_packet.sCmd);
		if (buffer == NULL) {
			TRACE("�ڴ治�㣡\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFERSIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFERSIZE - index, 0);
			if (len <= 0) {
				delete[]buffer; //�����ӵ������ֻ����һ��cmd֮����Բ�����
				return -1;
			}

			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //������ܻ�ı�len
			if (len > 0) {
				//�����ɹ������
				memmove(buffer, buffer + len, BUFFERSIZE - len); //��ʣ��������ƶ��������ͷ��m_packet
				index -= len; //ע�⣺�����len�������len���ܲ�ͬ����Ϊ������2000�ֽڿ���1000���ֽ���һ�������Ƚ���һ��������ʣ�������Ƶ�ǰ�棬��index�仯
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1; //�������
	}

	bool Send(const char* pData, int nSize) {  //ֱ�ӽ����ݷ��ͳ�ȥ
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {  //�������ͳ�ȥ
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		//��ȡ�ļ��б�
		if (((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) || m_packet.sCmd == 9) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			//�������
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true; //�ɹ��õ�mouseEvent
		}

		return false;
	}
	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}

	}

private: 

	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	void* m_arg;  //callback�����Ͳ���
	SOCK_CALLBACK m_callback;

	//����ģʽ�������������������Ϊprivate
	//��������
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

//extern CServerSocket server; //������.cpp�ļ���