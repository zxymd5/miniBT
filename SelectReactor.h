#ifndef SELECT_REACTOR_H
#define SELECT_REACTOR_H

#include "MyBitTorrent.h"
#include <list>
#include <sys/select.h>
using namespace std;

class CSelectReactor : public ISocketReactor
{
public:
    CSelectReactor(void);

    void ClearFdSet();

    virtual ~CSelectReactor(void);
    virtual bool AddSocket(ISocket *pSocket);
    virtual void RemoveSocket(ISocket *pSocket);
    void AddToFdSet();
    virtual bool Startup();
    virtual void Update();
    virtual void Shutdown();
    int SelectSocket();
    int AddTimer(ITimerCallback *pCallback, int nInterval, bool bOneShot);
    void RemoveTimer(int nTimerID);
    void UpdateTimerList();

private:
    vector<ISocket *> m_vecSockets;
    fd_set  m_rSet;
    fd_set  m_wSet;
    int m_nMaxSocketFd;
    int m_nFreeTimerID;
    list<int> m_lstFreeTimerID;
    list<TimerInfo> m_lstTimerInfo;
    list<TimerInfo> m_lstAddedTimerInfo;
};

#endif
