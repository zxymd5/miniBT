#include "PeerLink.h"
#include <string.h>

CPeerLink::CPeerLink(void) :
		m_pPeerManager(NULL),
		m_llLastUnchokeTime(0)
{
}

CPeerLink::~CPeerLink(void)
{
}

void CPeerLink::SetPeerManager(IPeerManager *pManager)
{
	m_pPeerManager = pManager;
}

int CPeerLink::GetPeerState()
{
	return m_nPeerState;
}

void CPeerLink::Connect(const char *IpAddr, int nPort)
{
	CSocket::CreateTCPSocket();
	CSocket::SetReactor(m_pPeerManager->GetTorrentTask()->GetSocketReactor());

	m_strIPAddr = IpAddr;
	m_nPort = nPort;

	CSocket::SetHandleMask(WRITE_MASK);
	CSocket::Connect(IpAddr, nPort);
	m_nPeerState = PS_CONNECTING;

	m_nConnTimeoutID = GetReactor()->AddTimer(this, 10000, true);
}

void CPeerLink::CloseLink()
{
	HandleClose();
}

bool CPeerLink::IsAccepted()
{
	return m_bAccepted;
}

bool CPeerLink::ShouldClose()
{
	if (!m_bBitSetRecved)
	{
		return false;
	}

	if (m_pPeerManager->GetTorrentTask()->GetTaskStorage()->Finished())
	{
		if (m_clBitSet.IsAllSet())
		{
			return true;
		}
	}

	for (int i = 0; i < m_clBitSet.GetSize(); ++i)
	{
		if (m_clBitSet.IsSet(i)
				&& !m_pPeerManager->GetTorrentTask()->GetTaskStorage()->GetBitSet()->IsSet(
						i))
		{
			return false;
		}
	}

	return true;
}

void CPeerLink::ComputeSpeed()
{
	m_nUploadSpeed = (m_nUploadCount - m_nLastUploadCount) * 1000
			/ (GetTickCount() - m_llLastCountSpeedTime + 1);
	m_nDownloadSpeed = (m_nDownloadCount - m_nLastDownloadCount) * 1000
			/ (GetTickCount() - m_llLastCountSpeedTime + 1);

	m_llLastCountSpeedTime = GetTickCount();
	m_nLastUploadCount = m_nUploadCount;
	m_nLastDownloadCount = m_nDownloadCount;
}

void CPeerLink::OnTimer(int nTimerID)
{
	if (nTimerID == m_nConnTimeoutID)
	{
		m_nConnTimeoutID = 0;
		if (m_nPeerState == PS_CONNECTING)
		{
			CloseLink();
		}
	}
}

void CPeerLink::SetUploadSpeed(unsigned int nSpeed)
{
	m_nUploadSpeed = nSpeed;
}

void CPeerLink::SetDownloadSpeed(unsigned int nSpeed)
{
	m_nDownloadSpeed = nSpeed;
}

unsigned int CPeerLink::GetUploadSpeed()
{
	return m_nUploadSpeed;
}

unsigned int CPeerLink::GetDownloadSpeed()
{
	return m_nDownloadSpeed;
}

void CPeerLink::SendData(const void *pData, int nLen)
{
	m_strSendBuffer.append((const char *) pData, nLen);
}

int CPeerLink::ProcRecvData()
{
	if (!m_bHandShaked)
	{
		if (m_strRecvBuffer.size() >= 68)
		{
			string strHandShake = m_strRecvBuffer.substr(0, 68);
			m_strRecvBuffer.erase(0, 68);
			if (!CheckHandshake(strHandShake))
			{
				return -1;
			}
		}
	}
	else
	{
		for (; m_strRecvBuffer.size() >= 4;)
		{
			unsigned int nLen = *((unsigned int*) m_strRecvBuffer.data());
			nLen = ntohl(nLen);

			if (nLen == 0)
			{
				m_strRecvBuffer.erase(0, 4);
				continue;
			}

			if (m_strRecvBuffer.size() < 5)
			{
				break;
			}

			unsigned char nCmd = m_strRecvBuffer[4];
			if (m_strRecvBuffer.size() >= nLen + 4)
			{
				if (-1
						== ProcCmd(nCmd, (void *) (m_strRecvBuffer.data() + 5),
								nLen - 1))
				{
					return -1;
				}

				m_strRecvBuffer.erase(0, nLen + 4);
			}
			else
			{
				break;
			}

		}
	}

	return 0;
}

void CPeerLink::SendHandShake()
{
	char szBuff[68];
	memset(szBuff, 0, sizeof(szBuff));

	szBuff[0] = 19;
	strncpy(szBuff + 1, "BitTorrent protocol", 19);
	strncpy(szBuff + 28,
			(const char *) m_pPeerManager->GetTorrentTask()->GetTorrentFile()->GetInfoHash(),
			20);
	strncpy(szBuff + 48,
			(const char *) m_pPeerManager->GetTorrentTask()->GetPeerID().c_str(),
			20);

	SendData(szBuff, sizeof(szBuff));
}

void CPeerLink::SendBitField()
{
	string strBitField =
			m_pPeerManager->GetTorrentTask()->GetTaskStorage()->GetBitField();
	if (strBitField.size() == 0)
	{
		return;
	}

	string strBuff;
	char szHeader[5];
	*((int *) (szHeader)) = htonl(1 + strBitField.size());
	*((char*) (szHeader + 4)) = 5;

	strBuff.append(szHeader, 5);
	strBuff.append(strBitField.data(), strBitField.size());

	SendData(strBuff.data(), strBuff.size());
}

void CPeerLink::SendChoke(bool bChoke)
{
	char szBuff[5];
	*((int *) szBuff) = htonl(1);
	m_bAmChoking = bChoke;
	*((char *) (szBuff + 4)) = bChoke ? 0 : 1;

	m_llLastUnchokeTime = (bChoke ? GetTickCount() : m_llLastUnchokeTime);

	SendData(szBuff, sizeof(szBuff));
}

void CPeerLink::SendInterested(bool bInterested)
{
	char szBuff[5];
	*((int *) szBuff) = htonl(1);
	m_bAmInterested = bInterested;
	*((char *) (szBuff + 4)) = bInterested ? 2 : 3;

	SendData(szBuff, sizeof(szBuff));
}

void CPeerLink::SendHave(int nPieceIndex)
{
	char szBuff[9];
	*((unsigned int *) szBuff) = htonl(5);
	*((unsigned char *) (szBuff + 4)) = 4;
	*((unsigned int *) (szBuff + 5)) = htonl(nPieceIndex);

	SendData(szBuff, sizeof(szBuff));
}

void CPeerLink::SendPieceRequest(int nPieceIndex, int nOffset, int nLen)
{
	char szBuff[17];
	*((unsigned int *) szBuff) = htonl(13);
	*((unsigned char *) (szBuff + 4)) = 6;
	*((unsigned int *) (szBuff + 5)) = htonl(nPieceIndex);
	*((unsigned int *) (szBuff + 9)) = htonl(nOffset);
	*((unsigned int *) (szBuff + 13)) = htonl(nLen);

	SendData(szBuff, sizeof(szBuff));
}

void CPeerLink::SendPieceData(int nPieceIndex, int nOffset, string &strData)
{
	char szBuff[13];
	*((unsigned int *) szBuff) = htonl(9 + strData.size());
	*((unsigned char *) (szBuff + 4)) = 7;
	*((unsigned int *) (szBuff + 5)) = htonl(nPieceIndex);
	*((unsigned int *) (szBuff + 9)) = htonl(nOffset);

	string strMsgContent;
	strMsgContent.append((const char *) szBuff, sizeof(szBuff));
	strMsgContent.append(strData.c_str(), strData.size());

	SendData(strMsgContent.data(), strMsgContent.size());
}

void CPeerLink::SendPieceCancel(unsigned int nPieceIndex, unsigned int nOffset,
		unsigned int nLen)
{
	char szBuff[17];
	*((unsigned int *) szBuff) = htonl(13);
	*((unsigned char *) (szBuff + 4)) = 8;
	*((unsigned int *) (szBuff + 5)) = htonl(nPieceIndex);
	*((unsigned int *) (szBuff + 9)) = htonl(nOffset);
	*((unsigned int *) (szBuff + 13)) = htonl(nLen);

	SendData(szBuff, sizeof(szBuff));
}

bool CPeerLink::CheckHandshake(string strInfo)
{
	if (strInfo[0] != 19)
	{
		return false;
	}

	string strProtocol = strInfo.substr(1, 19);
	if (strProtocol != "BitTorrent protocol")
	{
		return false;
	}

	string strInfoHash = strInfo.substr(28, 20);
	string strMyInfoHash;
	strMyInfoHash.append(
			(const char*) m_pPeerManager->GetTorrentTask()->GetTorrentFile()->GetInfoHash(),
			20);

	if (strInfoHash != strMyInfoHash)
	{
		return false;
	}

	m_strPeerLinkID = strInfo.substr(48, 20);
	m_bHandShaked = true;

	SendBitField();
	SendInterested(true);

	m_clBitSet.Alloc(
			m_pPeerManager->GetTorrentTask()->GetTorrentFile()->GetPieceCount());

	m_llLastUnchokeTime = GetTickCount();
	return true;
}

int CPeerLink::ProcCmd(int nCmd, void *pData, int nDataLen)
{
	switch (nCmd)
	{
	case 0:
		if (ProcCmdChoke(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 1:
		if (ProcCmdUnchoke(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 2:
		if (ProcCmdInterested(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 3:
		if (ProcCmdNotInterested(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 4:
		if (ProcCmdHave(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 5:
		if (ProcCmdBitfield(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 6:
		if (ProcCmdRequest(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 7:
		if (ProcCmdPiece(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	case 8:
		if (ProcCmdCancel(pData, nDataLen) == -1)
		{
			return -1;
		}
		break;
	}

	return 0;
}

int CPeerLink::ProcCmdChoke(void *pData, int nDataLen)
{
	m_bPeerChoking = true;
	m_clPieceRequest.ClearRequest();

	return 0;
}

int CPeerLink::ProcCmdUnchoke(void *pData, int nDataLen)
{
	m_bPeerChoking = false;
	GetNewPieceTask();
	DoPieceRequest();

	return 0;
}

int CPeerLink::ProcCmdInterested(void *pData, int nDataLen)
{
	m_bPeerInterested = true;
	return 0;
}

int CPeerLink::ProcCmdNotInterested(void *pData, int nDataLen)
{
	m_bPeerInterested = false;
	return 0;
}

void CPeerLink::OnConnect()
{
	SetHandleMask(READ_MASK);

	m_strSendBuffer.clear();
	m_strRecvBuffer.clear();
	m_bHandShaked = false;

	m_clPieceRequest.CreatePieceRequestList(-1, 0, 0);

	m_bAmChoking = true;
	m_bAmInterested = false;
	m_bPeerChoking = true;
	m_bPeerInterested = false;

	m_nDownloadCount = 0;
	m_nUploadCount = 0;
	m_nLastDownloadCount = 0;
	m_nLastUploadCount = 0;
	m_nLastUploadCount = 0;
	m_nDownloadSpeed = 0;
	m_nUploadSpeed = 0;

	m_llLastCountSpeedTime = GetTickCount();

	m_bCanRead = false;
	m_bCanWrite = true;

	m_pPeerManager->GetTorrentTask()->GetRateMeasure()->AddClient(this);

	SendHandShake();
}

void CPeerLink::OnConnectFailed()
{

}

void CPeerLink::OnConnectClosed()
{
	m_pPeerManager->GetTorrentTask()->GetRateMeasure()->RemoveClient(this);

	m_pPeerManager->GetTorrentTask()->GetTaskStorage()->AbandonPieceTask(
			m_clPieceRequest.GetPieceIndex());
	m_clPieceRequest.CreatePieceRequestList(-1, 0, 0);
}

void CPeerLink::OnSendComplete()
{
	DoPieceSend();
}

void CPeerLink::OnDownloadComplete()
{
	SendInterested(false);
	m_clPieceRequest.CreatePieceRequestList(-1, 0, 0);
}

void CPeerLink::HandleClose()
{
	if (m_nPeerState == PS_CONNECTING)
	{
		m_nPeerState = PS_CONNECTFAILED;
		OnConnectFailed();
	}

	if (m_nPeerState == PS_ESTABLISHED)
	{
		m_nPeerState = PS_CLOSED;
		OnConnectClosed();
	}

	if (m_nConnTimeoutID != 0)
	{
		GetReactor()->RemoveTimer(m_nConnTimeoutID);
		m_nConnTimeoutID = 0;
	}

	m_bCanRead = false;
	m_bCanWrite = false;
	GetReactor()->RemoveSocket(this);
	SetReactor(NULL);
	CSocket::Close();
}

int CPeerLink::HandleRead()
{
	m_bCanRead = true;
	return 0;
}

int CPeerLink::HandleWrite()
{
	if (m_nPeerState == PS_CONNECTING)
	{
		m_nPeerState = PS_ESTABLISHED;
		RemoveHandleMask(WRITE_MASK);

		OnConnect();

		return 0;
	}

	m_bCanWrite = true;
	return 0;
}

void CPeerLink::GetNewPieceTask()
{
	if (!m_clPieceRequest.Complete())
	{
		return;
	}

	if (m_pPeerManager->GetTorrentTask()->GetTaskStorage()->Finished())
	{
		return;
	}

	if (m_bPeerChoking)
	{
		return;
	}

	int nPieceIndex =
			m_pPeerManager->GetTorrentTask()->GetTaskStorage()->GetPieceTask(
					&m_clBitSet);
	if (nPieceIndex != -1)
	{
		if (!m_bAmInterested)
		{
			SendInterested(true);
		}

		m_clPieceRequest.CreatePieceRequestList(nPieceIndex,
				m_pPeerManager->GetTorrentTask()->GetTaskStorage()->GetPieceLength(
						nPieceIndex), REQUEST_BLOCK_SIZE);
	}
	else
	{
		if (m_bAmInterested)
		{
			SendInterested(false);
		}
	}

}

void CPeerLink::DoPieceRequest()
{
	if (m_bPeerChoking)
	{
		return;
	}

	for (; m_clPieceRequest.GetPendingCount() < 5;)
	{
		unsigned int nIndex;
		unsigned int nOffset;
		unsigned int nLen;
		if (!m_clPieceRequest.GetRequest(nIndex, nOffset, nLen))
		{
			break;
		}

		SendPieceRequest(nIndex, nOffset, nLen);
	}
}

int CPeerLink::ProcCmdHave(void *pData, int nDataLen)
{
	if (nDataLen != 4)
	{
		return -1;
	}

	int nIndex = *((int *) pData);
	nIndex = ntohl(nIndex);
	m_clBitSet.Set(nIndex, true);

	if (!m_bPeerChoking)
	{
		GetNewPieceTask();
		DoPieceRequest();
	}

	return 0;
}

int CPeerLink::ProcCmdBitfield(void *pData, int nDataLen)
{
	string strBitset;
	strBitset.append((const char *) pData, nDataLen);
	m_clBitSet.Alloc(strBitset,
			m_pPeerManager->GetTorrentTask()->GetTorrentFile()->GetPieceCount());

//     string strLog;
//     for (int i = 0; i < m_clBitSet.GetSize(); ++i)
//     {
//         if (m_clBitSet.IsSet(i))
//         {
//             strLog.append("1");
//         }
//         else
//         {
//             strLog.append("0");
//         }
//     }
	m_bBitSetRecved = true;

	return 0;
}

int CPeerLink::ProcCmdRequest(void *pData, int nDataLen)
{
	if (nDataLen != 12)
	{
		return -1;
	}

	if (m_bAmChoking)
	{
		return 0;
	}

	int nIndex = *((int *) pData);
	nIndex = ntohl(nIndex);
	int nOffset = *((int *) ((char *) pData + 4));
	nOffset = ntohl(nOffset);
	int nLen = *((int *) ((char *) pData + 8));
	nLen = ntohl(nLen);

	list<PeerPieceRequest>::iterator it = m_lstPeerPieceRequest.begin();
	for (; it != m_lstPeerPieceRequest.end(); ++it)
	{
		if (it->nIndex == nIndex && it->nOffset == nOffset)
		{
			break;
		}
	}

	if (it == m_lstPeerPieceRequest.end())
	{
		PeerPieceRequest stRequst;
		stRequst.nIndex = nIndex;
		stRequst.nOffset = nOffset;
		stRequst.nLen = nLen;

		m_lstPeerPieceRequest.push_back(stRequst);
	}

	DoPieceSend();

	return 0;
}

int CPeerLink::ProcCmdPiece(void *pData, int nDataLen)
{
	if (nDataLen <= 8)
	{
		return -1;
	}

	int nIndex = *(int *) pData;
	nIndex = ntohl(nIndex);
	int nOffset = *((int *) ((char *) pData + 4));
	nOffset = ntohl(nOffset);

	int nLen = nDataLen - 8;
	if (nIndex != m_clPieceRequest.GetPieceIndex())
	{
		return -1;
	}

	if (!m_clPieceRequest.AddPieceData(nOffset, (const char *) pData + 8, nLen))
	{
		return -1;
	}

	if (m_clPieceRequest.Complete())
	{
		string strPieceData = m_clPieceRequest.GetPiece();

		if (ShaString((char *) strPieceData.data(), strPieceData.size())
				== m_pPeerManager->GetTorrentTask()->GetTorrentFile()->GetPieceHash(
						nIndex))
		{
			m_pPeerManager->GetTorrentTask()->GetTaskStorage()->WritePiece(
					nIndex, strPieceData);
			m_pPeerManager->BroadcastHave(nIndex);
			GetNewPieceTask();
		}
		else
		{
			m_clPieceRequest.CreatePieceRequestList(-1, 0, 0);
			CloseLink();
		}
	}

	DoPieceRequest();
	return 0;
}

int CPeerLink::ProcCmdCancel(void *pData, int nDataLen)
{
	if (nDataLen != 12)
	{
		return -1;
	}

	unsigned int nIndex = *((unsigned int*) pData);
	nIndex = ntohl(nIndex);

	unsigned int nOffset = *((unsigned int *) ((char *) pData + 4));
	nOffset = ntohl(nOffset);

	unsigned int nLen = *((unsigned int *) ((char *) pData + 8));
	nLen = ntohl(nLen);

	list<PeerPieceRequest>::iterator it = m_lstPeerPieceRequest.begin();
	for (; it != m_lstPeerPieceRequest.end(); ++it)
	{
		if (it->nIndex == nIndex && it->nOffset == nOffset)
		{
			m_lstPeerPieceRequest.erase(it);
			break;
		}
	}

	return 0;
}

int CPeerLink::SendKeepAlive()
{
	int nSendCount = 0;

	return nSendCount;
}

long long CPeerLink::GetLastUnchokeTime()
{
	return m_llLastUnchokeTime;
}

bool CPeerLink::IsEmpty()
{
	return m_clBitSet.IsEmpty();
}

void CPeerLink::DoPieceSend()
{
	if (!m_bAmChoking && m_strSendBuffer.size() == 0
			&& m_lstPeerPieceRequest.size() > 0)
	{
		PeerPieceRequest stRequest = m_lstPeerPieceRequest.front();
		m_lstPeerPieceRequest.pop_front();

		string strPieceData =
				m_pPeerManager->GetTorrentTask()->GetTaskStorage()->ReadData(
						stRequest.nIndex, stRequest.nOffset, stRequest.nLen);

		if (strPieceData.size() == stRequest.nLen)
		{
			SendPieceData(stRequest.nIndex, stRequest.nOffset, strPieceData);
		}
	}
}

void CPeerLink::NotifyHavePiece(int nPieceIndex)
{
	if (m_bHandShaked)
	{
		if (m_clPieceRequest.GetPieceIndex() == nPieceIndex)
		{
			CancelPieceRequest(nPieceIndex);
		}
		SendHave(nPieceIndex);
	}
}

void CPeerLink::CancelPieceRequest(int nPieceIndex)
{
	if (m_clPieceRequest.GetPieceIndex() == nPieceIndex)
	{
		unsigned int nIndex;
		unsigned int nOffset;
		unsigned int nLen;

		while (m_clPieceRequest.CancelPendingRequest(nIndex, nOffset, nLen))
		{
			SendPieceCancel(nIndex, nOffset, nLen);
		}

		m_clPieceRequest.CreatePieceRequestList(-1, 0, 0);

		if (!m_bPeerChoking)
		{
			GetNewPieceTask();
			DoPieceRequest();
		}
	}
}

void CPeerLink::BlockRead(bool bBlock)
{
	if (bBlock && (GetHandleMask() & READ_MASK))
	{
		RemoveHandleMask(READ_MASK);
		return;
	}

	if (!bBlock && !(GetHandleMask() & READ_MASK))
	{
		SetHandleMask(READ_MASK);
	}
}

void CPeerLink::BlockWrite(bool bBlock)
{
	if (!bBlock)
	{
		if (m_strSendBuffer.size() > 0 && !m_bCanWrite)
		{
			if (!(GetHandleMask() & WRITE_MASK))
			{
				SetHandleMask(WRITE_MASK);
			}
		}
		return;
	}

	if (bBlock && (GetHandleMask() & WRITE_MASK))
	{
		RemoveHandleMask(WRITE_MASK);
	}
}

void CPeerLink::SetWritePriority(int nPriority)
{
	m_nWritePriority = nPriority;
}

int CPeerLink::GetWritePriority()
{
	return m_nWritePriority;
}

void CPeerLink::SetReadPriority(int nPriority)
{
	m_nReadPriority = nPriority;
}

int CPeerLink::GetReadPriority()
{
	return m_nReadPriority;
}

bool CPeerLink::CanWrite()
{
	return m_bCanWrite && (m_strSendBuffer.size() > 0);
}

bool CPeerLink::CanRead()
{
	return m_bCanRead;
}

int CPeerLink::DoWrite(long long llCount)
{
	int nSendCount = 0;
	if (m_nPeerState == PS_ESTABLISHED)
	{
		for (; (m_strSendBuffer.size() > 0) && (nSendCount < llCount);)
		{
			int nSendSize = m_strSendBuffer.size();
			if (nSendSize > llCount)
			{
				nSendSize = llCount;
			}

			int nRet = send(GetHandle(), m_strSendBuffer.data(), nSendSize, 0);

			if (nRet == -1)
			{
				if (errno == EAGAIN || errno == EINTR)
				{
					m_bCanWrite = false;
				}
				else
				{
					CloseLink();
					return nSendCount;
				}

				break;
			}

			nSendCount += nRet;
			m_nUploadCount += nRet;
			m_pPeerManager->GetTorrentTask()->AddUploadCount(nRet);

			m_strSendBuffer.erase(0, nRet);
			if (m_strSendBuffer.size() == 0)
			{
				OnSendComplete();
				break;
			}
		}
	}

	if (m_bCanWrite)
	{
		if (GetHandleMask() & WRITE_MASK)
		{
			RemoveHandleMask(WRITE_MASK);
		}
	}
	else
	{
		if (!(GetHandleMask() & WRITE_MASK))
		{
			SetHandleMask(WRITE_MASK);
		}
	}

	return nSendCount;
}

int CPeerLink::DoRead(long long llCount)
{
	int nReadCount = 0;
	char *pBuff = new char[RECV_BUFFER_SIZE];

	for (; nReadCount < llCount;)
	{
		int nReadSize = RECV_BUFFER_SIZE;
		if (nReadSize > llCount - nReadCount)
		{
			nReadSize = llCount - nReadCount;
		}

		int nRet = recv(GetHandle(), pBuff, nReadSize, 0);
		if (nRet == 0)
		{
			CloseLink();
			delete[] pBuff;
			return nReadCount;
		}

		if (nRet == -1)
		{
			if (errno == EAGAIN)
			{
				m_bCanRead = false;
				break;
			}

			if (errno == EINTR)
			{
				continue;
			}

			CloseLink();
			delete[] pBuff;
			return nReadCount;
		}

		if (nRet > 0)
		{
			nReadCount += nRet;
			m_nDownloadCount += nRet;
			m_pPeerManager->GetTorrentTask()->AddDownloadCount(nRet);

			m_strRecvBuffer.append((const char *) pBuff, nRet);
		}
	}

	delete[] pBuff;

	ProcRecvData();

	if (m_bCanRead)
	{
		if (GetHandleMask() & READ_MASK)
		{
			RemoveHandleMask(READ_MASK);
		}
	}
	else
	{
		if (!(GetHandleMask() & READ_MASK))
		{
			SetHandleMask(READ_MASK);
		}
	}

	return nReadCount;
}

void CPeerLink::Attach(int nHandle, const char *IpAddr, int nPort,
		IPeerManager *pManager)
{
	m_strIPAddr = IpAddr;
	m_nPort = nPort;
	m_bAccepted = true;
	m_nPeerState = PS_ESTABLISHED;
	m_nConnTimeoutID = 0;
	CSocket::Attach(nHandle);
	SetPeerManager(pManager);
	CSocket::SetReactor(m_pPeerManager->GetTorrentTask()->GetSocketReactor());
	OnConnect();
}

bool CPeerLink::PeerChoked()
{
	return m_bAmChoking;
}

bool CPeerLink::PeerInterested()
{
	return m_bPeerInterested;
}

void CPeerLink::ChokePeer(bool bChoke)
{
	if (bChoke)
	{
		m_lstPeerPieceRequest.clear();
	}
	SendChoke(bChoke);
}

unsigned int CPeerLink::GetDownloadCount()
{
	return m_nDownloadCount;
}

unsigned int CPeerLink::GetUploadCount()
{
	return m_nUploadCount;
}

bool CPeerLink::NeedRemoteData()
{
	bool bNeed = false;

	int nPieceCount =
			m_pPeerManager->GetTorrentTask()->GetTorrentFile()->GetPieceCount();
	IBitSet *pBitSet =
			m_pPeerManager->GetTorrentTask()->GetTaskStorage()->GetBitSet();

	if (pBitSet != NULL)
	{
		for (int i = 0; i < nPieceCount; ++i)
		{
			if (!pBitSet->IsSet(i) && m_clBitSet.IsSet(i))
			{
				bNeed = true;
			}
		}
	}
	return bNeed;
}
