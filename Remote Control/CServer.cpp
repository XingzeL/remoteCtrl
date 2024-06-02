#include "pch.h"
#include "CServer.h"
#include "utils.h"
#pragma warning(disable:4407)

template<EOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
    m_worker = ThreadWorker(this, (FUNCTYPE)& AcceptOverlapped<op>::AcceptWorker);
    m_operator = EAccept;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024);
    m_server = NULL;
}

template<EOperator op>
int AcceptOverlapped<op>::AcceptWorker() { //Accept动作，执行一次
    INT lLength = 0, rLength = 0;
    if (*(LPDWORD)*m_client > 0) {
        sockaddr* ploal = NULL, * promote = NULL;
        GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
            (sockaddr**)m_client->GetLocalAddr(), &lLength,//本地地址,设置了m_client的地址
            (sockaddr**)m_client->GetRemoteAddr() ,&rLength//远程地址


        );
        memcpy(m_client->GetLocalAddr(), ploal, sizeof(sockaddr_in));
        memcpy(m_client->GetRemoteAddr(), promote, sizeof(sockaddr_in));
        m_server->BindSocket(*m_client);

        int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1,
            *m_client, &m_client->flags(), *m_client, NULL);

        if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
            //排除WSA_IO_PENDING

        }

        if (!m_server->NewAccept()) //提交一次accept的请求
        {
            return -2; //accept请求发送失败
        }
    }
    return -1;
}


CClient::CClient() 
    : m_isbusy(false), m_flags(0), 
    m_overlapped(new ACCEPTOVERLAPPED()),
    m_recv(new RECVOVERLAPPED()),
    m_send(new SENDOVERLAPPED())
    //m_vecSend(this, (SENDCALLBACK)&CClient::SendData)
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(m_laddr));
    memset(&m_raddr, 0, sizeof(m_raddr));
}


void CClient::SetOverlapped(CClient* ptr) {
    m_overlapped->m_client = ptr;
    m_recv->m_client = ptr;
    m_send->m_client = ptr;
}

CClient::operator SOCKET() {
    return m_sock;
}

CClient::operator PVOID() {
    return &m_buffer[0]; //返回缓冲区的首地址
}

CClient::operator LPOVERLAPPED() {
    return &m_overlapped->m_overlapped;
}

CClient::operator LPDWORD() {
    return &m_received;
}

LPWSABUF CClient::RecvWSABuffer()
{
    return &m_recv->m_wsabuffer;
}

LPWSABUF CClient::SendWSABuffer()
{
    return &m_send->m_wsabuffer;
}

LPWSAOVERLAPPED CClient::SendOverlapped()
{
    return &m_send->m_overlapped;
}

bool CClient::Recv() {
    int ret = recv(m_sock, m_buffer.data(), m_buffer.size(), 0); //vector的.data()返回第一个元素的地址
    if (ret <= 0) return false;
    m_used += (size_t)ret; //记录读取了多少字节
    //TODO: 解析数据
    return true;
}

int CClient::Send(void* buffer, size_t nSize) {
    std::vector<char> data(nSize);
    memcpy(data.data(), buffer, nSize);
    //if (m_vecSend.PushBack(data)) { //第一版：Pop的操作在SendQueue中的threadtick中定时pop
    //    return 0;
    //}
    return -1;
}

int CClient::SendData(std::vector<char>& data)
{
   /* if (m_vecSend.Size() > 0) {
        int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
        if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
            Cutils::ShowError();
            return -1;
        }
    }*/
    return 0;
}




bool CServer::StartServer() {
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
    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4); //创建一个iocp
    if (m_hIOCP == NULL) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        m_hIOCP = INVALID_HANDLE_VALUE;
        return false;
    }
    CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); //绑定socket和iocp
    m_pool.Invoke(); //启动线程池
    m_pool.DispathchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
    if (!NewAccept()) return false; //发送给iocp一个accept请求， 完成后在threadIocp中得到通知

    return true;
}

bool CServer::NewAccept() {
    CClient* pClient = new CClient(); //创建的client
    pClient->SetOverlapped(pClient);
    //m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
    if (!AcceptEx(m_sock,
        *pClient,
        *pClient,
        0,
        sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
        *pClient,
        *pClient))
    {
        TRACE("%d\r\n", WSAGetLastError());
        if (WSAGetLastError() != WSA_IO_PENDING) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }

    }
    return true;
}

void CServer::CreateSocket() {
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    int opt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

}

int CServer::threadIocp()
{
    DWORD transferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
    TRACE("into ThreadIocp\r\n");
    if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITY))
    {
        if (transferred > 0 && (CompletionKey != 0)) {
            COverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, COverlapped, m_overlapped);
            TRACE("pOverlapped->m_operator %d \r\n", pOverlapped->m_operator);
            pOverlapped->m_server = this;
            switch (pOverlapped->m_operator) {
            case EAccept:
            {
                ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                m_pool.DispathchWorker(pOver->m_worker);
            }
            break;
            case ERecv:
            {
                RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                WSABUF* buffer;
                buffer = pOver->m_client->SendWSABuffer();
                m_pool.DispathchWorker(pOver->m_worker);
            }
            break;
            case ESend:
            {
                TRACE("Send\r\n");
                SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                m_pool.DispathchWorker(pOver->m_worker);
            }
            break;
            case EError:
            {
                ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                m_pool.DispathchWorker(pOver->m_worker);
            }
            break;
            }
        }
        else {
            return -1;
        }
    }
    return 0;
}

template<EOperator op>
inline SendOverlapped<op>::SendOverlapped() {
    m_operator = ESend;
    m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}


template<EOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
    m_operator = op;
    m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}

