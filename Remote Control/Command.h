#pragma once
#include <map>
#include "ServerSocket.h"
#include <atlimage.h>
#include <direct.h>
#include "utils.h"

#include <stdio.h>
#include <io.h>
#include <list>

#include "LockDialog.h"
#include "Resource.h"

class CCommand
{
public:
	CCommand();
    ~CCommand() {};
	int ExecuteCommand(int nCmd);

protected:
	typedef int(CCommand::* CMDFUNC)(); //��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction; //����ŵ����ܵ�ӳ��
    CLockDialog dig;
    unsigned threadId = 0;

protected:
    static unsigned __stdcall threadLockDlg(void* arg) {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }

    void threadLockDlgMain() {
        TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        dig.Create(IDD_DIALOG_INFO, NULL); //��������
        dig.ShowWindow(SW_SHOW);

        //�����ƶ����ڱκ��������
        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN) / 2; //w1
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) / 2;
        dig.MoveWindow(rect);

        CWnd* pText = dig.GetDlgItem(IDC_STATIC);
        if (pText) {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWidth = rtText.Width() / 2; //w0
            int x = (rect.right - nWidth) / 2; //
            int nHeight = rtText.Height();
            int y = (rect.bottom - nHeight) / 2;
            pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
        }

        dig.SetWindowPos(&dig.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //û����Ϣѭ���Ļ���������ʧ

        TRACE("right = %d bottom = %d/r/n", rect.right, rect.bottom);
        //dig.GetWindowRect(rect); //��ȡ���ڷ�Χ

        ShowCursor(false); //�ڴ����ھͲ���������
        //::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE); //����������
        rect.left = 0;
        rect.top = 0;
        rect.right = 1;
        rect.bottom = 1;
        ClipCursor(rect); //����������ڴ��ڷ�Χ�ڣ�ֻ��һ�����ص�

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) { //�Ի���������Ϣѭ��
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN) { //���յ��������ͷ�
                TRACE("msg::%08X wparam:%08x lparam:%08X/r/n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x1B) { //��esc����ʱ��
                    break;
                }

            }
        }
        ClipCursor(NULL); //�ָ����
        dig.DestroyWindow(); //�������Create�ɶ�
        ShowCursor(true);
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); //�ָ�������
        _endthreadex(0);
    }

protected:
    int LockMachine() {
        //��Ҫ����һ����Ϣ��������ϵ����Ա�� ��ʾ�����
        //ģ̬���Ƿ�ģ̬ û��ģ̬��������Ϊ�����������

        //����ǿջ�����Ч�Ϳ����߳�
        if ((dig.m_hWnd == NULL) || (dig.m_hWnd == INVALID_HANDLE_VALUE)) {
            //_beginthread(threadLockDlg, 0, NULL); //����һ���µ��̣߳��Լ�����������Ȼ���ղ��������Ϣ��
            _beginthreadex(NULL, 0, CCommand::threadLockDlg, this, 0, &threadId);
            TRACE("threadId = %d\r\n", threadId);
        }

        CPacket pack(7, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int UnLockMachine() {
        //dig.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);  //����������Ϣ
        //::SendMessage(dig.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001); //ȫ������Ϣ
        //����������������Ϣ�ķ�ʽ�����ᱻ��һ���߳��յ�
        PostThreadMessage(threadId, WM_KEYDOWN, 0x1B, 0); //��ҪthreadID
        CPacket pack(8, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int TestConncet() {
        CPacket pack(1981, NULL, 0);

        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("����˷���ret: %d\r\n", ret);
        return 0;
    }

    int DeleteLocalFile() {

        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath); //�õ��ļ�·��
        TCHAR sPath[MAX_PATH] = _T("");
        //mbstowcs(sPath, strPath.c_str(), strPath.size()); //���Ŀ��ܻ������뵼��ɾ�������ļ�
        MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(),
            sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFile(sPath);
        //������Ӧ
        CPacket pack(9, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("����˷���ret: %d\r\n", ret);
        return 0;
    }

    int MakeDriverInfo() {
        //�������̷�������Ϣ
        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0) {
                if (result.size() > 0) {
                    //ǰ���Ѿ���������
                    result += ',';
                }
                result += 'A' + i - 1;
            }
        }
        //CServerSocket::getInstance()->Send(CPacket(1, (BYTE*)result.c_str(), result.size())); //�����֧�У�socket����Ӵ��ʼ��
        CPacket pack(1, (BYTE*)result.c_str(), result.size());
        //Dump((BYTE*)&pack, pack.nLength + 6); //(BYTE*)&pack����ȡ���ݵķ�ʽ�����⣬��Ϊpack��һ������ȡ��ַ����õ����������
        Cutils::Dump((BYTE*)pack.Data(), pack.Size());
        CServerSocket::getInstance()->Send(pack);
        return 0;
        //_chdrive(1); //�ı䵱ǰ������������ܸı�ɹ���˵����������
    }



    int MakeDirectoryInfo() {

        //��ȡָ���ļ��е��ļ�
        std::string strPath;
        //std::list<FILEINFO> IstFileInfos; //�����Ѽ���Ϣ
        if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
            OutputDebugString(_T("��ǰ����ǻ�ȡ�ļ���Ϣ�������������"));
            return -1;
        }
        if (_chdir(strPath.c_str()) != 0) {
            // OutputDebugString(_T(strPath.c_str());
             //�л���Ŀ¼ʧ��
            FILEINFO finfo;
            finfo.IsInvalid = TRUE;
            finfo.IsDirectory = FALSE;
            finfo.HasNext = FALSE;
            //memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
            //IstFileInfos.push_back(finfo);��//��һ���Ѽ�һ������һ��һ�����ļ���Ϣ������
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            if (CServerSocket::getInstance()->Send(pack) == false) {
                OutputDebugString(_T("������ʧ��"));
                return -4;
            }

            OutputDebugString(_T("û��Ȩ�޷���Ŀ¼"));
            return -2;
        }

        _finddata_t fdata;
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) { //==�������ȼ�����=
            OutputDebugString(_T("û���ҵ��κ��ļ�"));
            FILEINFO finfo;
            finfo.HasNext = FALSE; //���ѭ����
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            if (CServerSocket::getInstance()->Send(pack) == false) {
                OutputDebugString(_T("������ʧ��"));
                return -4;
            }
        }
        //��ͨ����ҵ���һ���ļ�
        //����֤��һ���ļ���Ȼ��next
        int count = 0;
        do {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name)); //��fdata���������ָ��Ƹ�finfo
            TRACE("%s\r\n", finfo.szFileName);
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            ////������ʱ
            //auto start = std::chrono::steady_clock::now();
            //while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1) {}
            //IstFileInfos.push_back(finfo);
            count++;
        } while (!_findnext(hfind, &fdata));
        //������Ϣ�����ƶ�
        //���⣺�е�Ŀ¼�¿�����10���ļ������ѭ����ǳ���
        FILEINFO finfo;
        finfo.HasNext = FALSE; //���ѭ����
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        TRACE("������%d����Ŀ", count);
        if (CServerSocket::getInstance()->Send(pack) == false) {
            OutputDebugString(_T("������ʧ��"));
            return -4;
        }
        return 0;
    }

    int RunFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        CPacket pack(3, NULL, 0);
        if (CServerSocket::getInstance()->Send(pack) == false) {
            OutputDebugString(_T("������ʧ��"));
            return -4;
        }
        return 0;
    }

#pragma warning(disable:4966) //�ή��fopen, sprintf, strcpy, strstr�Ⱥ����İ�ȫ���Ѵ�error��warning
    int DownloadFile() { //���ƶ�Ҫ���أ�����˽����ϴ�
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        long long data = 0;  //���ڷ��ļ��ĳ���

        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        //FILE* pFile = fopen(strPath.c_str(), "rb"); //����Ϊ��ƽ̨�������ǰ�ȫ�����warning��������error��ԭ��
        //fopen_s: ����֤��open�������ֶ�����̶�ͬһ���ļ�ʱ�����ܵõ������Ϊ�յ��Ƕ��������ݵ����
        if (err != 0) {
            CPacket pack(4, (BYTE*)&data, 8); //���߶Է��ļ�����Ϊ0���൱�ڶ�����
            if (CServerSocket::getInstance()->Send(pack) == false) {
                OutputDebugString(_T("������ʧ��"));
                return -4;
            }
            return -1;
        }
        if (pFile != NULL) {
            fseek(pFile, 0, SEEK_END); //���ļ�ָ�����һ��ƫ��
            data = _ftelli64(pFile); //���ļ��ĳ���
            CPacket head(4, (BYTE*)&data, 8);
            CServerSocket::getInstance()->Send(head); //ԭ��������head��ȥ��
            fseek(pFile, 0, SEEK_SET); //���ļ�ָ��ƫ�����ûؿ�ͷ

            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);
                CPacket pack(4, (BYTE*)buffer, rlen);
                CServerSocket::getInstance()->Send(pack);
            } while (rlen >= 1024); //С�ڵĻ�˵������β����

            fclose(pFile);
        }
        CPacket pack(4, NULL, 0); //���ͻ�ȥһ����Ϣ����ʾ�����ˣ�����Ҳ�ܸ����ļ��ĳ����ж��ļ��Ƿ�����
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int MouseEvent() {
        MOUSEEV mouse;
        if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {

            DWORD nFlag = 0; //����λ��Ǽ�������λ��Ƕ���

            //ʹ�ñ������ֹ�����Ƕ��
            switch (mouse.nButton) {
            case 0: //���
                nFlag = 1;
                break;
            case 1: //�Ҽ�
                nFlag = 2;
                break;

            case 2: //�н�
                nFlag = 4;
                break;
            case 3:  //û�а�
                nFlag = 8;
                break;
            }

            if (nFlag != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); //�а���ʱ��������λ��
            switch (mouse.nAction) {
            case 0: //����
                nFlag |= 0x10;
                break;
            case 1: //˫��
                nFlag |= 0x20;
                break;
            case 2: //����
                nFlag |= 0x40;
                break;

            case 3: //�ſ�
                nFlag |= 0x80;
                break;
            default:
                break;
            }
            TRACE("mouse event: : %08X x %d y %d\r\n", nFlag, mouse.ptXY.x, mouse.ptXY.y);
            switch (nFlag) {
            case 0x21: //���˫��
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                //�����˫���Ļ��Ͳ���break��ֱ�ӵ�����ĵ�����ȥ�����ٴ�������
            case 0x11: //�������
                //GetMessageExtraInfo:��ȡ��ǰ�̵߳Ķ���������Ϣ
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo()); //����
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); //����
                break;

            case 0x41: //�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;

            case 0x22: //�Ҽ�˫��
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo()); //���º���
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12: //�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo()); //���º���
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;

            case 0x42: //�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82: //�Ҽ��ſ�
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;

            case 0x24: //�м�˫��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo()); //���º���
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14: //�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo()); //���º���
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;

            case 0x44: //�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84: //�м��ſ�
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08: //����ƶ�
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
            }
            CPacket pack(4, NULL, 0);
            CServerSocket::getInstance()->Send(pack);
        }
        else {
            OutputDebugString(_T("��ȡ����������ʧ��"));
            return -1;
        }
        return 0;
    }

    int SendScreen() {
        CImage screen;
        HDC hScreen = ::GetDC(NULL); //�õ��豸������,���ظ���Ļ���
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); //�ö��ٸ�bit��ʾ��ɫ
        int nWidth = GetDeviceCaps(hScreen, HORZRES); //ˮƽ��
        int nHeight = GetDeviceCaps(hScreen, VERTRES); //��ֱ���õ���
        screen.Create(nWidth, nHeight, nBitPerPixel); //����һ����ͼ
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hScreen);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL) return -1;
        IStream* pStream = NULL;
        //��Save���������ر��浽�ڴ���
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatPNG); //�����ݱ��浽�����������У�
            //stream������seek��������ָ��
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL); //�����Ƶ���ͷ
            PBYTE pData = (PBYTE)GlobalLock(hMem);

            SIZE_T nSize = GlobalSize(hMem);
            CPacket pack(6, pData, nSize);
            CServerSocket::getInstance()->Send(pack);
            GlobalUnlock(hMem);
        }
        pStream->Release();
        //DWORD tick = GetTickCount64(); //��õ�ǰtickֵ
        //screen.Save(_T("test1.png"), Gdiplus::ImageFormatPNG); //���浽�ļ���
        //TRACE("png %d\r\n", GetTickCount64() - tick); //30-50ms�������Ǵ���������

        //tick = GetTickCount64();
        //screen.Save(_T("test1.jpg"), Gdiplus::ImageFormatJPEG);
        //TRACE("jpg %d\r\n", GetTickCount64() - tick); //16ms���쵫����Ҫ����
        screen.ReleaseDC(); //ע���ͷ������ģ�ע�͵������л�������������
        GlobalFree(hMem);
        return 0;
    }

};

