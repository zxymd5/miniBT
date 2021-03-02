#ifndef TASK_STORAGE_H
#define TASK_STORAGE_H

#include "MyBitTorrent.h"
#include "BitSet.h"
#include <map>

class CTaskStorage :
    public ITaskStorage
{
public:
    CTaskStorage(void);
    virtual ~CTaskStorage(void);
    virtual bool Startup();
    virtual void Shutdown();
    virtual bool Finished();
    virtual IBitSet *GetBitSet();
    virtual int GetPieceLength(int nPieceIndex);
    virtual int GetPieceTask(IBitSet *pBitSet);
    virtual void WritePiece(int nPieceIndex, string &strData);
    virtual string ReadData(int nPieceIndex, long long llOffset, int nLen);
    virtual string ReadPiece(int nPieceIndex);
    virtual void SaveToReadCache(int nPieceIndex, string &strData);
    virtual float GetFinishedPercent();
    virtual long long GetLeftCount();
    virtual void SetTorrentTask(ITorrentTask *pTorrentTask);
    virtual ITorrentTask *GetTorrentTask();
    virtual string GetBitField();
    virtual void AbandonPieceTask(int nPieceIndex);
private:
    ITorrentTask *m_pTorrentTask;
    string m_strDstDir;
    string m_strPsfPath;
    bool m_bNewTask;
    list<StorageFileInfo> m_lstStorageFile;
    CBitSet m_clBitSet;
    list<Range> m_lstRange;
    list<int> m_lstDownloadingPieces;
    unsigned int m_nFinishedPiece;
    map<int, string> m_mapWriteCache;
    map<int, PieceCache> m_mapReadCache;

private:
    bool OpenFiles();
    bool ShouldCheckFiles();
    void CheckFiles();
    void MakeRequestRange();
    void SaveBitset();
    bool OpenFile(int nIndex, FileInfo stFileInfo);
    StorageFileInfo GetFileInfoByOffset(long long llOffset);
    unsigned int GetMaxReadCacheSize();
    unsigned int GetMaxWriteCacheSize();
    void WritePieceD(int nPieceIndex, string &strData);
    void SaveWriteCacheToDisk();
    string ReadDataD(int nPieceIndex, long long llOffset, int nLen);

    int GetPieceTaskInRange(IBitSet *pBitSet, int nBeginIndex, int nEndIndex);
    void RemoveDownloadingPiece(int nPieceIndex);

    void GenBanedBitSet();
    int GetPieceIndexByOffset(long long llOffset);
};

#endif
