#include "SelectReactor.h"
#include <algorithm>

CSelectReactor::CSelectReactor(void)
{
    m_nMaxSocketFd = 0;
    m_nFreeTimerID = 0;
}

CSelectReactor::~CSelectReactor(void)
{
}

bool CSelectReactor::AddSocket( ISocket *pSocket )
{
    vector<ISocket *>::iterator it = find(m_vecSockets.begin(), m_vecSockets.end(), pSocket);
    if (it == m_vecSockets.end())
    {
        m_vecSockets.push_back(pSocket);
    }

    return true;
}

void CSelectReactor::RemoveSocket( ISocket *pSocket )
{
    vector<ISocket *>::iterator it = find(m_vecSockets.begin(), m_vecSockets.end(), pSocket);
    if (it != m_vecSockets.end())
    {
        m_vecSockets.erase(it);
    }

    if (pSocket->GetHandleMask() & READ_MASK)
    {
        FD_CLR(pSocket->GetHandle(), &m_rSet);
    }

    if (pSocket->GetHandleMask() & WRITE_MASK)
    {
        FD_CLR(pSocket->GetHandle(), &m_wSet);
    }
    
}

void CSelectReactor::AddToFdSet()
{
    vector<ISocket *>::iterator it = m_vecSockets.begin();
    for (; it != m_vecSockets.end(); ++it)
    {
        if ((*it)->GetHandle() > m_nMaxSocketFd)
        {
            m_nMaxSocketFd = (*it)->GetHandle();
        }

        if ((*it)->GetHandleMask() & READ_MASK)
        {
            FD_SET((*it)->GetHandle(), &m_rSet);
        }

        if ((*it)->GetHandleMask() & WRITE_MASK)
        {
            FD_SET((*it)->GetHandle(), &m_wSet);
        }

    }
}

bool CSelectReactor::Startup()
{
    ClearFdSet();
    return true;
}

void CSelectReactor::Update()
{
    SelectSocket();
    UpdateTimerList();
}

void CSelectReactor::Shutdown()
{
    ClearFdSet();
    m_vecSockets.clear();
}

void CSelectReactor::ClearFdSet()
{
    FD_ZERO(&m_rSet);
    FD_ZERO(&m_wSet);
    m_nMaxSocketFd = 0;
}

int CSelectReactor::SelectSocket()
{
    ClearFdSet();
    AddToFdSet();

    int nRet = 0;
    timeval tmval;
    tmval.tv_sec = 1;
    tmval.tv_usec = 0;

    nRet = select(m_nMaxSocketFd + 1, &m_rSet, &m_wSet, NULL, &tmval);

    if (nRet > 0)
    {
        vector<ISocket *>::iterator it = m_vecSockets.begin();
        for (; it != m_vecSockets.end(); ++it)
        {
            if (FD_ISSET((*it)->GetHandle(), &m_rSet))
            {
               int nRes = (*it)->HandleRead();
               if (nRes == -1)
               {
                   (*it)->HandleClose();
                   (*it)->SetReactor(NULL);
                   (*it)->Close();
                   continue;
               }
            }

            if (FD_ISSET((*it)->GetHandle(), &m_wSet))
            {
                int nRes = (*it)->HandleWrite();
                if (nRes == -1)
                {
                    (*it)->HandleClose();
                    (*it)->SetReactor(NULL);
                    (*it)->Close();
                    continue;
                }
            }
        }

    }

    return nRet;
}

int CSelectReactor::AddTimer( ITimerCallback *pCallback, int nInterval, bool bOneShot )
{
    if (pCallback == NULL)
    {
        return 0;
    }

    TimerInfo stInfo;
    if (m_lstFreeTimerID.size() == 0)
    {
        stInfo.nTimerID = m_nFreeTimerID;
        m_nFreeTimerID++;
    }
    else
    {
        stInfo.nTimerID = m_lstFreeTimerID.front();
        m_lstFreeTimerID.pop_front();
    }

    stInfo.nInterval = nInterval;
    stInfo.bOneShot = bOneShot;
    stInfo.bRemove = false;
    stInfo.llLastshotTick = GetTickCount();
    stInfo.pCallback = pCallback;

    m_lstAddedTimerInfo.push_back(stInfo);

    return stInfo.nTimerID;
}

void CSelectReactor::RemoveTimer( int nTimerID )
{
    list<TimerInfo>::iterator it = m_lstTimerInfo.begin();
    for (; it != m_lstTimerInfo.end(); ++it)
    {
        if (it->nTimerID == nTimerID)
        {
            it->bRemove = true;
            return;
        }
    }

    list<TimerInfo>::iterator it2 = m_lstAddedTimerInfo.begin();
    for (; it2 != m_lstAddedTimerInfo.end(); ++it2)
    {
        if (it2->nTimerID == nTimerID)
        {
            m_lstFreeTimerID.push_back(nTimerID);
            m_lstAddedTimerInfo.erase(it2);
            return;
        }
    }
}

void CSelectReactor::UpdateTimerList()
{
    list<TimerInfo>::iterator it = m_lstTimerInfo.begin();

    for (; it != m_lstTimerInfo.end(); )
    {
        if (it->bRemove == true)
        {
            m_lstFreeTimerID.push_back(it->nTimerID);
            it = m_lstTimerInfo.erase(it);
            continue;
        }

        if (GetTickCount() >= it->llLastshotTick + it->nInterval)
        {
            it->pCallback->OnTimer(it->nTimerID);
            it->llLastshotTick = GetTickCount();

            if(it->bOneShot)
            {
                m_lstFreeTimerID.push_back(it->nTimerID);
                it = m_lstTimerInfo.erase(it);
                continue;
            }
        }

        ++it;
    }

    list<TimerInfo>::iterator it2 = m_lstAddedTimerInfo.begin();
    for(; it2 != m_lstAddedTimerInfo.end(); ++it2)
    {
        m_lstTimerInfo.push_back(*it2);
    }

    m_lstAddedTimerInfo.clear();
}
