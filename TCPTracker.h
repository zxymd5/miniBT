#ifndef TCP_TRACKER_H
#define TCP_TRACKER_H

#include "MyBitTorrent.h"
#include "MySocket.h"

class CTCPTracker :
    public ITracker
{
public:
    CTCPTracker(void);
    virtual ~CTCPTracker(void);
    virtual void SetTrackerManager(ITrackerManager *pTrackerManager);
    virtual bool IsProtocolSupported(const char * pProtocol);
    virtual void SetURL(const char *pUrl);
    virtual void Update();
    virtual void Shutdown();
    virtual int GetSeedCount();
    virtual int GetPeerCount();
    virtual int GetInterval();
    virtual long long GetNextUpdateTick();
    int GetCurrentEvent();
    const char *Event2Str(int nEvent);
    string GenTrackerURL( const char *pEvent );
    virtual int GetTrackerState();

    void ParseTrackerResponse();
    int GetTrackerResponseType();
    bool ParseBasicInfo();
    bool ParsePeerInfoType1();
    bool ParsePeerInfoType2();
    bool TrackerRedirection(string &strRedirectionURL);

    static size_t OnRecvData(void *pBuffer, size_t nSize, size_t nMemb, void *ptr);

private:
    string m_strTrackerURL;
    string m_strTrackerResponse;

    int m_nTrackerState;
    int m_nCompletePeer;
    int m_nInterval;
    long long m_llNextUpdateTick;
    int m_nPeerCount;
    int m_nCurrentEvent;
    bool m_bSendStartEvent;
    bool m_bSendCompleteEvent;

    ITrackerManager *m_pTrackerManager;
};

#endif
