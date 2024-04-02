#pragma once
#include "pch.h"
#include "framework.h"

//#define _CRT_SECURE_NO_WARNINGS
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket():sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//������ع�
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear(); //�����Ͳ������forѭ��
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

	CPacket(const BYTE* pData, size_t& nSize) {
		//���ڽ������ݵĹ��캯��
		size_t i;
		for (i = 0; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2; //���ֻ�а�ͷ����û�ж�����������Խ���return�ķ�֧
				break;
			}
		}
		if (i + 4 + 2 + 2> nSize) { //�еİ�û�����ݣ�����һ�����Ҳ���Խ��н���
			//û�а�ͷ������ֻ�а�ͷ(���ݲ�ȫ)
			nSize = 0;
			return;
		}
		//˵���а�ͷ�Һ���������
		nLength = *(DWORD*)(pData + i); i += 4; //ȡ�����ĳ���
		if (nLength + i > nSize) {
			//˵����û����ȫ���յ����ͷ��أ�����ʧ��
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
		//����У��
		WORD sum = 0;
		for (int j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i; //head2 length4 data...  i����ʵ�ʴӻ��������˵�size
			return;
		}
		//����ʧ�ܵ����
		nSize = 0;
		return;
	}

	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) { //������������
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() { //�����ݵĴ�С
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		//������
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}
public:
	WORD sHead; //��ͷ�ⲿ��Ҫʹ�ã� �̶�ΪFEFF
	DWORD nLength;
	WORD sCmd; //Զ������
	std::string strData; //������
	WORD sSum; //��У��
	std::string strOut; //������������
};
#pragma pack(pop)


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

	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr); //fault1: ��adr��size��д����sock��size����acceptֱ�ӷ���-1
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) return false;
		//char buffer[1024];
		//recv(m_sock, buffer, sizeof(buffer), 0);
		//send(m_sock, buffer, sizeof(buffer), 0);
		//closesocket(m_sock);
		return true;
	}

#define BUFFERSIZE 4096
	int DealCommand() {
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

	bool Send(const char* pData, int nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_client == -1) return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		//��ȡ�ļ��б�
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
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
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}

private: 

	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
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