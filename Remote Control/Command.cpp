#include "pch.h"
#include "Command.h"

CCommand::CCommand() :threadId(0)
{
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1, &CCommand::MakeDriverInfo}, //注意&
		{2, &CCommand::MakeDirectoryInfo},
		{3, &CCommand::RunFile},
		{4, &CCommand::DownloadFile},
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen},
		{7, &CCommand::LockMachine},
		{8, &CCommand::UnLockMachine},
		{9, &CCommand::DeleteLocalFile},
		{1981, &CCommand::TestConncet},
		{-1, NULL}
	};

	for (int i = 0; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
		//映射表未来可变，想加命令的时候就往data里面加项目,switch最多256个，而且也是用的汇编的if-else
	}
}

int CCommand::ExecuteCommand(int nCmd)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}

	return (this->*it->second)(); //这里没有this不行
}
