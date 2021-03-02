#include "PieceRequest.h"

CPieceRequest::CPieceRequest(void)
{
}

CPieceRequest::~CPieceRequest(void)
{
}

void CPieceRequest::CreatePieceRequestList( unsigned int nPieceIndex, unsigned int nPieceLength, unsigned int nMaxRequestLength )
{
    m_nPieceIndex = nPieceIndex;
    m_nPieceLength = nPieceLength;
    m_lstRequest.clear();

    unsigned int nLeft = nPieceLength;
    for (; nLeft > 0; )
    {
        int nRequestLength;
        if (nLeft > nMaxRequestLength)
        {
            nRequestLength = nMaxRequestLength;
        }
        else
        {
            nRequestLength = nLeft;
        }

        PieceRequest stRequest;
        stRequest.nOffset = nPieceLength - nLeft;
        stRequest.nLen = nRequestLength;
        stRequest.bRequested = false;
        stRequest.strData.clear();

        m_lstRequest.push_back(stRequest);
        nLeft -= nRequestLength;
    }
}

bool CPieceRequest::CancelPendingRequest( unsigned int &nPieceIndex, unsigned int &nOffset, unsigned int &nLength )
{
    list<PieceRequest>::iterator it = m_lstRequest.begin();
    for (; it != m_lstRequest.end(); ++it)
    {
        if (it->bRequested && it->strData.size() == 0)
        {
            nPieceIndex = m_nPieceIndex;
            nOffset = it->nOffset;
            nLength = it->nLen;

            m_lstRequest.erase(it);

            return true;
        }
    }

    return false;
}

bool CPieceRequest::Complete()
{
    list<PieceRequest>::iterator it = m_lstRequest.begin();
    for (; it != m_lstRequest.end(); ++it)
    {
        if (it->strData.size() != it->nLen)
        {
            return false;
        }
    }

    return true;
}

unsigned int CPieceRequest::GetPieceIndex()
{
    return m_nPieceIndex;
}

unsigned int CPieceRequest::GetPieceLength()
{
    return m_nPieceLength;
}

unsigned int CPieceRequest::GetPendingCount()
{
    unsigned int nCount = 0;
    list<PieceRequest>::iterator it = m_lstRequest.begin();
    for (; it != m_lstRequest.end(); ++it)
    {
        if (it->bRequested && it->strData.size() == 0)
        {
            nCount++;
        }
    }

    return nCount;
}

string CPieceRequest::GetPiece()
{
    if (!Complete())
    {
        return "";
    }

    string strResult;
    list<PieceRequest>::iterator it = m_lstRequest.begin();
    for (; it != m_lstRequest.end(); ++it)
    {
        strResult += it->strData;
    }

    return strResult;
}

bool CPieceRequest::GetRequest( unsigned int &nPieceIndex, unsigned int &nOffset, unsigned int &nLength )
{
    list<PieceRequest>::iterator it = m_lstRequest.begin();
    for (; it != m_lstRequest.end(); ++it)
    {
        if (!it->bRequested && it->strData.size() == 0)
        {
            nPieceIndex = m_nPieceIndex;
            nOffset = it->nOffset;
            nLength = it->nLen;
            it->bRequested = true;
            return true;
        }
    }

    return false;
}

bool CPieceRequest::AddPieceData( unsigned int nOffset, const char *pData, unsigned int nLength )
{
    list<PieceRequest>::iterator it = m_lstRequest.begin();
    for (; it != m_lstRequest.end(); ++it)
    {
        if (it->bRequested &&
            it->nOffset == nOffset &&
            it->nLen == nLength)
        {
            it->strData.clear();
            it->strData.append(pData, nLength);
            return true;
        }
    }

    return false;
}

void CPieceRequest::ClearRequest()
{
    list<PieceRequest>::iterator it = m_lstRequest.begin();

    for (; it != m_lstRequest.end(); ++it)
    {
        if (it->bRequested && it->strData.size() == 0)
        {
            it->bRequested = false;
        }
    }
}
