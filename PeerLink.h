#ifndef PEER_LINK_H
#define PEER_LINK_H

#include "MyBitTorrent.h"
#include "MySocket.h"
#include "PieceRequest.h"
#include "BitSet.h"

class CPeerLink :
    public IPeerLink,
    public ITimerCallback,
    public CSocket,
    public IRateMeasureClient
{
public:
    CPeerLink(void);
    virtual ~CPeerLink(void);
    virtual void SetPeerManager(IPeerManager *pManager);
    virtual int GetPeerState();
    virtual void Connect(const char *IpAddr, int nPort);
    virtual void CloseLink();
    virtual bool IsAccepted();
    virtual bool ShouldClose();
    virtual void ComputeSpeed();
    
    virtual void OnTimer(int nTimerID);

    virtual void SetUploadSpeed(unsigned int nSpeed);
    virtual void SetDownloadSpeed(unsigned int nSpeed);
    virtual unsigned int GetUploadSpeed();
    virtual unsigned int GetDownloadSpeed();

    virtual int HandleRead();
    virtual int HandleWrite();
    virtual void HandleClose();

    virtual void BlockRead(bool bBlock);
    virtual void BlockWrite(bool bBlock);
    virtual void SetWritePriority(int nPriority);
    virtual int GetWritePriority();
    virtual void SetReadPriority(int nPriority);
    virtual int GetReadPriority();
    virtual bool CanWrite();
    virtual bool CanRead();
    virtual int DoWrite(long long llCount);
    virtual int DoRead(long long llCount);
    virtual void NotifyHavePiece(int nPieceIndex);
    virtual void Attach(int nHandle, const char *IpAddr, int nPort, IPeerManager *pManager);
    virtual unsigned int GetDownloadCount();
    virtual unsigned int GetUploadCount();

    void CancelPieceRequest(int nPieceIndex);
    virtual bool PeerChoked();
    virtual bool PeerInterested();
    virtual void ChokePeer(bool bChoke);

    void OnConnect();
    void OnConnectFailed();
    void OnConnectClosed();
    void OnSendComplete();
    virtual void OnDownloadComplete();
    virtual bool NeedRemoteData();
    virtual long long GetLastUnchokeTime();
    virtual bool IsEmpty();

private:
    int m_nPeerState;
    IPeerManager *m_pPeerManager;
    string  m_strIPAddr;
    int m_nPort;
    string m_strPeerLinkID;
    bool m_bAccepted;
    int m_nConnTimeoutID;

    CPieceRequest   m_clPieceRequest;
    string  m_strSendBuffer;
    string  m_strRecvBuffer;

    CBitSet m_clBitSet;
    bool m_bBitSetRecved;

    bool m_bHandShaked;
    bool m_bAmChoking;
    bool m_bAmInterested;
    bool m_bPeerChoking;
    bool m_bPeerInterested;

    unsigned int m_nDownloadCount;
    unsigned int m_nUploadCount;
    long long m_llLastCountSpeedTime;
    unsigned int m_nLastDownloadCount;
    unsigned int m_nLastUploadCount;
    unsigned int m_nUploadSpeed;
    unsigned int m_nDownloadSpeed;
    long long m_llLastUnchokeTime;

    list<PeerPieceRequest> m_lstPeerPieceRequest;

    bool m_bCanRead;
    bool m_bCanWrite;
    int m_nWritePriority;
    int m_nReadPriority;

private:
    void SendData(const void *pData, int nLen);
    int ProcRecvData();
    int SendKeepAlive();
    void SendHandShake();
    void SendBitField();
    void SendChoke(bool bChoke);
    void SendInterested(bool bInterested);
    void SendHave(int nPieceIndex);
    void SendPieceRequest(int nPieceIndex, int nOffset, int nLen);
    void SendPieceData(int nPieceIndex, int nOffset, string &strData);
    void SendPieceCancel(unsigned int nPieceIndex, unsigned int nOffset, unsigned int nLen);
    bool CheckHandshake(string strInfo);

    int ProcCmd(int nCmd, void *pData, int nDataLen);
    int ProcCmdChoke(void *pData, int nDataLen);
    int ProcCmdUnchoke(void *pData, int nDataLen);
    int ProcCmdInterested(void *pData, int nDataLen);
    int ProcCmdNotInterested(void *pData, int nDataLen);
    int ProcCmdHave(void *pData, int nDataLen);
    int ProcCmdBitfield(void *pData, int nDataLen);
    int ProcCmdRequest(void *pData, int nDataLen);
    int ProcCmdPiece(void *pData, int nDataLen);
    int ProcCmdCancel(void *pData, int nDataLen);

    void GetNewPieceTask();
    void DoPieceRequest();
    void DoPieceSend();

};

#endif
