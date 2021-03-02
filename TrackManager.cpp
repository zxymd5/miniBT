#include "TrackManager.h"
#include "TorrentParser.h"
#include "TCPTracker.h"

#include <unistd.h>

CTrackerManager::CTrackerManager(void) : m_pTorrentTask(NULL)
{
}

CTrackerManager::~CTrackerManager(void)
{
}

bool CTrackerManager::Startup()
{
    CreateTrackers();
    m_bExit = false;

    pthread_create(&m_dwTrackerThread, NULL, ThreadFunc, this);
    return true;
}

void CTrackerManager::Shutdown()
{
    m_bExit = true;
    ShutdownTrackers();

    pthread_join(m_dwTrackerThread, NULL);

    DestroyTrackers();
}

void CTrackerManager::SetTorrentTask( ITorrentTask *pTask )
{
    m_pTorrentTask = pTask;
}

ITorrentTask * CTrackerManager::GetTorrentTask()
{
    return m_pTorrentTask;
}

void CTrackerManager::Svc()
{
    while(!m_bExit)
    {
        vector<ITracker *>::iterator it = m_vecTrackers.begin();
        for (; it != m_vecTrackers.end(); ++it)
        {
            if (m_bExit)
            {
                break;
            }

            if (GetTickCount() < (*it)->GetNextUpdateTick())
            {
                continue;
            }

            (*it)->Update();
        }

        usleep(15000);
    }
}

void *CTrackerManager::ThreadFunc( void *pParam )
{
    CTrackerManager *pManager = (CTrackerManager *)pParam;
    pManager->Svc();

    return NULL;
}

void CTrackerManager::CreateTrackers()
{
    vector<string> vecAnnouce = m_pTorrentTask->GetTorrentFile()->GetAnnounceList();

    vector<string>::iterator it = vecAnnouce.begin();
    for (; it != vecAnnouce.end(); ++it)
    {
        ITracker *pTracker = NULL;
        int nStart = 0;
        int nEnd = 0;
        if (CTorrentParser::FindPattern((*it).c_str(), "HTTP://", nStart, nEnd) == true ||
            CTorrentParser::FindPattern((*it).c_str(), "http://", nStart, nEnd) == true)
        {
            pTracker = new CTCPTracker;
            pTracker->SetURL((*it).c_str());
            pTracker->SetTrackerManager(this);
            m_vecTrackers.push_back(pTracker);
        }
    }
}

void CTrackerManager::DestroyTrackers()
{
    vector<ITracker *>::iterator it = m_vecTrackers.begin();
    for (; it != m_vecTrackers.end(); ++it)
    {
        delete (*it);
    }

    m_vecTrackers.clear();
}

void CTrackerManager::ShutdownTrackers()
{
    vector<ITracker *>::iterator it = m_vecTrackers.begin();
    for (; it != m_vecTrackers.end(); ++it)
    {
        (*it)->Shutdown();
    }
}
