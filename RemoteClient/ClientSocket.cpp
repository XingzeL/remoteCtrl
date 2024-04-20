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

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc() //��һ���߳�����������
{
	if (InitSocket() == false) {
		return;
	}
	std::string strBuffer;
	strBuffer.resize(BUFFERSIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0; //index:һ����ȡ�˶����ֽ�
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			//˵��������Ҫ����
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("����ʧ��\r\n");
				continue;
			}
			auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));
			
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
					pack.hEvent = head.hEvent;
					pr.first->second.push_back(pack); //pr�Ǹ�pair���ڶ���Ԫ����bool
					SetEvent(head.hEvent);
				}
			}

			else if (length == 0 && index <= 0) {
				CloseSocket();
			}
			m_lstSend.pop_front();
		}
	}
}
