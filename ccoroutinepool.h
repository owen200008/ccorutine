#pragma once

#include "ccoroutinedef.h"
#include "ccoroutine.h"
#include <vector>
#include <atomic>

///////////////////////////////////////////////////////////////////////////////////////////////////////
class CCorutinePool;
class CCorutineThreadData {
public:
    CCorutineThreadData();
    virtual ~CCorutineThreadData() = default;
    CCorutinePool* GetCorutine() { return m_pPool; }
    CCTHREADID_DWORD GetThreadID() { return m_dwThreadID; }

    static CCorutineThreadData* GetTLSValue();

    // Diagnostic allocations
    void* operator new(size_t nSize) {
        return CCORUTINE_MALLOC(nSize);
    }
    void operator delete(void* p) {
        CCORUTINE_FREE(p);
    }
protected:
    CCTHREADID_DWORD									    m_dwThreadID;
    CCorutinePool*                                          m_pPool;

    static CCORUTINETLS_Key                                 m_tlsKey;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
//协程超时管理
class CCorutinePoolMgr {
public:
    CCorutinePoolMgr();
    virtual ~CCorutinePoolMgr();

    // Diagnostic allocations
    void* operator new(size_t nSize) {
        return CCORUTINE_MALLOC(nSize);
    }
    void operator delete(void* p) {
        CCORUTINE_FREE(p);
    }
    //! 加入多余的协程
    void ReleaseCorutineToGlobal(CCorutine* pMore[], int nCount);
    //! 获取多余的协程
    int GetCorutineFromGlobal(CCorutine* pMore[], int nCount);


    //! 创建协程需要添加
    void CreateCorutine(CCorutine* p);

    //! 检查所有的协程是否有问题
    void CheckAllCorutine();

    bool CheckCorutinePoolMgrCorrect();
protected:
    void AddNewQueue();
protected:
    //保存创建的协程
    std::atomic<bool>	                                    m_nLock;
    uint32_t												m_cap;
    uint32_t												m_tail;
    CCorutine**										        m_queue;

    uint32_t												m_CheckTail;
    uint32_t												m_checkCap;
    CCorutine**										        m_checkQueue;

    //用于全局调度的协程
    std::atomic<bool>	                                    m_nLockMore;
    uint32_t												m_moreCap;
    uint32_t												m_moreTail;
    CCorutine**										        m_moreQueue;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
class CCorutinePool {
#define CorutinePlus_Max_Stack 16
public:
    CCorutinePool();
    virtual ~CCorutinePool();

    // Diagnostic allocations
    void* operator new(size_t nSize) {
        return CCORUTINE_MALLOC(nSize);
    }
    void operator delete(void* p) {
        CCORUTINE_FREE(p);
    }

    //! 初始化协程数和最大的协程数
    bool InitCorutine(int nDefaultSize = 16, int nLimitCorutineCount = 1024);

    //! 获取协程
    CCorutine* GetCorutine();

    uint32_t GetVTCorutineSize() { return m_nCorutineSize; }
    uint32_t GetCreateCorutineTimes() { return m_nCreateTimes; }

    static bool CheckCorutinePoolMgrCorrect();
protected:
    //! 回收协程
    void ReleaseCorutine(CCorutine* pPTR);

    //! 唤醒协程
    void ResumeFunc(CCorutine* pNext);
    void YieldFunc(CCorutine* pCorutine);

    virtual void FinishFunc(CCorutine* pCorutine);

    //! 扩展队列
    void AddNewQueue();
protected:
    CCorutine * CreateCorutine();
    virtual CCorutine* ConstructCorutine();
protected:
    bool                                m_bInit;
    uint32_t                            m_nSleepTimes;
    
    uint32_t						    m_cap;
    uint32_t                            m_nCorutineSize;
    CCorutine**	                        m_queue;
    uint32_t						    m_nCreateTimes;
    uint32_t                            m_nLimitSize;           //如果超过这个数字，协程会放到公共里面去

    CCorutine*					        m_pStackRunCorutine[CorutinePlus_Max_Stack];
    unsigned short						m_usRunCorutineStack;
    CCorutine					        m_main;

    static CCorutinePoolMgr             m_poolMgr;

    friend class CCorutine;
};







