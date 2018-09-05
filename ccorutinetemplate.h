#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstring>
#include "ccoroutinepool.h"

//!
//线程工作函数定义
typedef void (*CCLOCKFREEQUEUE_THREAD_START_ROUTINE)(void* lpThreadParameter);

enum CCContainUnitStatus {
    CCContainUnitStatus_Ready,
    CCContainUnitStatus_Wait,
    CCContainUnitStatus_Finish
};

class CCoroutineUnitThread;
class CCoroutineUnit {
public:
    struct CoroutineUintData {
        uint32_t m_nSendTimes;
        uint32_t m_nReceiveTimes;
        void Send() {
            m_nSendTimes++;
        }
        void Received() {
            m_nReceiveTimes++;
        }
        bool IsSendReceiveSame() {
            return m_nReceiveTimes == m_nSendTimes && m_nSendTimes > 0;
        }
    };
    typedef CCoroutineUnitThread UnitThread;
public:
    virtual ~CCoroutineUnit() {
        if (m_pNode) {
            delete[]m_pNode;
        }
    }
    UnitThread* GetUnitThread() {
        return m_pUnitThread;
    }
    virtual void Init(UnitThread* pThread, uint32_t nIndex, uint32_t nCount, CCoroutineUnit* pNext) {
        m_pUnitThread = pThread;
        m_nIndex = nIndex;
        m_nCount = nCount;
        m_pNode = new CoroutineUintData[nCount];
        memset(m_pNode, 0, sizeof(CoroutineUintData) * nCount);
        m_pNext = pNext;
        m_bFinish = false;
        m_lock = false;
    }
    uint32_t GetPushCtx() {
        uint32_t lRet = m_nPushTime++;
        if (lRet >= m_nCount)
            return 0xFFFFFFFF;
        return getPushCtx(lRet);
    }
    uint32_t GetPushCtxNoNull() {
        return getPushCtx(m_nPushTime++);
    }
    inline uint32_t getPushCtx(uint32_t lRet) {
        lRet = lRet % m_nCount;
        auto* pRet = &m_pNode[lRet];
        pRet->Send();
        return lRet;
    }
    virtual bool CheckIsSuccess() {
        for (uint32_t i = 0; i < m_nCount; i++) {
            if (!m_pNode[i].IsSendReceiveSame())
                return false;
        }
        return true;
    }
    virtual void Send(uint32_t nIndex) {
        m_pNode[nIndex].Send();
    }
    virtual void Receive(uint32_t nIndex) {
        m_pNode[nIndex].Received();
    }
    uint32_t GetPushTimes() {
        return m_nPushTime;
    }
    uint32_t GetIndex() {
        return m_nIndex;
    }
    uint32_t GetCount() {
        return m_nCount;
    }
    void AddCorutine(CCorutine* p) {
        while (m_lock.exchange(true, std::memory_order_acquire)) {
            _mm_pause();
        }
        m_vtWaitCo.push_back(p);
        m_lock.store(false, std::memory_order_release);
    }

    void getVTWaitCo(std::vector<CCorutine*>& vtWaitCo) {
        while (m_lock.exchange(true, std::memory_order_acquire)) {
            _mm_pause();
        }
        swap(m_vtWaitCo, vtWaitCo);
        m_lock.store(false, std::memory_order_release);
    }
    void SetFinish() {
        m_bFinish = true;
    }
    bool                                    m_bFinish;
    CCoroutineUnit*                         m_pNext;
    std::atomic<bool>                       m_lock;
    std::vector<CCorutine*>                 m_vtWaitCo;
protected:
    uint32_t                                m_nIndex = 0;
    uint32_t                                m_nCount = 0;
    uint32_t                                m_nPushTime = 0;
    CoroutineUintData*                      m_pNode = nullptr;
    UnitThread*                             m_pUnitThread = nullptr;
};

class CCoroutineUnitThread {
public:
    virtual ~CCoroutineUnitThread() {
        if (m_pUint)
            delete[]m_pUint;
    }
    virtual void Init(uint32_t nThreadCount, uint32_t nCount) {
        m_nThreadCount = nThreadCount;
        m_pUint = createCCContainUnit(nThreadCount);
        for (uint32_t i = 0; i < nThreadCount; i++) {
            m_pUint[i].Init(this, i, nCount, &m_pUint[(i + 1) % nThreadCount]);
        }
    }
    virtual bool CheckIsSuccess() {
        for (uint32_t i = 0; i < m_nThreadCount; i++) {
            if (!m_pUint[i].CheckIsSuccess())
                return false;
        }
        return true;
    }
    virtual void Receive(uint32_t nThreadIndex, uint32_t nIndex) {
        m_pUint[nThreadIndex].Receive(nIndex);
    }
    CCContainUnitStatus GetTimeStatus() {
        return m_statusTimeFinish;
    }
    uint32_t GetSendTimes() {
        uint32_t nRetValue = 0;
        for (uint32_t i = 0; i < m_nThreadCount; i++) {
            nRetValue += m_pUint[i].GetPushTimes();
        }
        return nRetValue;
    }
    CCoroutineUnit* GetCoroutineUintByIndex(uint32_t nIndex) {
        return &m_pUint[nIndex];
    }
    void SetTimeFinish() {
        for (uint32_t i = 0; i < m_nThreadCount; i++) {
            m_pUint[i].SetFinish();
        }
    }
protected:
    virtual CCoroutineUnit* createCCContainUnit(uint32_t nCount) {
        return new CCoroutineUnit[nCount];
    }
protected:
    uint32_t                        m_nThreadCount = 0;
    CCoroutineUnit*                 m_pUint = nullptr;
    CCContainUnitStatus             m_statusTimeFinish = CCContainUnitStatus_Ready;
};

void SingleCorutineTest(void* p) {
    CCoroutineUnit* pTest = (CCoroutineUnit*)p;
    CCorutinePool* S = new CCorutinePool();
    S->InitCorutine();
    CCorutine* pCorutine = S->GetCorutine();
    pCorutine->Create([](CCorutine* p) {
        CCoroutineUnit* pTest = p->GetParamPoint<CCoroutineUnit>(0);
        while (true) {
            p->YieldCorutine();
            uint32_t nIndex = p->GetParam<uint32_t>(0);
            if (nIndex == 0xFFFFFFFF)
                break;
            pTest->Receive(nIndex);
            
        }
    });
    pCorutine->Resume(S, pTest);
    while (true) {
        uint32_t nIndex = pTest->GetPushCtx();
        pCorutine->Resume(S, &nIndex);
        if (nIndex == 0xFFFFFFFF) {
            break;
        }
    }
#ifdef _DEBUG
    assert(pCorutine->GetCoroutineState() == CoroutineState_Death);
#endif
    delete S;
}

void MultiCorutineTest(void* p) {
    CCoroutineUnit* pTest = (CCoroutineUnit*)p;
    CCorutinePool* S = new CCorutinePool();
    S->InitCorutine();
    while (true) {
        uint32_t nIndex = pTest->GetPushCtx();
        if (nIndex == 0xFFFFFFFF) {
            break;
        }
        CCorutine* pCorutine = S->GetCorutine();
        pCorutine->Create([](CCorutine* p) {
            CCoroutineUnit* pTest = p->GetParamPoint<CCoroutineUnit>(0);
            uint32_t nIndex = p->GetParam<uint32_t>(1);
            pTest->Receive(nIndex);
        });
        pCorutine->Resume(S, pTest, &nIndex);
#ifdef _DEBUG
        assert(pCorutine->GetCoroutineState() == CoroutineState_Death);
#endif
    }
    delete S;
}

void DiGuiFunc(CCorutine* p) {
    CCoroutineUnit* pTest = p->GetParamPoint<CCoroutineUnit>(0);
    uint32_t nBegin = p->GetParam<uint32_t>(1);
    uint32_t nEnd = p->GetParam<uint32_t>(2);
    pTest->Receive(nBegin);
    if (nBegin == nEnd)
        return;
    CCorutinePool* pRunPool = p->GetRunPool();
    CCorutine* pPlus = p->GetRunPool()->GetCorutine();
    pPlus->Create(DiGuiFunc);
    pPlus->Resume(pRunPool, pTest, &(++nBegin), &nEnd);
}

void DiGuiCorutineTest(void* p) {
    CCoroutineUnit* pTest = (CCoroutineUnit*)p;
   
    uint32_t nDiGui = 6;
    CCorutinePool* S = new CCorutinePool();
    S->InitCorutine();
    //默认使用6层递归创建
    uint32_t i = 0;
    while (true) {
        uint32_t nBegin = pTest->GetPushCtx();
        if (nBegin == 0xFFFFFFFF)
            break;
        uint32_t j = 0;
        for (; j < nDiGui - 1; j++) {
            uint32_t nIndex = pTest->GetPushCtx();
            if (nIndex == 0xFFFFFFFF)
                break;
        }
        uint32_t nEnd = nBegin + j;
        CCorutine* pCorutine = S->GetCorutine();
        pCorutine->Create(DiGuiFunc);
        pCorutine->Resume(S, pTest, &nBegin, &nEnd);

#ifdef _DEBUG
        assert(pCorutine->GetCoroutineState() == CoroutineState_Death);
#endif
    }
    delete S;
}

void CrossThreadTest(void* p) {
    CCoroutineUnit* pTest = (CCoroutineUnit*)p;
    uint32_t nCount = pTest->GetCount();
    
    CCorutinePool* S = new CCorutinePool();
    S->InitCorutine();
    uint32_t nCreate = 10;
    uint32_t nEveryCorutineCount = nCount / nCreate;
    for (uint32_t i = 0; i < nCreate; i++) {
        CCorutine* pCorutine = S->GetCorutine();
        pCorutine->Create([](CCorutine* p) {
            CCoroutineUnit* pTest = p->GetParamPoint<CCoroutineUnit>(0);
            uint32_t nBegin = p->GetParam<uint32_t>(1);
            uint32_t nCount = p->GetParam<uint32_t>(2);
            for (uint32_t i = 0; i < nCount; i++) {
                pTest->Send(nBegin + i);
                pTest->Receive(nBegin + i);
                p->YieldCorutine();
            }
        });
        uint32_t nBegin = nEveryCorutineCount * i;
        pCorutine->Resume(S, pTest, &nBegin, &nEveryCorutineCount);
        //push到另外线程
        pTest->m_pNext->AddCorutine(pCorutine);
    }
    uint32_t nResumeTimes = 0;
    while (true) {
        std::vector<CCorutine*> vtWaitCo;
        pTest->getVTWaitCo(vtWaitCo);
        uint32_t nSize = vtWaitCo.size();
        if (nSize != 0) {
            for (CCorutine* p : vtWaitCo) {
                p->Resume(S);
                if (p->GetCoroutineState() != CoroutineState_Death) {
                    //push到另外线程
                    pTest->m_pNext->AddCorutine(p);
                }
            }
            nResumeTimes += nSize;
            if (nResumeTimes == nCount) {
                break;
            }
        }
        else {
            _mm_pause();
        }
    }
    delete S;
}


void RunAlways(void* p) {
    CCoroutineUnit* pTest = (CCoroutineUnit*)p;

    CCorutinePool* S = new CCorutinePool();
    S->InitCorutine(100);
    uint32_t nCreate = 100;
    for (uint32_t i = 0; i < nCreate; i++) {
        CCorutine* pCorutine = S->GetCorutine();
        pCorutine->Create([](CCorutine* p) {
            CCoroutineUnit* pTest = p->GetParamPoint<CCoroutineUnit>(0);
            while(!pTest->m_bFinish){
                uint32_t nIndex = pTest->GetPushCtxNoNull();
                pTest->Receive(nIndex);
                p->YieldCorutine();
            }
        });
        pCorutine->Resume(S, pTest);
        //push到另外线程
        pTest->m_pNext->AddCorutine(pCorutine);
    }
    while (true) {
        std::vector<CCorutine*> vtWaitCo;
        pTest->getVTWaitCo(vtWaitCo);
        if (vtWaitCo.size() != 0) {
            for (CCorutine* p : vtWaitCo) {
                p->Resume(S);
                if (p->GetCoroutineState() != CoroutineState_Death) {
                    //push到另外线程
                    pTest->m_pNext->AddCorutine(p);
                }
            }
        }
        else {
            if (pTest->m_bFinish)
                break;
            _mm_pause();
        }
    }
    delete S;
}

//use time
#define PrintLockfreeUseTime(calcName, totalPerformance)\
[&](DWORD dwUseTime, DWORD dwTotalUseTime){\
    printf("P:%12.3f/ms Time:%5d/%5d F:%s\n", (double)(totalPerformance)/dwTotalUseTime, dwUseTime, dwTotalUseTime, calcName);\
}

class CCoroutineUnitThreadRunMode {
public:
    CCoroutineUnitThreadRunMode(uint32_t nMaxCountTimeFast, uint32_t nRepeatTimes) {
        m_nMaxCountTimesFast = nMaxCountTimeFast;
        m_nRepeatTimes = nRepeatTimes;
    }
    virtual ~CCoroutineUnitThreadRunMode(){
        
    }

    template<class F>
    bool RepeatCCContainUnitThread(uint32_t nThreadCount, uint32_t maxPushTimes, F func) {
        uint32_t nReceiveThreadCount = nThreadCount;
        for (uint32_t i = 0; i < m_nRepeatTimes; i++) {
            auto p = createUnitThread();
            p->Init(nThreadCount, maxPushTimes);
            if (!func(p, nReceiveThreadCount)) {
                delete p;
                return false;
            }
            delete p;
        }
        return true;
    }

    bool PowerOfTwoThreadCountImpl(uint32_t nThreadCount, CCLOCKFREEQUEUE_THREAD_START_ROUTINE func) {
        bool bRet = true;
        char szBuf[2][32];
        ccsnprintf(szBuf[0], 32, "ThreadCount(%d)", nThreadCount);
        CreateCalcUseTime(begin, PrintLockfreeUseTime(szBuf[0], m_nMaxCountTimesFast / nThreadCount * m_nRepeatTimes * nThreadCount), false);
        RepeatCCContainUnitThread(nThreadCount, m_nMaxCountTimesFast / nThreadCount, [&](CCoroutineUnitThread* pMgr, uint32_t nReceiveThreadCount) {
            bool bFuncRet = true;
            std::thread* pPushThread = new std::thread[nThreadCount];
            StartCalcUseTime(begin);
            for (uint32_t j = 0; j < nThreadCount; j++) {
                pPushThread[j] = std::thread(func, pMgr->GetCoroutineUintByIndex(j));
            }
            for (uint32_t j = 0; j < nThreadCount; j++) {
                pPushThread[j].join();
            }
            EndCalcUseTimeCallback(begin, nullptr);
            if (!pMgr->CheckIsSuccess()) {
                bFuncRet = false;
                printf("check fail\n");
            }
            delete[]pPushThread;
            return bFuncRet;
        });
        CallbackUseTime(begin);
        return bRet;
    }

    bool PowerOfTwoThreadCountTest(CCLOCKFREEQUEUE_THREAD_START_ROUTINE func, uint32_t nMaxThreadCount = 8, uint32_t nMinThreadCount = 1) {
        for (uint32_t nThreadCount = nMinThreadCount; nThreadCount <= nMaxThreadCount; nThreadCount *= 2) {
            if (!PowerOfTwoThreadCountImpl(nThreadCount, func))
                return false;
        }
        return true;
    }

protected:
    virtual CCoroutineUnitThread* createUnitThread() {
        return new CCoroutineUnitThread();
    }
protected:
    uint32_t            m_nRepeatTimes;
    uint32_t            m_nMaxCountTimesFast;
};


//use time
#define PrintEffect(calcName, pMgr)\
[&](DWORD dwUseTime, DWORD dwTotalUseTime){\
    uint32_t nSendTimes = pMgr->GetSendTimes();\
    printf("PushTime: %d Push:%8.3f/ms F:%s\n", nSendTimes, (double)(nSendTimes)/dwTotalUseTime, calcName);\
}

class CCoroutineUnitThreadRunModeTime {
public:
    CCoroutineUnitThreadRunModeTime(uint32_t nMaxCountTimeFast, uint32_t nTotalTimes) {
        m_nMaxCountTimesFast = nMaxCountTimeFast;
        m_nTotalTimes = nTotalTimes;
    }
    virtual ~CCoroutineUnitThreadRunModeTime(){
        
    }

    bool PowerOfTwoThreadCountImpl(uint32_t nThreadCount) {
        bool bRet = true;
        char szBuf[128];
        ccsnprintf(szBuf, 128, "ThreadCount(%d)", nThreadCount);
        CreateCalcUseTime(begin, nullptr, false);

        auto pMgr = createUnitThread();
        pMgr->Init(nThreadCount, m_nMaxCountTimesFast / nThreadCount);

        std::thread* pThread = new std::thread[nThreadCount];
        uint32_t nLastSendTimes = 0;
#define PRINT_TIME 1000;
        uint32_t nCheckTime = PRINT_TIME;
        StartCalcUseTime(begin);
        for (uint32_t j = 0; j < nThreadCount; j++) {
            pThread[j] = std::thread(RunAlways, pMgr->GetCoroutineUintByIndex(j));
        }
        while (!IsTimeEnoughUseTime(begin, m_nTotalTimes, PrintEffect(szBuf, pMgr))) {
            if (!IsTimeEnoughUseTime(begin, nCheckTime, PrintEffect(szBuf, pMgr))) {
                CCSleep(1);
                continue;
            }
            nCheckTime += PRINT_TIME;
        }
        pMgr->SetTimeFinish();

        for (uint32_t j = 0; j < nThreadCount; j++) {
            pThread[j].join();
        }
        if (!pMgr->CheckIsSuccess()) {
            printf("check fail\n");
        }
        delete pMgr;
        delete[]pThread;

        return bRet;
    }

    bool PowerOfTwoThreadCountTest(uint32_t nThreadCount) {
        return PowerOfTwoThreadCountImpl(nThreadCount);
    }

protected:
    virtual CCoroutineUnitThread* createUnitThread() {
        return new CCoroutineUnitThread();
    }
protected:
    uint32_t            m_nTotalTimes;
    uint32_t            m_nMaxCountTimesFast;
};

