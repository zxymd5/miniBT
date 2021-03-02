#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H

#include "MyBitTorrent.h"
#include <map>
#include <pthread.h>

bool SortDownlaodSpeed(const PeerInfo &stPeer1, const PeerInfo &stPeer2);

class CPeerManager :
    public IPeerManager,
    public ITimerCallback
{
public:
    CPeerManager(void);
    virtual ~CPeerManager(void);
    virtual bool Startup();
    virtual void Shutdown();
    virtual void AddPeerInfo(const char *pIpAddr, int nPort);
    virtual bool AddAcceptedPeer(int nHandle, const char *pIpAddr, int nPort);
    virtual void SetTorrentTask(ITorrentTask *pTask);
    virtual ITorrentTask *GetTorrentTask();
    virtual void OnTimer(int nTimerID);
    virtual void OnDownloadComplete();
    virtual void BroadcastHave(int nPieceIndex);
    virtual int GetConnectedPeerCount();
    string GenPeerLinkID(const char *pIPAddr, int nPort);
    bool PeerExists(string strPeerLinkID);
    void CheckPeerConnection();
    void CheckPeerChoke();
    void CheckPeerChoke(bool bOptUnchoke);
    void ComputePeerSpeed();

private:
    ITorrentTask *m_pTorrentTask;
    map<string, PeerInfo> m_mapUnusedPeer;
    map<string, PeerInfo> m_mapConnectingPeer;
    map<string, PeerInfo> m_mapConnectedPeer;
    map<string, PeerInfo> m_mapBanedPeer;
    int m_nConnectTimerID;
    int m_nChokeTimerID;
    
    pthread_mutex_t m_MutexUnusedPeer;
};

#endif
