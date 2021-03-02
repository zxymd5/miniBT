#ifndef RATE_MEASURE_H
#define RATE_MEASURE_H

#include "MyBitTorrent.h"
#include <vector>
using namespace std;

typedef struct RateMeasureCtx
{
    IRateMeasureClient *pClient;
    bool bRemove;
} RateMeasureCtx;

class CRateMeasure :
    public IRateMeasure
{
public:
    CRateMeasure(void);
    virtual ~CRateMeasure(void);
    virtual void AddClient(IRateMeasureClient *pClient);
    virtual void RemoveClient(IRateMeasureClient *pClient);
    virtual void Update();
    virtual void SetUploadSpeed(long long llSpeed);
    virtual void SetDownloadSpeed(long long llSpeed);
    virtual long long GetUploadSpeed();
    virtual long long GetDownloadSpeed();
    
    void ResetAllPriority();
    void ScheduleUpload();
    void ScheduleDownload();
    void BlockReadAll(bool bBlock);
    void BlockWriteAll(bool bBlock);

private:
    vector<RateMeasureCtx> m_vecClient;
    long long m_llUploadSpeed;
    long long m_llDownloadSpeed;
    long long m_llLastTick;
    long long m_llUploadCount;
    long long m_llDownloadCount;
    bool m_bWriteBlock;
    bool m_bReadBlock;
};

#endif
