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

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClose)
{
	if (m_sock == INVALID_SOCKET) {
		//if (InitSocket() == false) return false;
		_beginthread(&CClientSocket::threadEntry, 0, this); //��Ч��ʱ�������߳�
	}
	auto pr = m_mapAck.insert(std::pair<HANDLE,
		std::list<CPacket>>(pack.hEvent, lstPacks));

	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClose));
	//������handleд���Ƿ��Զ��رյ���Ϣ

	m_lstSend.push_back(pack); //���뵽�����б���
	//���߳̽��з��ͺͽ��գ����ͺ������ȴ���Ӧ�İ�������ɹ���ͻ���Ӧ��Ӧ��Event
	WaitForSingleObject(pack.hEvent, INFINITE); //�ȴ���Ӧ
	std::map<HANDLE, std::list<CPacket>>::iterator it;
	it = m_mapAck.find(pack.hEvent);

	if (it != m_mapAck.end()) {
		std::list<CPacket>::iterator i;
		//10.6.8��������ʦ�����forɾ���ˣ����ǵ���û�л�����ʾ
		for (i = it->second.begin(); i != it->second.end(); i++) { 
			//���forѭ��û�еĻ�����Ͳ�����ʾ��SendCommandPacket�ͻ᷵��-1��ΪlstPack�Ĵ�С��0
			lstPacks.push_back(*i);
		}
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
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("����ʧ��\r\n");

				continue;
			}
			std::map<HANDLE, std::list<CPacket>>::iterator it;
			it = m_mapAck.find(head.hEvent);
			std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
			auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));
			
			do {
				int length = recv(m_sock, pBuffer, BUFFERSIZE - index, 0);
				if (length > 0 || index > 0) { //��ȡ�ɹ�
					//���
					index += length;
					size_t size = (size_t)index;
					CPacket pack((BYTE*)pBuffer, size); //�����size������
					//��ص�ʱ����һ���������һ��pack�������ж��TCP����
					//���ļ���ʱ����һ������������pack��������յ�һ�ξ�pop_front�������İ����ò�����Ӧ��handle����Ҫ���
					if (size > 0) {
						//TODO��֪ͨ��Ӧ���¼�
						pack.hEvent = head.hEvent; //pack�ǽ��յ��İ�����hEvent�ͷ��͵İ���eventƥ�䣬Ȼ�󼤻�event
						it->second.push_back(pack); //pr�Ǹ�pair���ڶ���Ԫ����bool
						SetEvent(head.hEvent);
						memmove(pBuffer, pBuffer + size, index - size);
						index -= size;
						if (it0->second) {
							SetEvent(head.hEvent);
						}
					}
				}

				else if (length == 0 && index <= 0) {
					CloseSocket();
					SetEvent(head.hEvent);
				}
				
			} while (it0->second == false); //��������Զ��رյĻ��ͻ�һֱ����
			m_lstSend.pop_front();
			InitSocket();
			//CloseSocket(); //���Ӳ��Ե�ʱ�����
		}
	}
	CloseSocket();
}
//10.6.8���⣺�ͻ��˰��ճ����ӽ��У���serverÿ������һ�������͹ر�socket�������ٴη���ʱ���ʧ��