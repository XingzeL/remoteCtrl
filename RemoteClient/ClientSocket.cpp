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

bool CClientSocket::InitSocket()
{
	//�޸ĵ�bug����Ϊÿ������Ϣ����ر����ӣ�client������serverһ��socketֻ��ʼ��һ��
	// ÿ��Inite��Ҫ��ʼ��m_sock

	//10.6.10����������: �ڷ��ͽ��յ��߳��л����InitSocket,CloseSocket������socket֮��
	if (m_sock != INVALID_SOCKET) CloseSocket(); //�����ϵ��CloseSocket������socket֮��
	m_sock = socket(PF_INET, SOCK_STREAM, 0); //ѡ��Э���壺IPV4�� stream��TCPЭ��
	//TODO: ����socket��У��
	if (m_sock == -1) return false;
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	//serv_adr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//TRACE("addr: %08X nIP %08X\r\n", serv_adr.sin_addr.s_addr, nIp);
	serv_adr.sin_addr.s_addr = htonl(m_nIP);
	serv_adr.sin_port = htons(m_nPort);
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

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClose, WPARAM wParam)
{
	//if (m_hThread == INVALID_HANDLE_VALUE) {
	//	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	//} //��һ�η���û����Ϣ�����̵߳Ļ��ʹ���һ��
	UINT nMode = isAutoClose ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	//����Ϣ�����̣߳����͸���Ӧ���̺߳Ŷ�Ӧ����Ϣ�Ͱ���Ŀǰֻ��WM_SEND_PACK��Ϣ������һ�����õ�����Ӧ�Ͳ�ͬ����Ӧ��ͬ��Ӧ����OnSendPacketAck�жԺ�����(ʹ��switch-case)
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData,(LPARAM)hWnd);
	//������Ϣ����Ϣ�߳�
	if (ret == false) {
		delete pData;
	}
	return ret;
}

/* ���л���ʱ���SendPacket
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClose)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		//if (InitSocket() == false) return false;
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this); //��Ч��ʱ�������߳�,����߳����ڷ��ͺͽ�����Ӧ��
		TRACE("start thread\r\n"); //�����Ƿ��̵߳Ĵ���ֻ��һ��
	}

	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE,
		std::list<CPacket>&>(pack.hEvent, lstPacks)); //һ���������������Ӧ��ʱ��handle����Ӧ���б�
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClose));
	TRACE("cmd:%d event %08X\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	//������handleд���Ƿ��Զ��رյ���Ϣ

	m_lstSend.push_back(pack); //���뵽�����б���
	m_lock.unlock();
	//���߳̽��з��ͺͽ��գ����ͺ������ȴ���Ӧ�İ�������ɹ���ͻ���Ӧ��Ӧ��Event
	WaitForSingleObject(pack.hEvent, INFINITE); //�ȴ���Ӧ
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);

	if (it != m_mapAck.end()) {
		std::list<CPacket>::iterator i;
		//10.6.8��������ʦ�����forɾ���ˣ����ǵ���û�л�����ʾ
		//for (i = it->second.begin(); i != it->second.end(); i++) { 
			//���forѭ��û�еĻ�����Ͳ�����ʾ��SendCommandPacket�ͻ᷵��-1��ΪlstPack�Ĵ�С��0
			// ��m_mapAck�е�list�ĳ�&ǰ���е��ֶ�����
		//	lstPacks.push_back(*i);
		//}
		m_mapAck.erase(it);
		return true;
	}
	return false;
}
*/
CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

CClientSocket::CClientSocket():
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true),
	m_hThread(INVALID_HANDLE_VALUE)
{
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0); //��������
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL); //Ĭ�ϣ��Զ����ã���ʼֵΪδ��������Ϊ""
	
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("������Ϣ�����߳�����ʧ��!\r\n");
	}
	CloseHandle(m_eventInvoke); //֮��Ͳ���Ҫ���event��
	//m_sock = socket(PF_INET, SOCK_STREAM, 0); //ѡ��Э���壺IPV4�� stream��TCPЭ��
	m_buffer.resize(BUFFERSIZE);
	memset(m_buffer.data(), 0, BUFFERSIZE);
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL},
	};
	for (int i = 0; funcs[i].message != 0; ++i) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
			TRACE("����ʧ�ܣ���Ϣֵ�� %d  ����ֵ�� %08X  ��ţ�%d\r\n", funcs[i].message, funcs[i].func, i);
	}

}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	//thiz->threadFunc();
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/)
//��Ϣ��������Ӧ��Ҳ����Ϣ���Ὣ���ݷŵ�wParam��lParam��
{//��Ҫ����һ����Ϣ�����ݽṹ(���ݳ��ȣ�ģʽautoclose��)�� �ص���Ϣ�����ݽṹ(��Ҫ֪�������ڵ�HANDLE�� MESSAGE)
	PACKET_DATA data = *(PACKET_DATA*)wParam; //�����ݸ��Ƶ�����������ֹ��һ���̵߳ľֲ��������ͷ�
	//���߳�֮�䷢�����ݣ�������͵���һ���ֲ������Ļ����ܻ�������߳�ʹ��֮ǰ���ͷ���
	delete (PACKET_DATA*)wParam; //��һ���̴߳����İ���new�����ģ����յ��󣬸������ݣ������ͷţ��������Ӧ������ҲӦ��ʹ��
	HWND hWnd = (HWND)lParam;
	//�����Ƿ�ֹserver�ر���socket��������߻���һ������û���յ������current��
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {

		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0); //��Packet�����ݷ��ͳ�ȥ
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFERSIZE); //ʹ��resize�ĺô����õĶѵ��ڴ棬�ֿ����Զ���������new����
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFERSIZE - index, 0);
				if (length > 0 || (index > 0)) { //length: �յ������ݣ� index: �������е�����
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						//Ϊ�˿��̴߳���������ұ�֤�������ڣ�������һ���̵߳İ�Ӧ����new������
						if (data.nMode & CSM_AUTOCLOSE) {
							
							CloseSocket();
							return;
						}
						index -= nLen;
						//memmove(pBuffer, pBuffer + index, nLen);
						memmove(pBuffer, pBuffer + nLen, index);
					}
				}
				else {
					TRACE("recv failed length %d, index %d\r\n", length, index);
					//�Է��ر���socket���������豸�쳣
					CloseSocket();

					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1); //��Ӧ ֪ͨ����
				}
			}
		}
		else {
			CloseSocket();
			//������ֹ����
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1); //����ҵ���߳̽�����
		}
	}
	else {
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2); //���һֱ��
	}
	
	//TODO:
}
//10.6.8���⣺�ͻ��˰��ճ����ӽ��У���serverÿ������һ�������͹ر�socket�������ٴη���ʱ���ʧ��

void CClientSocket::threadFunc2() {
	//����eventΪ�Ѵ���
	SetEvent(m_eventInvoke); 
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}