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
            server;
            
            SOCKET serv_sock = socket(PF_INET, SOCK_STREAM, 0); //选择协议族：IPV4， stream：TCP协议
            //TODO: 进行socket的校验

            sockaddr_in serv_adr, client_adr;
            memset(&serv_adr, 0, sizeof(serv_adr));
            serv_adr.sin_family = AF_INET;
            serv_adr.sin_addr.s_addr = INADDR_ANY; //所有的IP
            serv_adr.sin_port = htons(9527);
            //为什么server有多个IP: 有的时候是对内工作，比如是数据库，由内网的设备进行连接；
            // 对外的IP地址将对外的服务暴露给外面，内网的带宽可以弄得很宽(用光纤) 外网带宽成本高
            bind(serv_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)); //注意serv_adr的转换要取地址 TODO:校验
            listen(serv_sock, 1);

            int cli_sz = sizeof(client_adr);
            accept(serv_sock, (sockaddr*)&client_adr, &cli_sz);
            char buffer[1024];
            recv(serv_sock, buffer, sizeof(buffer), 0);
            send(serv_sock, buffer, sizeof(buffer), 0);
            closesocket(serv_sock);

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
