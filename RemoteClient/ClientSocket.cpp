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

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc() //��һ���߳�����������
{
	std::string strBuffer;
	strBuffer.resize(BUFFERSIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0; //index:һ����ȡ�˶����ֽ�
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			
			TRACE("lstSend size:%d\r\n", m_lstSend.size());
			//˵��������Ҫ����
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("����ʧ��\r\n");

				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
				auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(head.hEvent, std::list<CPacket>()));


				do {
					//int length = recv(m_sock, pBuffer, BUFFERSIZE - index, 0); //���ﵼ��bug
					int length = recv(m_sock, pBuffer + index, BUFFERSIZE - index, 0);
					TRACE("recv %d %d\r\n", length, index); //recvΪ0ʱ
					if (length > 0 || (index > 0)) { //��ȡ�ɹ�
						//���
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size); //�����size������
						//��ص�ʱ����һ���������һ��pack�������ж��TCP����
						//���ļ���ʱ����һ������������pack��������յ�һ�ξ�pop_front�������İ����ò�����Ӧ��handle����Ҫ���
						if (size > 0) {
							//TODO��֪ͨ��Ӧ���¼�
							pack.hEvent = head.hEvent; //pack�ǽ��յ��İ�����hEvent�ͷ��͵İ���eventƥ�䣬Ȼ�󼤻�event
							it->second.push_back(pack); //pr�Ǹ�pair���ڶ���Ԫ����bool�����ｫ��Ӧ���ŵ��˶���������
							/*SetEvent(head.hEvent); */
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) {
								SetEvent(head.hEvent);
								break;
							}
						}
					}

					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);
						if (it0 != m_mapAutoClosed.end()) {
							m_mapAutoClosed.erase(it0);
							//TRACE("SetEvent %d %d \r\n", head.sCmd, it0->second);
						}
						else {
							TRACE("�쳣�����û�ж�Ӧ��pair\r\n");
						}
						//m_mapAutoClosed.erase(it0);  //��Ҫ��AutoClosed��״̬���
						break; //��AutoCloseΪfalse��ʱ����Ҫ����break����Ȼ��һֱ��������ѭ��
					}

				} while (it0->second == false); //��������Զ��رյĻ��ͻ�һֱ����
				//index > 0 : ��ֹ�ڲ��رյ�����ʱ����recv=0ʱ������ѭ��
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();
			if (InitSocket() == false) {
				InitSocket();
			}
		}
		//���г���С�ڵ���0��ʱ����Ҫ����һ��
		Sleep(1);
	}
	CloseSocket();
}
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/)
//��Ϣ��������Ӧ��Ҳ����Ϣ���Ὣ���ݷŵ�wParam��lParam��
{//��Ҫ����һ����Ϣ�����ݽṹ(���ݳ��ȣ�ģʽautoclose��)�� �ص���Ϣ�����ݽṹ(��Ҫ֪�������ڵ�HANDLE�� MESSAGE)
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)wParam, (int)lParam, 0);
		if (ret > 0) {

		}
		else {
			CloseSocket();
			//������ֹ����
		}
	}
	else {

	}
	if (m_sock == -1) return;
	
	//TODO:
}
//10.6.8���⣺�ͻ��˰��ճ����ӽ��У���serverÿ������һ�������͹ر�socket�������ٴη���ʱ���ʧ��

void CClientSocket::threadFunc2() {
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}