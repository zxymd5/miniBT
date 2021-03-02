#include "PeerManager.h"
#include "PeerLink.h"

CPeerManager::CPeerManager(void) :
		m_pTorrentTask(NULL)
{
}

CPeerManager::~CPeerManager(void)
{

}

bool CPeerManager::Startup()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m_MutexUnusedPeer, &attr);
	pthread_mutexattr_destroy(&attr);

	m_nConnectTimerID = m_pTorrentTask->GetSocketReactor()->AddTimer(this, 2000,
			false);

	m_nChokeTimerID = m_pTorrentTask->GetSocketReactor()->AddTimer(this, 10000,
			false);

	return true;
}

void CPeerManager::Shutdown()
{
	m_pTorrentTask->GetSocketReactor()->RemoveTimer(m_nConnectTimerID);
	m_pTorrentTask->GetSocketReactor()->RemoveTimer(m_nChokeTimerID);

	map<string, PeerInfo>::iterator it = m_mapConnectedPeer.begin();
	for (; it != m_mapConnectedPeer.end(); ++it)
	{
		if (it->second.pPeerLink != NULL)
		{
			it->second.pPeerLink->CloseLink();
			delete it->second.pPeerLink;
			it->second.pPeerLink = NULL;
		}
	}

	it = m_mapConnectingPeer.begin();
	for (; it != m_mapConnectingPeer.end(); ++it)
	{
		if (it->second.pPeerLink != NULL)
		{
			it->second.pPeerLink->CloseLink();
			delete it->second.pPeerLink;
			it->second.pPeerLink = NULL;
		}
	}

	m_mapUnusedPeer.clear();
	m_mapConnectingPeer.clear();
	m_mapConnectedPeer.clear();
	m_mapBanedPeer.clear();
	pthread_mutex_destroy(&m_MutexUnusedPeer);
}

void CPeerManager::AddPeerInfo(const char *pIpAddr, int nPort)
{
	string strPeerLinkID = GenPeerLinkID(pIpAddr, nPort);
	if (PeerExists(strPeerLinkID))
	{
		return;
	}

	PeerInfo stPeerInfo;
	stPeerInfo.strLinkID = strPeerLinkID;
	stPeerInfo.strIPAddr = pIpAddr;
	stPeerInfo.nPort = nPort;
	stPeerInfo.pPeerLink = NULL;

	pthread_mutex_lock(&m_MutexUnusedPeer);
	m_mapUnusedPeer[strPeerLinkID] = stPeerInfo;
	pthread_mutex_unlock(&m_MutexUnusedPeer);
}

void CPeerManager::SetTorrentTask(ITorrentTask *pTask)
{
	m_pTorrentTask = pTask;
}

ITorrentTask * CPeerManager::GetTorrentTask()
{
	return m_pTorrentTask;
}

void CPeerManager::OnTimer(int nTimerID)
{
	static int nShotCount = 0;
	if (nTimerID == m_nConnectTimerID)
	{
		CheckPeerConnection();
	}

	if (nTimerID == m_nChokeTimerID)
	{
//        CheckPeerChoke();
		nShotCount++;
		CheckPeerChoke(nShotCount == 3);
		nShotCount = (nShotCount == 3 ? 0 : nShotCount);
	}
}

string CPeerManager::GenPeerLinkID(const char *pIPAddr, int nPort)
{
	char szBuff[100];
	sprintf(szBuff, "%s:%u", pIPAddr, nPort);

	return szBuff;
}

bool CPeerManager::PeerExists(string strPeerLinkID)
{
	bool bInUnusedPeer = false;
	pthread_mutex_lock(&m_MutexUnusedPeer);
	bInUnusedPeer = (m_mapUnusedPeer.find(strPeerLinkID)
			!= m_mapUnusedPeer.end());
	pthread_mutex_unlock(&m_MutexUnusedPeer);

	return bInUnusedPeer
			|| m_mapConnectingPeer.find(strPeerLinkID)
					!= m_mapConnectingPeer.end()
			|| m_mapConnectedPeer.find(strPeerLinkID)
					!= m_mapConnectedPeer.end()
			|| m_mapBanedPeer.find(strPeerLinkID) != m_mapBanedPeer.end();
}

void CPeerManager::CheckPeerConnection()
{
	map<string, PeerInfo>::iterator it = m_mapConnectingPeer.begin();
	for (; it != m_mapConnectingPeer.end();)
	{
		if (it->second.pPeerLink == NULL)
		{
			m_mapConnectingPeer.erase(it++);
			continue;
		}

		//Move to connected peer map
		if (it->second.pPeerLink->GetPeerState() == PS_ESTABLISHED)
		{
			it->second.nConnFailedCount = 0;
			m_mapConnectedPeer[it->second.strLinkID] = it->second;
			m_mapConnectingPeer.erase(it++);
			continue;
		}

		if (it->second.pPeerLink->GetPeerState() == PS_CONNECTFAILED)
		{
			it->second.nConnFailedCount++;
			it->second.pPeerLink->CloseLink();
			delete it->second.pPeerLink;
			it->second.pPeerLink = NULL;

			if (it->second.nConnFailedCount >= 3)
			{
				m_mapBanedPeer[it->second.strLinkID] = it->second;
			}
			else
			{
				pthread_mutex_lock(&m_MutexUnusedPeer);
				m_mapUnusedPeer[it->second.strLinkID] = it->second;
				pthread_mutex_unlock(&m_MutexUnusedPeer);
			}

			m_mapConnectingPeer.erase(it++);
			continue;
		}

		//Move closed connection to unused peer
		if (it->second.pPeerLink->GetPeerState() == PS_CLOSED)
		{
			it->second.pPeerLink->CloseLink();
			delete it->second.pPeerLink;
			it->second.pPeerLink = NULL;

			pthread_mutex_lock(&m_MutexUnusedPeer);
			m_mapUnusedPeer[it->second.strLinkID] = it->second;
			pthread_mutex_unlock(&m_MutexUnusedPeer);

			m_mapConnectingPeer.erase(it++);
			continue;
		}

		++it;
	}

	it = m_mapConnectedPeer.begin();
	for (; it != m_mapConnectedPeer.end();)
	{
		if (it->second.pPeerLink->GetPeerState() == PS_CLOSED)
		{
			bool bAccepted = it->second.pPeerLink->IsAccepted();
			it->second.pPeerLink->CloseLink();
			delete it->second.pPeerLink;
			it->second.pPeerLink = NULL;

			if (!bAccepted)
			{
				pthread_mutex_lock(&m_MutexUnusedPeer);
				m_mapUnusedPeer[it->second.strLinkID] = it->second;
				pthread_mutex_unlock(&m_MutexUnusedPeer);
			}

			m_mapConnectedPeer.erase(it++);
			continue;
		}
		++it;
	}

	pthread_mutex_lock(&m_MutexUnusedPeer);
	//Add new connection
	if (m_mapConnectedPeer.size() < m_pTorrentTask->GetMaxPeerLink())
	{
		for (;;)
		{
			if (m_mapUnusedPeer.size() == 0)
			{
				break;
			}

			if (m_mapConnectedPeer.size() >= m_pTorrentTask->GetMaxPeerLink())
			{
				break;
			}

			if (m_mapConnectingPeer.size()
					< m_pTorrentTask->GetMaxConnectingPeerLink())
			{
				PeerInfo stPeerInfo = m_mapUnusedPeer.begin()->second;
				stPeerInfo.pPeerLink = new CPeerLink();
				stPeerInfo.pPeerLink->SetPeerManager(this);
				stPeerInfo.pPeerLink->Connect(stPeerInfo.strIPAddr.c_str(),
						stPeerInfo.nPort);

				m_mapUnusedPeer.erase(m_mapUnusedPeer.begin());
				m_mapConnectingPeer[stPeerInfo.strLinkID] = stPeerInfo;
			}
			else
			{
				break;
			}
		}
	}
	pthread_mutex_unlock(&m_MutexUnusedPeer);

	//close some useless peers
	if (m_mapConnectedPeer.size() > m_pTorrentTask->GetMaxPeerLink() - 10)
	{
		int i = 0;
		it = m_mapConnectedPeer.begin();
		for (; it != m_mapConnectedPeer.end();)
		{
			if (it->second.pPeerLink->ShouldClose())
			{
				it->second.pPeerLink->CloseLink();
				i++;
			}
			if (i >= 5)
			{
				break;
			}
			++it;
		}
	}
}

//To do
void CPeerManager::CheckPeerChoke()
{
	ComputePeerSpeed();

	int nUploadCount = 0;
	map<string, PeerInfo>::iterator it = m_mapConnectedPeer.begin();

	for (; it != m_mapConnectedPeer.end(); ++it)
	{
		IPeerLink *pPeerLink = it->second.pPeerLink;
		if (pPeerLink == NULL)
		{
			continue;
		}

		if (!pPeerLink->PeerInterested())
		{
			pPeerLink->ChokePeer(true);
		}
		if (!pPeerLink->PeerChoked())
		{
			nUploadCount++;
		}
	}

	//判断是否选择优化非阻塞peer，如果选择优化非阻塞peer，则选择4个，否则只选择3个

	for (; nUploadCount < m_pTorrentTask->GetMaxUploadPeerLink();)
	{
		it = m_mapConnectedPeer.begin();
		map<string, PeerInfo>::iterator it2 = m_mapConnectedPeer.begin();
		for (; it != m_mapConnectedPeer.end(); ++it)
		{
			IPeerLink *pPeerLink = it->second.pPeerLink;
			if (pPeerLink == NULL)
			{
				continue;
			}

			if (pPeerLink->PeerChoked() && pPeerLink->PeerInterested())
			{
				if (it2 == m_mapConnectedPeer.end())
				{
					it2 = it;
				}
				else if (pPeerLink->GetDownloadSpeed()
						> it2->second.pPeerLink->GetDownloadSpeed())
				{
					it2 = it;
				}
			}
		}

		if (it2 != m_mapConnectedPeer.end())
		{
			it2->second.pPeerLink->ChokePeer(false);
			nUploadCount++;
		}
		else
		{
			break;
		}
	}

	if (nUploadCount >= m_pTorrentTask->GetMaxUploadPeerLink())
	{
		//获取当前上传peer中下载速度最慢的一个

		it = m_mapConnectedPeer.begin();
		map<string, PeerInfo>::iterator worstIt = m_mapConnectedPeer.end();

		for (; it != m_mapConnectedPeer.end(); ++it)
		{
			IPeerLink *pPeerLink = it->second.pPeerLink;
			if (pPeerLink == NULL)
			{
				continue;
			}

			if (!pPeerLink->PeerChoked() && pPeerLink->PeerInterested())
			{
				if (worstIt == m_mapConnectedPeer.end())
				{
					worstIt = it;
				}
				else if (pPeerLink->GetDownloadSpeed()
						< worstIt->second.pPeerLink->GetDownloadSpeed())
				{
					worstIt = it;
				}
			}
		}

		//得到非上传peer中下载速度最快的一个
		it = m_mapConnectedPeer.begin();
		map<string, PeerInfo>::iterator bestIt = m_mapConnectedPeer.begin();

		for (; it != m_mapConnectedPeer.end(); ++it)
		{
			IPeerLink *pPeerLink = it->second.pPeerLink;
			if (pPeerLink == NULL)
			{
				continue;
			}

			if (pPeerLink->PeerChoked() && pPeerLink->PeerInterested())
			{
				if (bestIt == m_mapConnectedPeer.end())
				{
					bestIt = it;
				}
				else if (pPeerLink->GetDownloadSpeed()
						> bestIt->second.pPeerLink->GetDownloadSpeed())
				{
					bestIt = it;
				}
			}
		}

		if (worstIt != m_mapConnectedPeer.end()
				&& bestIt != m_mapConnectedPeer.end()
				&& worstIt->second.pPeerLink->GetDownloadSpeed()
						< bestIt->second.pPeerLink->GetDownloadSpeed())
		{
			//阻塞下载最慢的downloader
			worstIt->second.pPeerLink->ChokePeer(true);

			//给下载最快的downloader机会
			bestIt->second.pPeerLink->ChokePeer(false);
		}

	}

}

void CPeerManager::CheckPeerChoke(bool bOptUnchoke)
{
	ComputePeerSpeed();

	map<string, PeerInfo>::iterator it = m_mapConnectedPeer.begin();
	vector<PeerInfo> vecTmpPeers;

	for (; it != m_mapConnectedPeer.end(); ++it)
	{
		IPeerLink *pPeerLink = it->second.pPeerLink;
		if (pPeerLink == NULL)
		{
			continue;
		}

		if (pPeerLink->PeerInterested() && pPeerLink->NeedRemoteData())
		{
			vecTmpPeers.push_back(it->second);
		}

	}

	//对mapTmpPeers中的peer按下载速度进行排序
	if (vecTmpPeers.size() > 0)
	{
		int nUnchokeCount = m_pTorrentTask->GetMaxUploadPeerLink()
				+ (bOptUnchoke ? 1 : 0);

		if (vecTmpPeers.size() > nUnchokeCount)
		{
			sort(vecTmpPeers.begin(), vecTmpPeers.end(), SortDownlaodSpeed);
		}

		if (bOptUnchoke && vecTmpPeers.size() > nUnchokeCount)
		{
			PeerInfo stPeerInfo = vecTmpPeers.at(nUnchokeCount - 1);
			for (int j = nUnchokeCount; j < vecTmpPeers.size(); ++j)
			{
				int nRandomDigits = random();
				IPeerLink *pVecPeerLink = vecTmpPeers.at(j).pPeerLink;

				if (pVecPeerLink->IsEmpty() && !stPeerInfo.pPeerLink->IsEmpty()
						&& ((nRandomDigits >>= 2) & 3))
				{
					PeerInfo stTmpPeerInfo = stPeerInfo;
					stPeerInfo = vecTmpPeers.at(j);
					vecTmpPeers[j] = stTmpPeerInfo;
				}
				else
				{
					if ((pVecPeerLink->PeerChoked()
							&& (!stPeerInfo.pPeerLink->PeerChoked()
									|| pVecPeerLink->GetLastUnchokeTime()
											< stPeerInfo.pPeerLink->GetLastUnchokeTime()))
							|| (!pVecPeerLink->PeerChoked()
									&& !stPeerInfo.pPeerLink->PeerChoked()
									&& stPeerInfo.pPeerLink->GetLastUnchokeTime()
											< pVecPeerLink->GetLastUnchokeTime()))
					{
						if (!stPeerInfo.pPeerLink->IsEmpty()
								|| pVecPeerLink->IsEmpty()
								|| !((nRandomDigits >>= 2) & 3))
						{
							PeerInfo stTmpPeerInfo = stPeerInfo;
							stPeerInfo = vecTmpPeers.at(j);
							vecTmpPeers[j] = stTmpPeerInfo;
						}
					}
				}
			}
		}

		vector<PeerInfo>::iterator pIt = vecTmpPeers.begin();
		int i = 0;
		for (; pIt != vecTmpPeers.end(); ++pIt)
		{
			i++;
			pIt->pPeerLink->ChokePeer(
					i > nUnchokeCount);
		}
	}

}

void CPeerManager::ComputePeerSpeed()
{
	map<string, PeerInfo>::iterator it = m_mapConnectedPeer.begin();
	for (; it != m_mapConnectedPeer.end(); ++it)
	{
		if (it->second.pPeerLink != NULL)
		{
			it->second.pPeerLink->ComputeSpeed();
		}
	}
}

void CPeerManager::OnDownloadComplete()
{
	map<string, PeerInfo>::iterator it = m_mapConnectedPeer.begin();

	for (; it != m_mapConnectedPeer.end(); ++it)
	{
		if (it->second.pPeerLink != NULL)
		{
			it->second.pPeerLink->OnDownloadComplete();
		}
	}
}

void CPeerManager::BroadcastHave(int nPieceIndex)
{
	map<string, PeerInfo>::iterator it = m_mapConnectedPeer.begin();
	for (; it != m_mapConnectedPeer.end(); ++it)
	{
		if (it->second.pPeerLink != NULL)
		{
			it->second.pPeerLink->NotifyHavePiece(nPieceIndex);
		}
	}
}

bool CPeerManager::AddAcceptedPeer(int nHandle, const char *pIpAddr, int nPort)
{
	if (m_mapConnectedPeer.size() >= m_pTorrentTask->GetMaxPeerLink())
	{
		return false;
	}

	string strPeerLinkID = GenPeerLinkID(pIpAddr, nPort);

	PeerInfo stInfo;
	stInfo.strLinkID = strPeerLinkID;
	stInfo.strIPAddr = pIpAddr;
	stInfo.nPort = nPort;
	stInfo.nConnFailedCount = 0;
	stInfo.pPeerLink = new CPeerLink();
	stInfo.pPeerLink->Attach(nHandle, pIpAddr, nPort, this);

	m_mapConnectedPeer[strPeerLinkID] = stInfo;

	return true;
}

int CPeerManager::GetConnectedPeerCount()
{
	return m_mapConnectedPeer.size();
}

bool SortDownlaodSpeed(const PeerInfo& stPeer1, const PeerInfo& stPeer2)
{
	bool bRet = false;
	if (stPeer1.pPeerLink->GetDownloadSpeed()
			== stPeer2.pPeerLink->GetDownloadSpeed())
	{
		bRet = (stPeer1.pPeerLink->GetUploadCount()
				/ (stPeer1.pPeerLink->GetDownloadCount() + 0.001))
				< (stPeer2.pPeerLink->GetUploadCount()
						/ (stPeer2.pPeerLink->GetDownloadCount() + 0.001));
	}
	else
	{
		bRet = stPeer1.pPeerLink->GetDownloadSpeed()
				> stPeer2.pPeerLink->GetDownloadSpeed();
	}

	return bRet;
}
