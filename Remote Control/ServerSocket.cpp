#include "pch.h"
#include "ServerSocket.h"

CServerSocket server; //声明的全局变量,会在主函数之前进行初始化
//这样的好处是在main函数之前是单线程，不会有竞争的风险产生
