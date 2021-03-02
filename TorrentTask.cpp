#include "TorrentTask.h"
#include "TorrentFile.h"
#include "SelectReactor.h"
#include "RateMeasure.h"
#include "TrackManager.h"
#include "TaskStorage.h"
#include "PeerAcceptor.h"
#include "PeerManager.h"

#include <string.h>
#include <time.h>

CTorrentTask::CTorrentTask(void) :
		m_pTorrentFile(NULL), m_pSocketReactor(NULL), m_pRateMeasure(NULL), m_pPeerAcceptor(
				NULL), m_pTaskStorage(NULL), m_pPeerManager(NULL), m_llCacheSize(
				0), m_bExit(false)
{

}

CTorrentTask::~CTorrentTask(void)
{
}

bool CTorrentTask::Startup()
{
	GenPeerID();
	Reset();

	m_pSocketReactor = new CSelectReactor;

	m_pRateMeasure = new CRateMeasure;
	m_pRateMeasure->SetDownloadSpeed(IRateMeasure::nNoLimitedSpeed);
	m_pRateMeasure->SetUploadSpeed(IRateMeasure::nNoLimitedSpeed);

	m_pTrackerManager = new CTrackerManager;
	m_pTrackerManager->SetTorrentTask(this);

	m_pPeerManager = new CPeerManager;
	m_pPeerManager->SetTorrentTask(this);

	m_pTaskStorage = new CTaskStorage;
	m_pTaskStorage->SetTorrentTask(this);

	m_pPeerAcceptor = new CPeerAcceptor;
	m_pPeerAcceptor->SetTorrentTask(this);

	m_pTaskStorage->Startup();
	m_pSocketReactor->Startup();
	m_pPeerAcceptor->Startup();
	m_pTrackerManager->Startup();
	m_pPeerManager->Startup();

	m_nSpeedTimerID = m_pSocketReactor->AddTimer(this, 2000, false);

	pthread_create(&m_dwTaskThread, NULL, ThreadFunc, this);

	return true;
}

const string & CTorrentTask::GetPeerID()
{
	return m_strPeerID;
}

void CTorrentTask::GenPeerID()
{
	char szPeerID[21];
	memset(szPeerID, 0, 21);
	srand((unsigned) time(NULL));
	sprintf(szPeerID, "-WB1234-%.12d", rand());
	m_strPeerID.assign(szPeerID);
}

void CTorrentTask::Reset()
{
	m_llDownloadCount = 0;
	m_llUploadCount = 0;
	m_llDownloadSpeed = 0;
	m_llUploadSpeed = 0;
	m_llLastDownloadCount = 0;
	m_llLastUploadCount = 0;
	m_llLastCheckSpeedTime = 0;
	m_lstDownloadSpeed.clear();
	m_lstUploadSpeed.clear();
	m_nSpeedTimerID = 0;
	m_nMaxPeerLink = 100;
	m_nMaxUploadPeerLink = 3;
	m_llCacheSize = 5 * 1024 * 1024;
}

void CTorrentTask::LoadTorrentFile(const char *pTorrentFilePath)
{
	m_pTorrentFile = new CTorrentFile;
	m_pTorrentFile->Load(pTorrentFilePath);
}

void CTorrentTask::Shutdown()
{
	m_bExit = true;

	pthread_join(m_dwTaskThread, NULL);

	if (m_pPeerAcceptor)
	{
		m_pPeerAcceptor->Shutdown();
		delete m_pPeerAcceptor;
		m_pPeerAcceptor = NULL;
	}

	if (m_pTrackerManager)
	{
		m_pTrackerManager->Shutdown();
		delete m_pTrackerManager;
		m_pTrackerManager = NULL;
	}

	if (m_pPeerManager)
	{
		m_pPeerManager->Shutdown();
		delete m_pPeerManager;
		m_pPeerManager = NULL;
	}

	if (m_pSocketReactor)
	{
		m_pSocketReactor->Shutdown();
		delete m_pSocketReactor;
		m_pSocketReactor = NULL;
	}

	if (m_pTaskStorage)
	{
		m_pTaskStorage->Shutdown();
		delete m_pTaskStorage;
		m_pTaskStorage = NULL;
	}

	if (m_pTorrentFile)
	{
		delete m_pTorrentFile;
		m_pTorrentFile = NULL;
	}

	if (m_pRateMeasure)
	{
		delete m_pRateMeasure;
		m_pRateMeasure = NULL;
	}

}

long long CTorrentTask::GetDownloadSpeed()
{
	return m_llDownloadSpeed;
}

long long CTorrentTask::GetUploadSpeed()
{
	return m_llUploadSpeed;
}

void *CTorrentTask::ThreadFunc(void *pParam)
{
	CTorrentTask *pTask = (CTorrentTask *) pParam;
	pTask->Svc();
	return NULL;
}

void CTorrentTask::Svc()
{
	while (!m_bExit)
	{
		m_pSocketReactor->Update();
		m_pRateMeasure->Update();
	}
}

ISocketReactor * CTorrentTask::GetSocketReactor()
{
	return m_pSocketReactor;
}

ITorrentFile * CTorrentTask::GetTorrentFile()
{
	return m_pTorrentFile;
}

IPeerAcceptor * CTorrentTask::GetAcceptor()
{
	return m_pPeerAcceptor;
}

long long CTorrentTask::GetDownloadCount()
{
	return m_llDownloadCount;
}

long long CTorrentTask::GetUploadCount()
{
	return m_llUploadCount;
}

ITaskStorage * CTorrentTask::GetTaskStorage()
{
	return m_pTaskStorage;
}

void CTorrentTask::OnTimer(int nTimerID)
{
	if (nTimerID == m_nSpeedTimerID)
	{
		m_llUploadSpeed = CheckUploadSpeed();
		m_llDownloadSpeed = CheckDownloadSpeed();

		m_llLastCheckSpeedTime = GetTickCount();
		m_llLastDownloadCount = m_llDownloadCount;
		m_llLastUploadCount = m_llUploadCount;
	}
}

IPeerManager * CTorrentTask::GetPeerManager()
{
	return m_pPeerManager;
}

int CTorrentTask::GetMaxPeerLink()
{
	return m_nMaxPeerLink;
}

void CTorrentTask::SetMaxPeerLink(int nMaxPeerLink)
{
	m_nMaxPeerLink = nMaxPeerLink;
}

int CTorrentTask::GetMaxConnectingPeerLink()
{
	return 20;
}

IRateMeasure * CTorrentTask::GetRateMeasure()
{
	return m_pRateMeasure;
}

string CTorrentTask::GetDstPath()
{
	return m_strDstPath;
}

void CTorrentTask::SetDstPath(const char *pPath)
{
	m_strDstPath = pPath;
	if (m_strDstPath[m_strDstPath.length() - 1] != '/')
	{
		m_strDstPath += "/";
	}
}

string CTorrentTask::GetTaskName()
{
	string strPath = m_pTorrentFile->GetTorrentFilePath();
	string::size_type pos = strPath.rfind('/');

	if (pos == string::npos)
	{
		return strPath;
	}

	if (pos != strPath.size() - 1)
	{
		return strPath.substr(pos + 1, strPath.size() - pos - 1);
	}

	return "";
}

long long CTorrentTask::GetCacheSize()
{
	return m_llCacheSize;
}

void CTorrentTask::SetCacheSize(long long llCacheSize)
{
	m_llCacheSize = llCacheSize;
}

void CTorrentTask::AddDownloadCount(int nCount)
{
	m_llDownloadCount += nCount;
}

void CTorrentTask::AddUploadCount(int nCount)
{
	m_llUploadCount += nCount;
}

int CTorrentTask::GetMaxUploadPeerLink()
{
	return m_nMaxUploadPeerLink;
}

void CTorrentTask::SetMaxUploadPeerLink(int nMaxUploadPeerLink)
{
	m_nMaxUploadPeerLink = nMaxUploadPeerLink;
}

long long CTorrentTask::CheckDownloadSpeed()
{
	long long llCurrentSpeed = 1000
			* (m_llDownloadCount - m_llLastDownloadCount)
			/ (GetTickCount() - m_llLastCheckSpeedTime);

	while (m_lstDownloadSpeed.size() >= 5)
	{
		m_lstDownloadSpeed.pop_front();
	}
	m_lstDownloadSpeed.push_back(llCurrentSpeed);

	long long llResult = 0;
	list<long long>::iterator it = m_lstDownloadSpeed.begin();
	for (; it != m_lstDownloadSpeed.end(); ++it)
	{
		llResult += *it;
	}

	return llResult / m_lstDownloadSpeed.size();
}

long long CTorrentTask::CheckUploadSpeed()
{
	long long llCurrentSpeed = 1000 * (m_llUploadCount - m_llLastUploadCount)
			/ (GetTickCount() - m_llLastCheckSpeedTime);

	while (m_lstUploadSpeed.size() >= 5)
	{
		m_lstUploadSpeed.pop_front();
	}
	m_lstUploadSpeed.push_back(llCurrentSpeed);

	long long llResult = 0;
	list<long long>::iterator it = m_lstUploadSpeed.begin();
	for (; it != m_lstUploadSpeed.end(); ++it)
	{
		llResult += *it;
	}

	return llResult / m_lstUploadSpeed.size();
}
