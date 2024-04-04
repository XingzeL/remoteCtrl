#pragma once

#include "pch.h"
#include <string>
#include "framework.h"
#include <vector>

//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma pack(push)
#pragma pack(1)

void Dump(BYTE* pData, size_t nSize);
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

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
		if (i + 4 + 2 + 2 > nSize) { //�еİ�û�����ݣ�����һ�����Ҳ���Խ��н���
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

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1; //����û��Ч��
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; //������ƶ���˫��
	WORD nButton; //������Ҽ����м�
	POINT ptXY; //����
}MOUSEEV, * PMOUSEEV;


//��ͷ�ļ��ж��岢ʵ�������� head.h�ж��岢ʵ���ˣ�a.cpp, b.cpp������ͬһ��head.h���ͻ����ͬһ�����ŵ��±���
//1>RemoteClientDlg.obj : error LNK2005: "class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __cdecl GetErrorInfo(int)" (?GetErrorInfo@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@H@Z) �Ѿ��� ClientSocket.obj �ж���
//1 > G:\Projets\remoteCtrl\x64\Debug\RemoteClient.exe : fatal error LNK1169 : �ҵ�һ���������ض���ķ���
std::string GetErrorInfo(int wsaErrCode);
class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		//��̬������Ϊ�ⲿ���ʶ���Ľӿڣ�û��thisָ��(��Ϊȫ�ֵĳ�Ա����Ҫ��������)�����ܷ���static֮��ĳ�Ա
		if (m_instance == NULL) {
			m_instance = new CClientSocket(); //�����н��е�new����Ϊû���ⲿ���ͷ�ָ��delete�����¶��󲻻���������������������CHelper��
		}
		return m_instance;
	}

	bool InitSocket(int nIp, int port) {
		//�޸ĵ�bug����Ϊÿ������Ϣ����ر����ӣ�client������serverһ��socketֻ��ʼ��һ��
		// ÿ��Inite��Ҫ��ʼ��m_sock
		
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0); //ѡ��Э���壺IPV4�� stream��TCPЭ��
		//TODO: ����socket��У��
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		//serv_adr.sin_addr.s_addr = inet_addr("127.0.0.1");
		//TRACE("addr: %08X nIP %08X\r\n", serv_adr.sin_addr.s_addr, nIp);
		serv_adr.sin_addr.s_addr = htonl(nIp);
		serv_adr.sin_port = htons(port);
		//Ϊʲôserver�ж��IP: �е�ʱ���Ƕ��ڹ��������������ݿ⣬���������豸�������ӣ�
		// �����IP��ַ������ķ���¶�����棬�����Ĵ������Ū�úܿ�(�ù���) ��������ɱ���
		
		if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			//LPCTSTR msg = "IP address not exist!";
			AfxMessageBox(_T("������IP��ַ��"));
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1) {
			AfxMessageBox(_T("����ʧ��"));
			TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
		}
		//else {
		//	AfxMessageBox(_T("���ӳɹ�"));

		//}
		return true;
	}

#define BUFFERSIZE 4096
	int DealCommand() { //���շ���������Ӧ������һ��packet������ֵΪ��Ӧ������ֵ
		if (m_sock == -1) return false;
		//char buffer[1024] = "";
		char* buffer = m_buffer.data(); 
		//ԭ��û��delete����ԭ��buffer��ȷ���ǳ����ӻ��Ƕ�����

		if (buffer == NULL) {
			TRACE("�ڴ治�㣡\r\n");
			return -2;
		}
		/*memset(buffer, 0, BUFFERSIZE);*/ //�����������Ļ��Ὣm_buffer��data���
		static size_t index = 0; //index��Ϊstatic���´ν���DealCommand��ʱ��������ն��������
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFERSIZE - index, 0);
			if ((len <= 0) && (index == 0)) { //10.3.3(����ļ�����bug)����index == 0���жϣ�index���������л�ʣ�������Ƿ���ꣻlen������recv��ȡʧ��
				//delete[]buffer; //�����ӵ������ֻ����һ��cmd֮����Բ�����
				TRACE("���ֶ��������⣡");
				return -1;
			}
			Dump((BYTE*)buffer, index);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //������ܻ�ı�len,len��ʵ�����õ���size
			if (len > 0) {
				//�����ɹ������
				//memmove(buffer, buffer + len, BUFFERSIZE - len); //��ʣ��������ƶ��������ͷ��m_packet
				memmove(buffer, buffer + len, index - len);
				index -= len; //ע�⣺�����len�������len���ܲ�ͬ����Ϊ������2000�ֽڿ���1000���ֽ���һ�������Ƚ���һ��������ʣ�������Ƶ�ǰ�棬��index�仯
				//delete[]buffer;
				return m_packet.sCmd; //�����һ��packet�ͷ��أ������ѭ���᲻�ϵ���DealCommand
			}
		}
		//delete[]buffer;
		return -1; //�������
	}

	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_sock == -1) return false;
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
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

	CPacket& GetPack() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET; //-1
	}
private:

	std::vector<char> m_buffer; //����ֱ��������ȡ��ַ
	SOCKET m_sock;
	CPacket m_packet;
	//����ģʽ�������������������Ϊprivate
	//��������
	CClientSocket(const CClientSocket& ss) {}
	CClientSocket& operator=(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}
	CClientSocket()
	{
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0); //��������
		}
		//m_sock = socket(PF_INET, SOCK_STREAM, 0); //ѡ��Э���壺IPV4�� stream��TCPЭ��
		m_buffer.resize(BUFFERSIZE);
		memset(m_buffer.data(), 0, BUFFERSIZE); //
	}
	~CClientSocket() {
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
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp; //��ʾ�ͷŶ��ڴ�
		}
	}

	static CClientSocket* m_instance; //���óɾ�̬��Ա������̬��������
	//CServerSocket* m_instance;
	class CHelper {
		//helper����socket�ĳ�Ա��helper���ͷŵ�ʱ�������delete��������
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};