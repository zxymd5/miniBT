#include "TCPTracker.h"
#include "TorrentParser.h"
#include <errno.h>
#include <fstream>
#include <map>
#include <curl/curl.h>
#include <string.h>

CTCPTracker::CTCPTracker(void) :
		m_nTrackerState(TS_INIT)
{
	m_nCompletePeer = 0;
	m_nInterval = 0;
	m_llNextUpdateTick = 0;
	m_nPeerCount = 0;
	m_nCurrentEvent = 0;
	m_bSendStartEvent = false;
	m_bSendCompleteEvent = false;

	m_pTrackerManager = NULL;
}

CTCPTracker::~CTCPTracker(void)
{
}

void CTCPTracker::SetTrackerManager(ITrackerManager *pTrackerManager)
{
	m_pTrackerManager = pTrackerManager;
}

void CTCPTracker::SetURL(const char *pUrl)
{
	m_strTrackerURL = pUrl;
}

void CTCPTracker::Update()
{

	CURL *pHandle = curl_easy_init();

	m_strTrackerResponse.clear();
	m_nTrackerState = TS_CONNECTING;
	m_nCurrentEvent = GetCurrentEvent();

	string strURL = GenTrackerURL(Event2Str(m_nCurrentEvent));

	curl_easy_setopt(pHandle, CURLOPT_URL, strURL.c_str());
	curl_easy_setopt(pHandle, CURLOPT_WRITEFUNCTION, OnRecvData);
	curl_easy_setopt(pHandle, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(pHandle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(pHandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(pHandle, CURLOPT_TIMEOUT, 60);
	CURLcode nRetCode = curl_easy_perform(pHandle);

	if (nRetCode == CURLE_OK)
	{
		ParseTrackerResponse();
	}
	else
	{
		m_nTrackerState = TS_ERROR;

		m_llNextUpdateTick = GetTickCount() + 15 * 1000;
	}

	curl_easy_cleanup(pHandle);
}

void CTCPTracker::Shutdown()
{

}

int CTCPTracker::GetSeedCount()
{
	return m_nCompletePeer;
}

int CTCPTracker::GetPeerCount()
{
	return m_nPeerCount;
}

int CTCPTracker::GetInterval()
{
	return m_nInterval;
}

long long CTCPTracker::GetNextUpdateTick()
{
	return m_llNextUpdateTick;
}

bool CTCPTracker::IsProtocolSupported(const char * pProtocol)
{
	return true;
}

int CTCPTracker::GetCurrentEvent()
{
	if (!m_bSendStartEvent)
	{
		return TE_START;
	}

	if (m_pTrackerManager->GetTorrentTask()->GetTaskStorage()->Finished()
			&& !m_bSendCompleteEvent)
	{
		return TE_COMPLETE;
	}

	return TE_NONE;
}

const char * CTCPTracker::Event2Str(int nEvent)
{
	switch (nEvent)
	{
	case TE_START:
		return "started";
		break;
	case TE_STOP:
		return "stopped";
		break;
	case TE_COMPLETE:
		return "completed";
		break;
	case TE_NONE:
		return "";
		break;
	}

	return "";
}

string CTCPTracker::GenTrackerURL(const char *pEvent)
{
	char szDstURL[1024];
	memset(szDstURL, 0, 1024);
	sprintf(szDstURL,
			"%s?info_hash=%s&peer_id=%s&port=%d&compact=1&uploaded=%lld&downloaded=%lld&left=%lld&event=%s",
			m_strTrackerURL.c_str(),
			URLEncode(
					m_pTrackerManager->GetTorrentTask()->GetTorrentFile()->GetInfoHash(),
					20).c_str(),
			URLEncode(
					(const unsigned char *) (m_pTrackerManager->GetTorrentTask()->GetPeerID().c_str()),
					20).c_str(),
			m_pTrackerManager->GetTorrentTask()->GetAcceptor()->GetPort(),
			m_pTrackerManager->GetTorrentTask()->GetDownloadCount(),
			m_pTrackerManager->GetTorrentTask()->GetUploadCount(),
			m_pTrackerManager->GetTorrentTask()->GetTaskStorage()->GetLeftCount(),
			pEvent);

	return szDstURL;
}

int CTCPTracker::GetTrackerState()
{
	return m_nTrackerState;
}

void CTCPTracker::ParseTrackerResponse()
{
//     fstream fs;
//     fs.open("announce", ios::in | ios::binary);
//     fs.seekg(0, ios::end);
//     int nSize = fs.tellg();
//     fs.seekg(0, ios::beg);
//     
//     char c;
//     for (int i = 0; i < nSize; ++i)
//     {
//         fs.seekg(i, ios::beg);
//         fs.read(&c, 1);
//         m_strTrackerResponse.push_back(c);
//     }
// 
//     fs.close();

	if (ParseBasicInfo() == false)
	{
		return;
	}

	int nType = GetTrackerResponseType();
	if (nType == 1)
	{
		ParsePeerInfoType1();
	}
	else if (nType == 2)
	{
		ParsePeerInfoType2();
	}

	int nConnectedPeerCount =
			m_pTrackerManager->GetTorrentTask()->GetPeerManager()->GetConnectedPeerCount();
	int nMaxPeerLink = m_pTrackerManager->GetTorrentTask()->GetMaxPeerLink();

	if (nConnectedPeerCount >= nMaxPeerLink
			|| nConnectedPeerCount > m_nPeerCount / 2 || m_nInterval <= 2 * 60)
	{
		m_llNextUpdateTick = GetTickCount() + m_nInterval * 1000;
	}
	else
	{
		m_llNextUpdateTick = GetTickCount() + 15 * 1000;
	}

	if (m_nCurrentEvent == TE_START)
	{
		m_bSendStartEvent = true;
	}
	else if (m_nCurrentEvent == TE_COMPLETE)
	{
		m_bSendCompleteEvent = true;
	}
}

size_t CTCPTracker::OnRecvData(void *pBuffer, size_t nSize, size_t nMemb,
		void *ptr)
{
	CTCPTracker *pTracker = (CTCPTracker *) ptr;
	pTracker->m_strTrackerResponse.append((char *) pBuffer, nSize * nMemb);

	return nSize * nMemb;
}

bool CTCPTracker::ParseBasicInfo()
{
	bool bRet = true;
	int nStart = 0;
	int nEnd = 0;
	const char *p = m_strTrackerResponse.c_str();

	if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(),
			"14:failure reason", nStart, nEnd))
	{
		bRet = false;
		m_nTrackerState = TS_ERROR;
		m_llNextUpdateTick = GetTickCount() + 60 * 1000;
	}
	else
	{
		if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(),
				"8:interval", nStart, nEnd))
		{
			m_nInterval = atoi(p + nEnd + 1);
		}

		if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(),
				"10:done peers", nStart, nEnd))
		{
			m_nCompletePeer = atoi(p + nEnd + 1);
		}

		if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(),
				"9:num peers", nStart, nEnd))
		{
			m_nPeerCount = atoi(p + nEnd + 1);
		}

		if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(),
				"8:complete", nStart, nEnd))
		{
			m_nCompletePeer = atoi(p + nEnd + 1);
		}

		if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(),
				"10:incomplete", nStart, nEnd))
		{
			m_nPeerCount = m_nCompletePeer + atoi(p + nEnd + 1);
		}
	}

	return bRet;
}

int CTCPTracker::GetTrackerResponseType()
{
	int nType = -1;

	int nStart = 0;
	int nEnd = 0;
	if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(), "5:peers",
			nStart, nEnd))
	{
		if (m_strTrackerResponse[nEnd] != 'l')
		{
			nType = 1;
		}
		else
		{
			nType = 2;
		}
	}

	return nType;
}

bool CTCPTracker::ParsePeerInfoType1()
{
	bool bRet = false;
	string strRedirectURL;

	if ((bRet = TrackerRedirection(strRedirectURL)) == true)
	{
		//To do
	}
	else
	{
		int nStart = 0;
		int nEnd = 0;
		long lCount = 0;
		const char *p = m_strTrackerResponse.c_str();

		if (CTorrentParser::FindPattern(p, "5:peers", nStart, nEnd))
		{
			lCount += atol(p + nEnd) / 6;

			p += nEnd;
			while ((*p) != ':')
			{
				p++;
				nEnd++;
			}

			int n = m_strTrackerResponse.size();

			while (lCount > 0)
			{
				unsigned char c[4];
				nEnd++;
				c[0] = m_strTrackerResponse[nEnd];
				nEnd++;
				c[1] = m_strTrackerResponse[nEnd];
				nEnd++;
				c[2] = m_strTrackerResponse[nEnd];
				nEnd++;
				c[3] = m_strTrackerResponse[nEnd];

				char szIpAddr[20];
				memset(szIpAddr, 0, 20);
				sprintf(szIpAddr, "%u.%u.%u.%u", c[0], c[1], c[2], c[3]);

				nEnd++;
				unsigned short nPort =
						ntohs(
								*(unsigned short *) (m_strTrackerResponse.c_str()
										+ nEnd));
				nEnd++;

				m_pTrackerManager->GetTorrentTask()->GetPeerManager()->AddPeerInfo(
						szIpAddr, nPort);
				bRet = true;
				lCount--;
			}
		}
	}

	return bRet;
}

bool CTCPTracker::ParsePeerInfoType2()
{
	return true;
}

bool CTCPTracker::TrackerRedirection(string &strRedirectionURL)
{
	bool bRedirect = false;
	int nStart = 0;
	int nEnd = 0;

	if (CTorrentParser::FindPattern(m_strTrackerResponse.c_str(), "Location:",
			nStart, nEnd))
	{
		for (size_t i = nEnd + 1, j = 0; i < m_strTrackerResponse.size();
				i++, j++)
		{
			if (m_strTrackerResponse[i] != '?')
			{
				strRedirectionURL.push_back(m_strTrackerResponse[i]);
			}
			else
			{
				break;
			}
		}
		bRedirect = true;
	}

	return bRedirect;
}
