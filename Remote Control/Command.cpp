#include "pch.h"
#include "Command.h"

CCommand::CCommand() :threadId(0)
{
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1, &CCommand::MakeDriverInfo}, //ע��&
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
		//ӳ���δ���ɱ䣬��������ʱ�����data�������Ŀ,switch���256��������Ҳ���õĻ���if-else
	}
}

int CCommand::ExecuteCommand(int nCmd, std::list<CPacket>& lsPacket, CPacket& inPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}

	return (this->*it->second)(lsPacket, inPacket); //����û��this����,Ϊʲô
}