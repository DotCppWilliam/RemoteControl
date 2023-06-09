﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 设置入口点为 mainCRTStartup, 子系统设置为 windows.则没有窗口,在后台运行

//#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup") // 控制台
//#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")
//#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

// 唯一的应用程序对象

CWinApp theApp;

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
        {
            // 业务逻辑:
            CServerSocket* pserv = CServerSocket::getInstance();
            int count = 0;
            if (pserv->InitSocket() == false)
            {
                MessageBox(nullptr, _T("网络初始化异常,请检查网络状态"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(EXIT_FAILURE);
            }
			
            while (CServerSocket::getInstance() != nullptr)
            {
                if (pserv->AcceptClient() == false)
                {
                    if (count >= 3)
                    {
                        MessageBox(nullptr, _T("多次无法正常连接用户,结束进程"), _T("连接用户失败"), MB_OK | MB_ICONERROR);
                        exit(EXIT_FAILURE);
                    }
					MessageBox(nullptr, _T("无法连接用户,正在重试"), _T("连接用户失败"), MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pserv->DealCommand();
                // TODO:



            }
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
