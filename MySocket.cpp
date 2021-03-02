#include "MySocket.h"
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

CSocket::CSocket(void)
            : m_nHandle(-1),
              m_bInReactor(false),
              m_pReactor(NULL),
              m_nHandleMask(0)
{
}

CSocket::~CSocket(void)
{
    Close();
}

void CSocket::SetReactor( ISocketReactor *pReactor )
{
    if (pReactor != NULL)
    {
        if (m_bInReactor)
        {
            pReactor->RemoveSocket(this);
            m_bInReactor = false;
        }
        m_pReactor = NULL;
    }

    if (pReactor != NULL)
    {
        m_pReactor = pReactor;
        m_pReactor->AddSocket(this);
        m_bInReactor = true;
    }
}

ISocketReactor * CSocket::GetReactor()
{
    return m_pReactor;
}

void CSocket::CreateTCPSocket()
{
    m_nHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nHandle == -1)
    {
        HandleErrMsg("Create socket failed", __FILE__, errno, __LINE__);
        return;
    }
    SetNonBlock();
}

int CSocket::GetHandle()
{
    return m_nHandle;
}

int CSocket::GetHandleMask()
{
    return m_nHandleMask;
}

void CSocket::SetNonBlock()
{
	int nFlags = fcntl(m_nHandle, F_GETFL, 0);
	fcntl(m_nHandle, F_SETFL, nFlags | O_NONBLOCK);
}

void CSocket::Close()
{
    if (m_nHandle != -1)
    {
        if (close(m_nHandle) != 0)
        {
            HandleErrMsg("Close failed", __FILE__, errno, __LINE__);
        }
        m_nHandle = -1;
    }
}

int CSocket::HandleRead()
{
    return 0;
}

int CSocket::HandleWrite()
{
    return 0;
}

void CSocket::HandleClose()
{
    GetReactor()->RemoveSocket(this);
}

bool CSocket::Bind( const char *pIpAddr, int nPort )
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    if (pIpAddr != NULL)
    {
        addr.sin_addr.s_addr = inet_addr(pIpAddr);
    }
    else
    {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    addr.sin_port = htons(nPort);

    return bind(m_nHandle, (const sockaddr*)&addr, sizeof(addr)) == 0;
}

void CSocket::Listen()
{
    listen(m_nHandle, 5);
}

void CSocket::SetHandleMask( int nHandleMask )
{
    m_nHandleMask = nHandleMask;
}

void CSocket::RemoveHandleMask( int nHandleMask )
{
    m_nHandleMask = (m_nHandleMask & (~nHandleMask));
}

bool CSocket::GetRemotAddrInfo( const char* pHostName, int nPort, sockaddr_in &stRemoteAddr )
{
    stRemoteAddr.sin_family=AF_INET;
    stRemoteAddr.sin_port=htons(nPort);
    stRemoteAddr.sin_addr.s_addr = inet_addr(pHostName);

    return true;
}

void CSocket::Connect( const char* pHostName, int nPort )
{
    struct sockaddr_in stRemoteAddr;
    GetRemotAddrInfo(pHostName, nPort, stRemoteAddr);
    connect(m_nHandle, (const sockaddr *)&stRemoteAddr, sizeof(stRemoteAddr));
}

void CSocket::Attach( int nSocketFd )
{
    Close();
    m_nHandle = nSocketFd;
    SetNonBlock();
}

int CSocket::Accept( string &strIpAddr, int &nPort )
{
    struct sockaddr_in addr;
    unsigned int nLen = sizeof(addr);
    memset(&addr, 0, nLen);

    int fd = accept(m_nHandle, (struct sockaddr *)&addr, &nLen);
    if (fd != -1)
    {
        strIpAddr = inet_ntoa(addr.sin_addr);
        nPort = ntohs(addr.sin_port);
    }

    return fd;
}
