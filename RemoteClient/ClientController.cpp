#include "pch.h"

#include "ClientController.h"

//ע�⣺��̬������Ա������Ҫ��cpp������һ�£��Ǿ�̬�������ǹ����ʱ���ʼ��
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper; //����

CClientController* CClientController::getInstance()
{
    if (m_instance == NULL) {
        m_instance = new CClientController();
        TRACE("CClientController size is %d\r\n", sizeof(*m_instance));
        struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
            {WM_SEND_PACK, &CClientController::OnSendPack},
            {WM_SEND_DATA, &CClientController::OnSendData},
            {WM_SHOW_STATUS, &CClientController::OnShowStatus},
            {WM_SHOW_WATCH, &CClientController::OnSendWatcher},
            {(UINT) - 1, NULL}
        };
        for (int i = 0; MsgFuncs[i].func != NULL; i++) {
            m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
        }
    }
    //return nullptr; //����д���nullptr���¿����߳�ʱ�����˷Ƿ���ַ
    return m_instance;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
    pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

int CClientController::InitController()
{
    //����һ���߳�
    m_hThread = (HANDLE)_beginthreadex(NULL, 0,
        &CClientController::threadEntry, this,
        0, &m_nThreadID); //�᷵��һ������ȫ�Ķ�
    m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg); //������״̬�Ĵ���
    return 0;
}

LRESULT CClientController::SendMessage(MSG msg)
{
    
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL) return -2;
    MSGINFO info(msg);
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent); //���̺߳ŷ�����Ϣ��Ȼ����취�õ����ؽ��
    //MSGINFO& inf = m_mapMessage.find(uuid)->second;
    WaitForSingleObject(hEvent, -1); //ͬ�����ȴ��¼�����
    CloseHandle(hEvent);
    return info.result; //�õ���Ϣ����ķ���ֵ
}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam) //����lstPackָ�룬���Էֱ����Ӧ���ǲ�����
{
    TRACE("cmd:%d %s start %lld \r\n", nCmd, __FUNCTION__, GetTickCount64());
    CClientSocket* pClient = CClientSocket::getInstance();

    bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);

    return ret;
}

int CClientController::DownFile(CString strPath)
{
    CFileDialog dlg(FALSE, "*",
        strPath,
        OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, NULL,
        &m_remoteDlg); //��һ��Ĭ�Ϻ�׺

    if (dlg.DoModal() == IDOK) {
        m_strRemote = strPath;
        m_strLocal = dlg.GetPathName();
        CString strLocal = dlg.GetPathName(); //MFC��API,�õ�·������Ҫ�����߳���
        FILE* pFile = fopen(m_strLocal, "wb+");
        if (pFile == NULL) {
            AfxMessageBox("����û��Ȩ�ޱ�����ļ������޷������ļ�����");
            return -1;
        }
        SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile); //������Ϣ���߳���Ϣ������pFile��Ϣ�������������4�� ���Զ��رգ��ļ���·��

        //�޸�remoteDlg״̬����������
        m_remoteDlg.BeginWaitCursor();
        m_statusDlg.m_info.SetWindowText(_T("��������ִ����"));
        m_statusDlg.ShowWindow(SW_SHOW);
        m_statusDlg.CenterWindow(&m_remoteDlg);
        m_statusDlg.SetActiveWindow();
        //Sleep(50); //����һЩ��ʱ���ȴ����λ�ñ��õ�
    }
    return 0;
}

void CClientController::DownloadEnd()
{
    m_statusDlg.ShowWindow(SW_HIDE);
    m_remoteDlg.EndWaitCursor();
    m_remoteDlg.MessageBox(_T("������ɣ�"), _T("���"));
}

void CClientController::StartWatchScreen()
{
    m_isClosed = false;
    //CWatchDialog dlg(&m_remoteDlg);//������ع�ǰ���½�һ�����棬����c����Ҫ�ó�Աm_watchDlg
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry,
        0, this);
    m_watchDlg.DoModal(); //�������Ӵ���
    m_isClosed = true;
    WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
    Sleep(50);
    ULONGLONG nTick = GetTickCount64();
    while (!m_isClosed) {
        if (m_watchDlg.isFull() == false) {
            if (GetTickCount64() - nTick < 100) {
                Sleep(100 - DWORD(GetTickCount64() - nTick)); //�趨һ��50���������
               
            }
            nTick = GetTickCount64(); //����
            std::list<CPacket> lstPacks;
            int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);  //��M�㽻������������
            //TODO: �����Ϣ��Ӧ����WM_SEND_PACK_ACK
            //TODO: ���Ʒ���Ƶ��
            if (ret == 1) { //�õ�cmd�źʹ��ص�����
                
                //error: ��������6������m_image�ǿ�
                TRACE("�ɹ�����ͼƬ\r\n");
            }
            else {
                TRACE("��ȡͼ��ʧ��");
            }
        }
        Sleep(1);
    }
    TRACE("thread end %d\r\n", m_isClosed);

}

void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();
}

void CClientController::threadDownloadFile()
{
    FILE* pFile = fopen(m_strLocal, "wb+");
    if (pFile == NULL) {
        AfxMessageBox("����û��Ȩ�ޱ�����ļ������޷������ļ�����");
        m_statusDlg.ShowWindow(SW_HIDE);
        m_remoteDlg.EndWaitCursor(); //��������ɳ©
        return;
    }
    CClientSocket* pClient = CClientSocket::getInstance();

    do {

        //int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength()); //1.ԭ����V��ֱ�ӵ��÷��Ͱ��ĺ���������updata�����ͻ

        //int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile); //2.��Ϊ��V�㷢����Ϣ�����߳̽��պ���а��ķ���
        int ret = SendCommandPacket(m_remoteDlg,4, false,
            (BYTE*)(LPCSTR)m_strRemote,
            m_strRemote.GetLength(), (WPARAM)pFile);  //��Ϣ���ƣ��̼߳���Ϣ����pFile

        if (ret < 0) {
            AfxMessageBox(_T("ִ����������ʧ��!!!"));
            TRACE("ִ������ʧ��: ret = %d\r\n", ret);
            break;
        }
        //���н��� - �ŵ�����Ϣack����
        //1.longlong ����  

        long long nlength = *(long long*)CClientSocket::getInstance()->GetPack().strData.c_str();
        if (nlength == 0) {
            AfxMessageBox("�ļ�����Ϊ������޷���ȡ�ļ�����");
            break; //����do��ѭ������������Ĺر�����
        }

        long long nCount = 0; //ά�����յĳ�����Ϣ
        //�����ļ�����fopen��fwrite
        //��һ��ģ̬�ĶԻ���
        while (nCount < nlength) {
            ret = pClient->DealCommand();
            if (ret < 0) {
                AfxMessageBox("����ʧ��!");
                TRACE("����ʧ�ܣ�ret = %d\r\n", ret);
                break;
            }

            fwrite(pClient->GetPack().strData.c_str(), 1, pClient->GetPack().strData.size(), pFile);
            nCount += pClient->GetPack().strData.size();
        }
    } while (false);
    fclose(pFile);
    pClient->CloseSocket();
    m_statusDlg.ShowWindow(SW_HIDE);
    m_remoteDlg.EndWaitCursor();
    m_remoteDlg.MessageBox(_T("������ɣ�"), _T("���"));
    m_remoteDlg.LoadFileInfo(); //����һ��
}

void CClientController::threadDownloadEntry(void* arg)
{
    //thiz��������̬��Ա����ת�����ܷ���this�Ķ����Ա����
    CClientController* thiz = (CClientController*)arg;
    thiz->threadDownloadFile();
    _endthread();
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadFunc();
    _endthreadex(0);
    return 0;
}


//#define WM_SEND_PACK (WM_USER + 1)  //���Ͱ�����
//#define WM_SEND_DATA (WM_USER + 2)  //������������
//#define WM_SHOW_STATUS (WM_USER + 3) //չʾ״̬
//#define WM_SHOW_WATCH (WM_USER + 4) //Զ�̼��
void CClientController::threadFunc() //���Ʋ���̣߳���Ϣ����
{
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0)) { //��Ϣѭ������������V�����Ϣ
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_SEND_MESSAGE) { //�����Ϣ����SendMessages������
            MSGINFO* pmsg = (MSGINFO*)msg.wParam;
            HANDLE hEvent = (HANDLE)msg.lParam;
            //�õ����µ�message
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end()) {
                //std::map<UUID, PMSGINFO>::iterator it = m_mapMessage.find(*puuid);
                pmsg->result = (this->*it->second)(pmsg->msg.message,
                    pmsg->msg.wParam, pmsg->msg.lParam);
            }
            else {
                pmsg->result = -1;
            }
            SetEvent(hEvent); //event�ܹ���Ǹ��ԵĴ���
        }

        else {
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end()) {
                (this->*it->second)(msg.message, msg.wParam, msg.lParam); //�õ�һ������ֵ
                //��Ϊ��Ҫ���ó�Ա����������ǰ����Ҫthis������
            }
        }

    }
}

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    //CClientSocket* pClient = CClientSocket::getInstance();
    //CPacket* pPacket = (CPacket*)wParam;
    //return pClient->Send(*pPacket); //���Ͱ�;
    return LRESULT();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    //CClientSocket* pClient = CClientSocket::getInstance();
    //char* pBuffer = (char*)wParam;
    //return pClient->Send(pBuffer, (int)lParam); //���Ͱ�;
    return LRESULT();
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();
}
