#ifndef BIT_SET_H
#define BIT_SET_H

#include "MyBitTorrent.h"

class CBitSet :
    public IBitSet
{
public:
    CBitSet(void);
    virtual ~CBitSet(void);
    virtual void Alloc(int nSize);
    virtual void Alloc(string &strBits, int nSize);
    virtual bool IsSet(int nIndex);
    virtual void Set(int nIndex, bool bSet);
    virtual bool IsAllSet();
    virtual bool IsEmpty();
    virtual int GetSetCount();
    virtual string &GetBits();
    virtual int GetSize();

private:
    string m_strBits;
    int m_nSize;

};

#endif
