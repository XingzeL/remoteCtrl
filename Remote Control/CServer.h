#pragma once
#include "CThread.h"
#include <map>
#include "CSafeQueue.h"
#include "MSWSock.h"

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
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    std::vector<char> m_buffer; //������
    ThreadWorker m_worker; //��Ӧ�Ĵ�����
    CServer* m_server; //����������
    PCLIENT m_client; //��Ӧ�Ŀͻ���
    WSABUF m_wsabuffer;
};

template<EOperator> class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

template<EOperator> class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EOperator> class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;


class CClient {
public:
    CClient();

    ~CClient() {
        closesocket(m_sock);
    }

    void SetOverlapped(PCLIENT& ptr);

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

    bool Recv() {
        int ret = recv(m_sock, m_buffer.data(), m_buffer.size(), 0); //vector��.data()���ص�һ��Ԫ�صĵ�ַ
        if (ret <= 0) return -1;
        m_used += (size_t)ret; //��¼��ȡ�˶����ֽ�
        //TODO: ��������
        return 0;
    }

private:
    SOCKET m_sock;
    DWORD m_received;
    DWORD m_flags;

    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED> m_recv;
    std::shared_ptr<SENDOVERLAPPED> m_send;
    std::vector<char> m_buffer;
    size_t m_used; //�Ѿ�ʹ�õĻ������Ĵ�С
    sockaddr_in m_laddr; //��ַ
    sockaddr_in m_raddr;
    bool m_isbusy;

};


template<EOperator>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped();
    int AcceptWorker();
    PCLIENT m_client;
};

template<EOperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
    RecvOverlapped();

    int RecvWorker() { //Accept����
        int ret = m_client->Recv(); //���ݲ�����ֱ�ӱ�overlapped���ͣ����������Ļ������У�Ҫ����recv
        return ret;
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
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    }
    ~CServer() {}

    bool StartServer() {
        CreateSocket();
        if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        if (listen(m_sock, 5) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4); //����һ��iocp
        if (m_hIOCP == NULL) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); //��socket��iocp
        m_pool.Invoke(); //�����̳߳�
        m_pool.DispathchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
        if (!NewAccept()) return false;

        return true;
    }

    bool NewAccept() {
        PCLIENT pClient(new CClient());
        pClient->SetOverlapped(pClient);
        m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
        if (!AcceptEx(m_sock,
                *pClient,
                *pClient,
                0,
                sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
                *pClient,
                *pClient))
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        return true;
    }

private:
    void CreateSocket() {
        m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

     }

    int threadIocp();

public:
    CThreadPool m_pool; //�̳߳�
    HANDLE m_hIOCP; //��ɶ˿�
    SOCKET m_sock;  //����˵�socket
    sockaddr_in m_addr;  //server�����ĵ�ַ
    std::map<SOCKET, std::shared_ptr<CClient>> m_client; //ÿһ���ͻ������Ӷ���һ����Ӧ����Ϣ
    SafeQueue<CClient> m_lstClient;
};

