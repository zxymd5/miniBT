#ifndef TRACKER_MANAGER_H
#define TRACKER_MANAGER_H

#include "MyBitTorrent.h"
#include <vector>
#include <pthread.h>

using namespace std;

class CTrackerManager :
    public ITrackerManager
{
public:
    CTrackerManager(void);
    virtual ~CTrackerManager(void);
    virtual bool Startup();
    virtual void Shutdown();
    virtual void SetTorrentTask(ITorrentTask *pTask);
    virtual ITorrentTask *GetTorrentTask();
    void Svc();
    void CreateTrackers();
    void DestroyTrackers();
    void ShutdownTrackers();
    static void *ThreadFunc(void *pParam);

private:
    ITorrentTask *m_pTorrentTask;
    pthread_t m_dwTrackerThread;
    vector<ITracker *> m_vecTrackers;
    bool m_bExit;

};

#endif
