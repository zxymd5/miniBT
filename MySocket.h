#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#include "MyBitTorrent.h"

class CSocket : public ISocket
{
public:
    CSocket(void);
    virtual ~CSocket(void);
    virtual void SetReactor(ISocketReactor *pReactor);
    virtual ISocketReactor *GetReactor();
    virtual void CreateTCPSocket();
    virtual int GetHandle();
    virtual int GetHandleMask();
    virtual void SetHandleMask(int nHandleMask);
    virtual void RemoveHandleMask(int nHandleMask);
    virtual void SetNonBlock();
    virtual void Close();
    virtual int HandleRead();
    virtual int HandleWrite();
    virtual void HandleClose();
    virtual bool Bind(const char *pIpAddr, int nPort);
    virtual void Listen();
    virtual void Connect( const char* pHostName, int nPort);
    virtual void Attach(int nSocketFd);
    bool GetRemotAddrInfo( const char* pHostName, int nPort, sockaddr_in &stRemoteAddr );
    virtual int Accept(string &strIpAddr, int &nPort);

private:
    int m_nHandle;
    int m_nHandleMask;
    ISocketReactor *m_pReactor;
    bool m_bInReactor;
};

#endif
