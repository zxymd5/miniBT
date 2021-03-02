#include "RateMeasure.h"
#include <string.h>

CRateMeasure::CRateMeasure(void) : m_llUploadSpeed(IRateMeasure::nNoLimitedSpeed),
                                   m_llDownloadSpeed(IRateMeasure::nNoLimitedSpeed),
                                   m_llLastTick(0),
                                   m_llUploadCount(0),
                                   m_llDownloadCount(0)
{
}

CRateMeasure::~CRateMeasure(void)
{
}

void CRateMeasure::AddClient( IRateMeasureClient *pClient )
{
    pClient->SetReadPriority(0);
    pClient->SetWritePriority(0);

    RateMeasureCtx ctx;
    ctx.pClient = pClient;
    ctx.bRemove = false;

    m_vecClient.push_back(ctx);
}

void CRateMeasure::RemoveClient( IRateMeasureClient *pClient )
{
    vector<RateMeasureCtx>::iterator it = m_vecClient.begin();

    for (; it != m_vecClient.end(); ++it)
    {
        if (it->pClient == pClient)
        {
            it->bRemove = true;
            break;
        }
    }
}

void CRateMeasure::Update()
{
    long long llCurrentTick = GetTickCount();
    if (llCurrentTick - m_llLastTick > 1000)
    {
        ResetAllPriority();
        m_llLastTick = llCurrentTick;
        m_llUploadCount = 0;
        m_llDownloadCount = 0;
    }

    ScheduleUpload();
    ScheduleDownload();
}

void CRateMeasure::SetUploadSpeed(long long llSpeed )
{
    m_llUploadSpeed = llSpeed;
}

void CRateMeasure::SetDownloadSpeed(long long llSpeed )
{
    m_llDownloadSpeed = llSpeed;
}

long long CRateMeasure::GetUploadSpeed()
{
    return m_llUploadSpeed;
}

long long CRateMeasure::GetDownloadSpeed()
{
    return m_llDownloadSpeed;
}

void CRateMeasure::ResetAllPriority()
{
    vector<RateMeasureCtx>::iterator it = m_vecClient.begin();

    for (; it != m_vecClient.end(); )
    {
        if (it->bRemove)
        {
            it = m_vecClient.erase(it);
            continue;
        }

        it->pClient->BlockWrite(false);
        it->pClient->BlockRead(false);

        it->pClient->SetWritePriority(0);
        it->pClient->SetReadPriority(0);
        ++it;
    }
}

void CRateMeasure::ScheduleUpload()
{
    unsigned int nActiveCount[MAX_PRIORITY_LEVEL];

    memset(nActiveCount, 0, sizeof(unsigned int) * MAX_PRIORITY_LEVEL);

    vector<RateMeasureCtx>::iterator it = m_vecClient.begin();
    for (; it != m_vecClient.end(); )
    {
        if (it->bRemove)
        {
            it = m_vecClient.erase(it);
            continue;
        }

        if (it->pClient->CanWrite())
        {
            nActiveCount[it->pClient->GetWritePriority()]++;
        }

        ++it;
    }

    for (unsigned int i = 0; i < MAX_PRIORITY_LEVEL; ++i)
    {
        if (nActiveCount[i] == 0)
        {
            continue;
        }

        if (m_llUploadSpeed <= m_llUploadCount)
        {
            vector<RateMeasureCtx>::iterator it = m_vecClient.begin();

            for (; it != m_vecClient.end(); ++it)
            {
                if (it->pClient->GetWritePriority() == i
                    && it->pClient->CanWrite())
                {
                    it->pClient->BlockWrite(true);
                }
            }

            continue;
        }

        long long llLeftCount = m_llUploadSpeed - m_llUploadCount;
        long long llWriteCount = llLeftCount / nActiveCount[i];
        if (llWriteCount < 512)
        {
            llWriteCount = 512;
        }

        vector<RateMeasureCtx>::iterator it = m_vecClient.begin();
        for (; it != m_vecClient.end(); ++it)
        {
            if (it->pClient->GetWritePriority() == i
                && it->pClient->CanWrite())
            {
                long long llRet = it->pClient->DoWrite(llWriteCount);
                m_llUploadCount += llRet;

                if (llRet > 0 && i < MAX_PRIORITY_LEVEL - 1)
                {
                    it->pClient->SetWritePriority(i + 1);
                }

                if (llRet < llLeftCount)
                {
                    llLeftCount -= llRet;
                }
                else
                {
                    llLeftCount = 0;
                    break;
                }
            }
        }
    }
}

void CRateMeasure::ScheduleDownload()
{
    unsigned int nActiveCount[MAX_PRIORITY_LEVEL];
    memset(nActiveCount, 0, sizeof(unsigned int) * MAX_PRIORITY_LEVEL);

    vector<RateMeasureCtx>::iterator it = m_vecClient.begin();
    for (; it != m_vecClient.end();)
    {
        if (it->bRemove)
        {
            it = m_vecClient.erase(it);
            continue;
        }

        if (it->pClient->CanRead())
        {
            nActiveCount[it->pClient->GetReadPriority()]++;
        }

        ++it;
    }

    for (unsigned int i = 0; i < MAX_PRIORITY_LEVEL; ++i)
    {
        if (nActiveCount[i] == 0)
        {
            continue;
        }

        if (m_llDownloadSpeed <= m_llDownloadCount)
        {
            vector<RateMeasureCtx>::iterator it = m_vecClient.begin();

            for (; it != m_vecClient.end(); ++it)
            {
                if (it->pClient->GetReadPriority() == i
                    && it->pClient->CanRead())
                {
                    it->pClient->BlockRead(true);
                }
            }

            continue;
        }

        long long llLeftCount = m_llDownloadSpeed - m_llDownloadCount;
        long long llReadCount = llLeftCount / nActiveCount[i];

        if (llReadCount < 512)
        {
            llReadCount = 512;
        }

        vector<RateMeasureCtx>::iterator it = m_vecClient.begin();
        for (; it != m_vecClient.end(); ++it)
        {
            if (it->pClient->GetReadPriority() == i
                && it->pClient->CanRead())
            {
                long long llRet = it->pClient->DoRead(llReadCount);
                m_llDownloadCount += llRet;

                if (llRet > 0 && i < MAX_PRIORITY_LEVEL - 1)
                {
                    it->pClient->SetReadPriority(i + 1);
                }

                if (llRet < llLeftCount)
                {
                    llLeftCount -= llRet;
                }
                else
                {
                    llLeftCount = 0;
                    break;
                }
            }
        }
    }
}

void CRateMeasure::BlockReadAll( bool bBlock )
{

}

void CRateMeasure::BlockWriteAll( bool bBlock )
{

}
