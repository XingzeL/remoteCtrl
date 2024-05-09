#pragma once

#include "pch.h"
#include <string>
#include "framework.h"
#include <vector>
#include <list>
#include <map>
#include <mutex>

//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma pack(push)
#pragma pack(1)
#define WM_SEND_PACK (WM_USER + 1)  //���Ͱ�����
#define WM_SEND_PACK_ACK (WM_USER+2) ///���Ͱ�����Ӧ��
//void Dump(BYTE* pData, size_t nSize);
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

	CPacket(const BYTE* pData, size_t& nSize){
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

	/*
		ԭ��Data������û�в����ģ����Ǹı�CPacket�ĳ�Ա������
		�ڽ��з��͵�ʱ�����ݴ����strOut������
	*/

	const char* Data(std::string& strOut) const { //3.����const��׺�������޸Ķ�����
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
	//std::string strOut; //������������
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

enum {
	CSM_AUTOCLOSE=1, //CSM = Client Socket Mode �Զ��ر�ģʽ
};

typedef struct PacketData{ //��Ϣ�д����Packet����2��Ԫ�أ�data��mode
	std::string strData;
	UINT nMode;
	PacketData(const char* pData, size_t nLen, UINT mode) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
		}
		return *this;
	}
}PACKET_DATA;

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

	bool InitSocket();

#define BUFFERSIZE 4096000
	int DealCommand() { //���շ���������Ӧ������һ��packet������ֵΪ��Ӧ������ֵ
		if (m_sock == -1) return false;
		//char buffer[1024] = "";
		char* buffer = m_buffer.data();  //TODO:���̷߳�������ʱ���ܻ���ֳ�ͻ
		//ԭ��û��delete����ԭ��buffer��ȷ���ǳ����ӻ��Ƕ�����

		if (buffer == NULL) {
			TRACE("�ڴ治�㣡\r\n");
			return -2;
		}
		/*memset(buffer, 0, BUFFERSIZE);*/ //�����������Ļ��Ὣm_buffer��data���
		static size_t index = 0; //index��Ϊstatic���´ν���DealCommand��ʱ��������ն��������
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFERSIZE - index, 0);
			if (((int)len <= 0) && ((int)index == 0)) { //10.3.3(����ļ�����bug)����index == 0���жϣ�index���������л�ʣ�������Ƿ���ꣻlen������recv��ȡʧ��
				//delete[]buffer; //�����ӵ������ֻ����һ��cmd֮����Բ�����
				TRACE("���ֶ��������⣡");
				return -1;
			}
			//Dump((BYTE*)buffer, index);
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

	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,
	//	bool isAutoClose = true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClose = true);

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

	void UpdateAddress(int nIP, int nPort) {
		if (m_nIP != nIP || m_nPort != nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

private:
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::map<HANDLE, bool> m_mapAutoClosed;
	std::list<CPacket> m_lstSend; //Ҫ���͵�����
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; //��Ϊ�Է���Ӧ��һϵ�еİ����ĳ�list�����ã��Ͳ����ֶ��Ѱ����ȥ
	//�����vector���������ȶ�ʱ�����ܴ����ſռ������push_back���ĵ�ʱ��ָ������
	
	//M�㣺����IP��Port����Ϣ
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer; //����ֱ��������ȡ��ַ
	SOCKET m_sock;
	CPacket m_packet;
	//����ģʽ�������������������Ϊprivate
	//��������
	CClientSocket(const CClientSocket& ss);


	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket();
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
		//���������ĺô���������ȫ�ֵ�ʱ��ֻ��main��������ã�Ҳ����Ӱ���κ��ж�
		//���˿�����ʱ�������main���ͷţ����ܱ�����ڲ�֪�����ں���д��Ҫ�õ�sock�Ĵ��룬���´���
		//ֻ��һ�εĲ����������ö���ȫ�ֵķ�ʽ
		//���ǵ���main���������������CServerSocket������һ���ֲ���������ʼ���ֻᱻ����һ�Ρ�������� ����ģʽ
	}

	static unsigned __stdcall threadEntry(void* arg); //�������̺߳ŵ�beginthreadex�Ļ�����Ҫ����Ϊunsigned __stdcall
	void threadFunc();
	void threadFunc2(); //�߳̽�����Ϣ���߳��е���ע��õĻص�����

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
			TRACE("Socket has been released!\r\n");
			delete tmp; //��ʾ�ͷŶ��ڴ�
		}
	}

	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	/*
	  ԭ��Send����Ĳ���const pack���ڽ��������send��ʱ�����Data()�ı�pack�е�����
	  ��C���ع��еķ��ͽӿڴ������const(Ϊ�˵������), ����ı�pack����ĺ�����Ҫ�޸ģ�
	  �ӵ���ԭ��Data()�ı�pack��strOut��Ա�ĳ�һ���ֲ�������Ϊ���������µ�Data(string&)
	  �����ı�ľ����������������pack��������C��ĺ���Ҫ��
	 */
	bool Send(const CPacket& pack) { //1.�������ı��const
		if (m_sock == -1) return false;
		std::string strOut;
		pack.Data(strOut); //2.const����ֻ�ܵ�����const��׺�ĺ���,����ı�pack��Data������ı�strOut
		return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
	}

	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);

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