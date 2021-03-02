#include "PeerAcceptor.h"
#include <unistd.h>

CPeerAcceptor::CPeerAcceptor(void)
{
}

CPeerAcceptor::~CPeerAcceptor(void)
{
}

bool CPeerAcceptor::Startup()
{
    for (int i = 7681; i < 7690; ++i)
    {
        CreateTCPSocket();

        if (CSocket::Bind(NULL, i) == true)
        {
            m_nPort = i;
            CSocket::Listen();
            CSocket::SetReactor(m_pTask->GetSocketReactor());
            CSocket::SetHandleMask(READ_MASK);

            return true;
        }
    }

    return false;
}

void CPeerAcceptor::Shutdown()
{
    RemoveHandleMask(READ_MASK);
    CSocket::SetReactor(NULL);
    CSocket::Close();
}

int CPeerAcceptor::GetPort()
{
    return m_nPort;
}

void CPeerAcceptor::SetTorrentTask( ITorrentTask *pTask )
{
    m_pTask = pTask;
}

ITorrentTask * CPeerAcceptor::GetTorrentTask()
{
    return m_pTask;
}

int CPeerAcceptor::HandleRead()
{
    string strIpAddr;
    int nPort;
    int fd = CSocket::Accept(strIpAddr, nPort);
    if (fd != -1)
    {
        if (!m_pTask->GetPeerManager()->AddAcceptedPeer(fd, strIpAddr.c_str(), nPort))
        {
            close(fd);
        }
    }

    return 0;
}

int CPeerAcceptor::HandleWrite()
{
    RemoveHandleMask(WRITE_MASK);
    return 0;
}

void CPeerAcceptor::HandleClose()
{

}
