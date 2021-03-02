#include "BitSet.h"

CBitSet::CBitSet(void)
{
}

CBitSet::~CBitSet(void)
{
}

void CBitSet::Alloc( int nSize )
{
    m_nSize = nSize;
    if (nSize % 8 == 0)
    {
        m_strBits.resize(nSize / 8);
    }
    else
    {
        m_strBits.resize(nSize / 8 + 1);
    }
    m_strBits.assign(m_strBits.size(), 0);
}

void CBitSet::Alloc( string &strBit, int nSize )
{
    m_nSize = nSize;
    m_strBits = strBit;
}

bool CBitSet::IsSet( int nIndex )
{
    char c = m_strBits[nIndex / 8];
    int nBit = nIndex % 8;
    int nMask = 128;

    nMask = nMask >> nBit;
    if ((c & nMask) != 0)
    {
        return true;
    }

    return false;
}

void CBitSet::Set( int nIndex, bool bSet )
{
    char c = m_strBits[nIndex / 8];
    int nBit = nIndex % 8;

    int nMask = 128;
    nMask = nMask >> nBit;

    if (bSet)
    {
        c = c | nMask;
    }
    else
    {
        nMask = ~nMask;
        c = c & nMask;
    }

    m_strBits[nIndex / 8] = c;
}

bool CBitSet::IsAllSet()
{
   for (int i = 0; i < m_strBits.size() - 1; ++i)
   {
       if (m_strBits[i] != 0xFF)
       {
           return false;
       }
   }
    
   unsigned char b = (unsigned char)m_strBits[m_strBits.size() - 1];
   for (int i = 0; i < m_nSize - (m_strBits.size() - 1) * 8; ++i)
   {
       int nMask = 128;
       nMask = nMask >> i;
       if ((b & nMask) == 0)
       {
           return false;
       }
   }

   return true; 
}

bool CBitSet::IsEmpty()
{
    for (int i = 0; i < m_strBits.size(); ++i)
    {
        if (m_strBits[i] != 0)
        {
            return false;
        }
    }

    return true;
}

int CBitSet::GetSetCount()
{
    int nRet = 0;
    for (int i = 0; i < m_nSize; ++i)
    {
        if (IsSet(i))
        {
            nRet++;
        }
    }

    return nRet;
}

string &CBitSet::GetBits()
{
    return m_strBits;
}

int CBitSet::GetSize()
{
    return m_nSize;
}
