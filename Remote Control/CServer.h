#pragma once
#include "CThread.h"
#include <map>
#include "CSafeQueue.h"
#include "MSWSock.h"
#include "Packet.h"
#include "Command.h"

static CCommand m_cmd; //�������

enum EOperator{
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};

class CServer;
class CClient;
typedef std::shared_ptr<CClient> PCLIENT;

class COverlapped {
public:
    OVERLAPPED m_overlapped; //overlapped����
    DWORD m_operator;  //�������
    std::vector<char> m_buffer; //������
    ThreadWorker m_worker; //��Ӧ�Ĵ�����
    CServer* m_server; //����������
    CClient* m_client; //��Ӧ�Ŀͻ���
    WSABUF m_wsabuffer; //����WSASend, WSARecv û��
    virtual ~COverlapped() {  //����������
        m_buffer.clear();
    }
};

template<EOperator> class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

template<EOperator> class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EOperator> class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

//******************��װclient**********************

class CClient: public ThreadFuncBase {
public:
    CClient();

    ~CClient() {
        closesocket(m_sock);
        m_recv.reset();
        m_send.reset();
        m_overlapped.reset();
        m_buffer.clear();
    }

    void SetOverlapped(CClient* ptr);

    operator SOCKET();

    operator PVOID();

    operator LPOVERLAPPED();

    operator LPDWORD();

    LPWSABUF RecvWSABuffer();

    LPWSABUF SendWSABuffer();

    sockaddr_in* GetLocalAddr() { return &m_laddr; }
    sockaddr_in* GetRemoteAddr() { return &m_raddr; }
    size_t GetBufferSize() const { return m_buffer.size(); }
    DWORD& flags() { return m_flags; }
    LPWSAOVERLAPPED SendOverlapped();

    bool Recv();

    int Send(void* buffer, size_t nSize);
    int SendData(std::vector<char>& data); //������Ϊcallback����������
    
    std::vector<char> m_buffer;
    std::list<CPacket> recvPackets;
    std::list<CPacket> sendPackets;

public:
    SOCKET m_sock;
    DWORD m_received;
    DWORD m_flags;

    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED> m_recv;
    std::shared_ptr<SENDOVERLAPPED> m_send;

    size_t m_used; //�Ѿ�ʹ�õĻ������Ĵ�С
    sockaddr_in m_laddr; //��ַ
    sockaddr_in m_raddr;
    bool m_isbusy;
    //SendQueue<std::vector<char>> m_vecSend; //�������ݵĶ���
};


//******************************������ɶ˿ں�Ĳ���********************
template<EOperator>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped();
    int AcceptWorker();
    //PCLIENT m_client; //ָ����������client���������������໥��������client�����ǰ���������ṩ��������һ����Ĵ��ڵ���Ϣ
};

template<EOperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
    RecvOverlapped();

    int RecvWorker() { //Accept����
        int index = 0;
        int len = m_client->Recv(); //���ݲ�����ֱ�ӱ�overlapped���ͣ����������Ļ������У�Ҫ����recv
        index += len;
        if (index >= 0) {
            CPacket pack((BYTE*)m_client->m_buffer.data(), (size_t&)index);
            m_cmd.ExecuteCommand(pack.sCmd, m_client->sendPackets, pack);
            if (pack.sCmd != 5) {
                //����mouse event
                WSASend((SOCKET)*m_client, m_client->SendWSABuffer(),
                    1, *m_client, m_client->flags(), m_client->SendOverlapped(), NULL);
            }
            else {
                closesocket(m_client->m_sock);
            }
        }
        
        return -1;
    }
};


template<EOperator>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
    SendOverlapped();
    //    : m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
    //    memset(&m_overlapped, 0, sizeof(m_overlapped));
    //    m_buffer.resize(1024 * 256);
    //}
    int SendWorker() { //Send����

        while (m_client->sendPackets.size() > 0) {
            CPacket pack = m_client->sendPackets.front();
            m_client->sendPackets.pop_front();
            int ret = send(m_client->m_sock, pack.Data(), pack.Size(), 0);
            TRACE("send ret: %d\r\n", ret);
        }
        closesocket(m_client->m_sock);
        return -1;
    }
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EOperator>
class ErrorOverlapped : public COverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped() : m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }
    int ErrorWorker() { //Error����
        return -1;
    }
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class CServer :
    public ThreadFuncBase
{
public:
    CServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
        //��ʼ��IOCP
        m_hIOCP = INVALID_HANDLE_VALUE;
        //m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    }
    ~CServer() {}
    void BindSocket(SOCKET m_sock) {
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
    }
    bool StartServer();

    bool NewAccept();

private:
    void CreateSocket();

    int threadIocp();

public:
    CThreadPool m_pool; //�̳߳�
    HANDLE m_hIOCP; //��ɶ˿�
    SOCKET m_sock;  //����˵�socket
    sockaddr_in m_addr;  //server�����ĵ�ַ
    std::map<SOCKET, std::shared_ptr<CClient>> m_client; //ÿһ���ͻ������Ӷ���һ����Ӧ����Ϣ
    SafeQueue<CClient> m_lstClient;
};

