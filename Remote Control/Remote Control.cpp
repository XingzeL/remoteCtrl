// Remote Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote Control.h"
#include "ServerSocket.h"
#include <direct.h>

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
    //CServerSocket::getInstance()->Send(pack);
    return 0;
    //_chdrive(1); //改变当前的驱动，如果能改变成功，说明驱动存在
}

#include <stdio.h>
#include <io.h>
#include <list>
typedef struct file_info{
    file_info() { //构造
        IsInvalid = FALSE;
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }

    BOOL IsInvalid; //是否是无效的文件比如链接，快捷方式
    char szFileName[256]; //名字
    BOOL IsDirectory; //是否是目录
    BOOL HasNext;
}FILEINFO, *PFILEINFO;
int MakeDirectoryInfo() {

    //获取指定文件夹的文件
    std::string strPath;
    std::list<FILEINFO> IstFileInfos; //链表搜集信息
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前命令不是获取文件信息，命令解析错误！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {
        //切换到目录失败
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
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
        return -3;
    }
    //用通配符找到第一个文件
    //先验证第一个文件，然后next
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; 
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name)); //把fdata的名字名字复制给finfo
        //IstFileInfos.push_back(finfo);
    } while (!_findnext(hfind, &fdata));
    //发送信息到控制端
    //问题：有的目录下可能有10万文件，这个循环会非常久
    FILEINFO finfo;
    finfo.HasNext = FALSE; //完成循环了
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    if (CServerSocket::getInstance()->Send(pack) == false) {
        OutputDebugString(_T("包发送失败"));
        return -4;
    }
    return 0;
}

int main()
{
    int nRetCode = 0;
    
    HMODULE hModule = ::GetModuleHandle(nullptr);
    //CServerSocket::m_helper; 私有的不可访问

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {   //1. 先做技术更难的一端，控制难度曲线得到更可控的进度先验预估
            // 整个项目进度的把握，与谁进行对接，进度的反馈形式 项目完成的预估 
            // TODO: socket, bind, listen, accept, read, write, close
            //套接字初始化：
            //CServerSocket* pserver = CServerSocket::getInstance(); //得到单例server对象
            //int count = 0;
            //if (pserver->InitSocket() == false) {
            //    MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //while (pserver) {
            int nCmd = 1;
            switch (nCmd) {
            case 1: //查看磁盘分区
                MakeDriverInfo();
                break;
            case 2: //查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            }
            
            //    if (pserver->AcceptClient() == false) {
            //        if (count >= 3) {
            //            MessageBox(NULL, _T("重试失败，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //        count++;
            //        //exit(0);
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO
            //}
           

            //全局静态变量：会在main调用之前被初始化，main结束后被释放；
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
