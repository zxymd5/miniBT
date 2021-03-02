#ifndef PEER_ACCEPTOR_H
#define PEER_ACCEPTOR_H

#include "MyBitTorrent.h"
#include "MySocket.h"

class CPeerAcceptor :
    public IPeerAcceptor,
    public CSocket
{
public:
    CPeerAcceptor(void);
    ~CPeerAcceptor(void);
    virtual bool Startup();
    virtual void Shutdown();
    virtual int GetPort();
    virtual void SetTorrentTask(ITorrentTask *pTask);
    virtual ITorrentTask *GetTorrentTask();
    virtual int HandleRead();
    virtual int HandleWrite();
    virtual void HandleClose();

private:
    int m_nPort;
    ITorrentTask *m_pTask;
};

#endif
