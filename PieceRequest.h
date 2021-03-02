#ifndef PIECE_REQUEST_H
#define PIECE_REQUEST_H
#include "MyBitTorrent.h"

class CPieceRequest
{
public:
    CPieceRequest(void);
    virtual ~CPieceRequest(void);
    void CreatePieceRequestList(unsigned int nPieceIndex, unsigned int nPieceLength, unsigned int nMaxRequestLength);
    unsigned int GetPieceIndex();
    unsigned int GetPieceLength();
    unsigned int GetPendingCount();
    bool CancelPendingRequest(unsigned int &nPieceIndex, unsigned int &nOffset, unsigned int &nLength);
    bool Complete();
    string GetPiece();
    bool GetRequest(unsigned int &nPieceIndex, unsigned int &nOffset, unsigned int &nLength);
    bool AddPieceData(unsigned int nOffset, const char *pData, unsigned int nLength);
    void ClearRequest();

private:
    unsigned int m_nPieceIndex;
    unsigned int m_nPieceLength;
    list<PieceRequest> m_lstRequest;
};

#endif
