// Remote Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote Control.h"
#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;
//server; ServerSocket.h中声明的外部变量server在main外直接使用会报错
using namespace std;

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
            CServerSocket* pserver = CServerSocket::getInstance(); //得到单例server对象
            int count = 0;
            while (pserver) {
                if (pserver->InitSocket() == false) {
                    MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                    exit(0);
                }
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("重试失败，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                    //exit(0);
                }
                int ret = pserver->DealCommand();
                //TODO
            }
           

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
