﻿// Remote Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote Control.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>
#include <chrono>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;
//server; ServerSocket.h中声明的外部变量server在main外直接使用会报错
using namespace std;
void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() {
    //创建磁盘分区的信息
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size() > 0) {
                //前面已经有内容了
                result += ',';
            }
            result += 'A' + i - 1;
        }
    }
    //CServerSocket::getInstance()->Send(CPacket(1, (BYTE*)result.c_str(), result.size())); //这个分支中，socket还美哟初始化
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    //Dump((BYTE*)&pack, pack.nLength + 6); //(BYTE*)&pack这种取数据的方式有问题，因为pack是一个对象，取地址不会得到里面的内容
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
    //_chdrive(1); //改变当前的驱动，如果能改变成功，说明驱动存在
}

#include <stdio.h>
#include <io.h>
#include <list>

int MakeDirectoryInfo() {

    //获取指定文件夹的文件
    std::string strPath;
    //std::list<FILEINFO> IstFileInfos; //链表搜集信息
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前命令不是获取文件信息，命令解析错误！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {
       // OutputDebugString(_T(strPath.c_str());
        //切换到目录失败
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = FALSE;
        finfo.HasNext = FALSE;
        //memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //IstFileInfos.push_back(finfo);　//从一起搜集一起打包到一个一个将文件信息发送走
        CPacket pack(2, (BYTE*) & finfo, sizeof(finfo));
        if (CServerSocket::getInstance()->Send(pack) == false) {
            OutputDebugString(_T("包发送失败"));
            return -4;
        } 

        OutputDebugString(_T("没有权限访问目录"));
        return -2;
    }

    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) { //==符号优先级大于=
        OutputDebugString(_T("没有找到任何文件"));
        FILEINFO finfo;
        finfo.HasNext = FALSE; //完成循环了
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        if (CServerSocket::getInstance()->Send(pack) == false) {
            OutputDebugString(_T("包发送失败"));
            return -4;
        }
    }
    //用通配符找到第一个文件
    //先验证第一个文件，然后next
    int count = 0;
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; 
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name)); //把fdata的名字名字复制给finfo
        TRACE("%s\r\n", finfo.szFileName);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        ////进行延时
        //auto start = std::chrono::steady_clock::now();
        //while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1) {}
        //IstFileInfos.push_back(finfo);
        count++;
    } while (!_findnext(hfind, &fdata));
    //发送信息到控制端
    //问题：有的目录下可能有10万文件，这个循环会非常久
    FILEINFO finfo;
    finfo.HasNext = FALSE; //完成循环了
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    TRACE("发送了%d个项目", count);
    if (CServerSocket::getInstance()->Send(pack) == false) {
        OutputDebugString(_T("包发送失败"));
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
        OutputDebugString(_T("包发送失败"));
        return -4;
    }
    return 0;
}

#pragma warning(disable:4966) //会降级fopen, sprintf, strcpy, strstr等函数的安全提醒从error到warning
int DownloadFile() { //控制端要下载，服务端进行上传
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath); 
    long long data = 0;  //用于放文件的长度

    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    //FILE* pFile = fopen(strPath.c_str(), "rb"); //会因为跨平台而报错，是安全问题的warning被提升成error的原因
    //fopen_s: 待验证的open，当出现多个进程读同一个文件时，可能得到句柄不为空但是读不到数据的情况
    if (err != 0) {
        CPacket pack(4, (BYTE*)&data, 8); //告诉对方文件长度为0，相当于读不了
        if (CServerSocket::getInstance()->Send(pack) == false) {
            OutputDebugString(_T("包发送失败"));
            return -4;
        }
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END); //将文件指针进行一次偏移
        data = _ftelli64(pFile); //拿文件的长度
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head); //原来忘记送head出去了
        fseek(pFile, 0, SEEK_SET); //将文件指针偏移重置回开头

        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024); //小于的话说明读到尾巴了

        fclose(pFile);
    }
    CPacket pack(4, NULL, 0); //发送回去一个消息，表示读完了，对面也能根据文件的长度判断文件是否完整
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        
        DWORD nFlag = 0; //低四位标记键，高四位标记动作

        //使用标记来防止情况的嵌套
        switch (mouse.nButton) {
        case 0: //左键
            nFlag = 1;
            break;
        case 1: //右键
            nFlag = 2;
            break;

        case 2: //中健
            nFlag = 4;
            break;
        case 3:  //没有按
            nFlag = 8; 
            break;
        }

        if (nFlag != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); //有按键时设置鼠标的位置
        switch (mouse.nAction) {
        case 0: //单击
            nFlag |= 0x10;
            break;
        case 1:
            nFlag |= 0x20;
            break;
        case 2: //按下
            nFlag |= 0x40;
            break;

        case 3: //放开
            nFlag |= 0x80;
            break;
        default: 
            break;
        }
        switch (nFlag) {
        case 0x21: //左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            //如果是双击的话就不用break，直接到下面的单击中去，减少代码行数
        case 0x11: //左键单击
            //GetMessageExtraInfo:获取当前线程的额外外设信息
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo()); //按下
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); //弹起
            break;

        case 0x41: //左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22: //右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo()); //按下后弹起
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12: //右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo()); //按下后弹起
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x42: //右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82: //右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24: //中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo()); //按下后弹起
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14: //中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo()); //按下后弹起
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x44: //中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84: //中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08: //鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break; 
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败"));
        return -1;
    }
    return 0;
}

int SendScreen() {
    CImage screen;
    HDC hScreen = ::GetDC(NULL); //得到设备上下文,返回给屏幕句柄
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); //用多少个bit表示颜色
    int nWidth = GetDeviceCaps(hScreen, HORZRES); //水平的
    int nHeight = GetDeviceCaps(hScreen, VERTRES); //垂直，拿到高
    screen.Create(nWidth, nHeight, nBitPerPixel); //创建一个截图
    BitBlt(screen.GetDC(), 0, 0, 1920, 1080, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); 
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;
    //用Save的流的重载保存到内存中
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG); //将内容保存到流（缓冲区中）
        //stream可以用seek来调整流指针
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL); //重新移到开头
        PBYTE pData = (PBYTE)GlobalLock(hMem);

        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }
    pStream->Release();
    //DWORD tick = GetTickCount64(); //获得当前tick值
    //screen.Save(_T("test1.png"), Gdiplus::ImageFormatPNG); //保存到文件中
    //TRACE("png %d\r\n", GetTickCount64() - tick); //30-50ms，慢但是带宽需求少

    //tick = GetTickCount64();
    //screen.Save(_T("test1.jpg"), Gdiplus::ImageFormatJPEG);
    //TRACE("jpg %d\r\n", GetTickCount64() - tick); //16ms，快但是需要带宽
    screen.ReleaseDC(); //注意释放上下文，注释掉后运行会在析构处报错
    GlobalFree(hMem);
    return 0;
}

#include "LockDialog.h"
CLockDialog dig;
unsigned threadId = 0;

unsigned __stdcall threadLockDlg(void* arg) {
    TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    dig.Create(IDD_DIALOG_INFO, NULL); //创建窗口
    dig.ShowWindow(SW_SHOW);

    //窗口制定，遮蔽后面的内容
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN) / 2;
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) / 2;
    dig.MoveWindow(rect);
    dig.SetWindowPos(&dig.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //没有消息循环的话会立马消失

    TRACE("right = %d bottom = %d/r/n", rect.right, rect.bottom);
    //dig.GetWindowRect(rect); //获取窗口范围

    ShowCursor(false); //在窗口内就不会出现鼠标
    //::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE); //隐藏任务栏
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    //ClipCursor(rect); //将鼠标限制在窗口范围内，只有一个像素点

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { //对话框依赖消息循环
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN) { //接收到按键就释放
            TRACE("msg::%08X wparam:%08x lparam:%08X/r/n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x1B) { //是esc键的时候
                break;
            }

        }
    }
    dig.DestroyWindow(); //和上面的Create成对
    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); //恢复任务栏
    _endthreadex(0);
    return 0;
}

int LockMachine() { 
    //需要弹出一个消息：“请联系管理员” 显示在最顶层
    //模态还是非模态 没有模态基础，因为隐藏了命令框

    //如果是空或者无效就开启线程
    if ((dig.m_hWnd == NULL) || (dig.m_hWnd == INVALID_HANDLE_VALUE)) {
        //_beginthread(threadLockDlg, 0, NULL); //启动一个新的线程，自己不阻塞，不然就收不到别的消息了
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadId);
        TRACE("threadId = %d\r\n", threadId);
    }

    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnLockMachine() {
    //dig.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);  //给对象送消息
    //::SendMessage(dig.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001); //全局送消息
    //！：以上两种送消息的方式都不会被另一个线程收到
    PostThreadMessage(threadId, WM_KEYDOWN, 0x1B, 0); //需要threadID
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TestConncet() {
    CPacket pack(1981, NULL, 0);
    
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("服务端发送ret: %d\r\n",ret);
    return 0;
}

int DeleteLocalFile() {
    
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath); //得到文件路径
    TCHAR sPath[MAX_PATH] = _T("");
    //mbstowcs(sPath, strPath.c_str(), strPath.size()); //中文可能会变成乱码导致删除不了文件
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(),
        sPath, sizeof(sPath) / sizeof(TCHAR));
    DeleteFile(sPath);
    //返回响应
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("服务端发送ret: %d\r\n", ret);
    return 0;
}

int ExecuteCommd(int nCmd) {
    TRACE("into Execute Commadn");
    int ret = 0;
    switch (nCmd) {
    case 1: //查看磁盘分区
        ret = MakeDriverInfo();
        break;
    case 2: //查看指定目录下的文件
        ret = MakeDirectoryInfo();
        break;
    case 3:
        ret = RunFile();
        break;
    case 4:
        ret = DownloadFile();
        break;
    case 5: //鼠标操作
        ret = MouseEvent();
        break;
    case 6: //发送屏幕内容 本质：发送屏幕的截图
        ret = SendScreen();
        break;
    case 7: //锁机
        ret = LockMachine();
        break;

    case 8:
        ret = UnLockMachine();
        break;
    case 9: //删除文件
        ret = DeleteLocalFile();
    case 1981:
        ret = TestConncet();
        break;
        
    }

    return ret;
}

int main() {
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr) {
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else {
            CServerSocket* pserver = CServerSocket::getInstance();
            int count = 0;
            if (pserver->InitSocket() == false) {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (pserver) {
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法正常进入用户，结束程序！"), _T("网络初始化失败"), \
                            MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), \
                        MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pserver->DealCommand();
                if (ret != -1) {
                    ret = ExecuteCommd(pserver->GetPacket().sCmd);
                    if (ret != 0) {
                        TRACE("执行命令失败，%d ret = %d\r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                    TRACE("Command has done\r\n");
                }
            }
        }
    }
    else {
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
