#include "ccoroutinepool.h"
#include <time.h>

#ifdef _MSC_VER
extern "C"
{
    extern void coctx_swap(coctx_t *, coctx_t*);
}
#elif defined(__GNUC__)
extern "C" {
    extern void coctx_swap(coctx_t *, coctx_t*) asm("coctx_swap");
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
CCORUTINETLS_Key CreateTLSKey() {
#ifdef _MSC_VER
    return TlsAlloc();
#else
    CCORUTINETLS_Key key;
    pthread_key_create(&key, nullptr);
    return key;
#endif
}

CCORUTINETLS_Key CCorutineThreadData::m_tlsKey = CreateTLSKey();

CCorutineThreadData::CCorutineThreadData() {
    m_dwThreadID = CCorutineGetThreadID();
#ifdef _MSC_VER
    TlsSetValue(m_tlsKey, this);
#else
    pthread_setspecific(m_tlsKey, this)
#endif
}
CCorutineThreadData* CCorutineThreadData::GetTLSValue(){
#ifdef _MSC_VER
    return (CCorutineThreadData*)TlsGetValue(m_tlsKey);
#else
    return (CCorutineThreadData*)pthread_getspecific(m_tlsKey);
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////
CCorutinePoolMgr CCorutinePool::m_poolMgr;
bool CCorutinePool::CheckCorutinePoolMgrCorrect() {
    return m_poolMgr.CheckCorutinePoolMgrCorrect();
}
CCorutinePool::CCorutinePool() {
    m_usRunCorutineStack = 0;
    memset(m_pStackRunCorutine, 0, sizeof(CCorutine*) * CorutinePlus_Max_Stack);
    m_pStackRunCorutine[m_usRunCorutineStack++] = &m_main;
    m_main.m_pRunPool = this;

    m_cap = 0;
    m_queue = nullptr;
    m_nCorutineSize = 0;
    m_nCreateTimes = 0;
    m_nLimitSize = 1024;

    m_bInit = false;
}

CCorutinePool::~CCorutinePool() {
    //释放所有协程
    if (m_queue) {
        if (m_nCorutineSize > 0) {
            m_poolMgr.ReleaseCorutineToGlobal(m_queue, m_nCorutineSize);
        }
        CCORUTINE_FREE(m_queue);
    }
}

//! 扩展队列
void CCorutinePool::AddNewQueue() {
    CCorutine** new_queue = (CCorutine**)CCORUTINE_MALLOC(m_cap * 2 * sizeof(CCorutine*));
    memcpy(new_queue, m_queue, m_cap * sizeof(CCorutine*));
    m_cap *= 2;
    CCORUTINE_FREE(m_queue);
    m_queue = new_queue;
}

bool CCorutinePool::InitCorutine(int nDefaultSize, int nLimitCorutineCount) {
    if (m_bInit)
        return true;
    if (nDefaultSize <= 0 || nLimitCorutineCount < nDefaultSize)
        return false;
    m_bInit = true;
    m_nLimitSize = nLimitCorutineCount;
    //create
    m_cap = nDefaultSize;
    m_queue = (CCorutine**)CCORUTINE_MALLOC(m_cap * sizeof(CCorutine*));

    for (uint32_t i = m_poolMgr.GetCorutineFromGlobal(m_queue, m_cap); i < nDefaultSize; i++) {
        m_queue[i] = CreateCorutine();
    }
    m_nCorutineSize = nDefaultSize;
    return true;
}

CCorutine* CCorutinePool::GetCorutine() {
    CCorutine* pRet = nullptr;
    if (m_nCorutineSize == 0) {
        do {
            //从全局拿空闲的, 最大128
            m_nCorutineSize = m_poolMgr.GetCorutineFromGlobal(m_queue, m_cap > 256 ? 128 : m_cap / 2);
            if (m_nCorutineSize > 0) {
                break;
            }
            if (m_nCreateTimes < m_nLimitSize) {
                //create self
                return CreateCorutine();
            }
            else {
#ifdef _MSC_VER
                SwitchToThread();
#else
                std::this_thread::yield();
#endif
                m_nSleepTimes++;
            }
        } while (true);
    }
    pRet = m_queue[--m_nCorutineSize];
    return pRet;
}

//异常的协程需要回收
void CCorutinePool::ReleaseCorutine(CCorutine* pPTR) {
    if (m_nCorutineSize == m_cap) {
        //默认1/4放入全局,最大512
        uint16_t nSize = m_nCorutineSize > 2048 ? 512 : m_nCorutineSize / 4;
        m_poolMgr.ReleaseCorutineToGlobal(m_queue + m_nCorutineSize  - nSize, nSize);
        //扩大到原来2倍
        if (m_cap < m_nLimitSize) {
            AddNewQueue();
        }
    }
    m_queue[m_nCorutineSize++] = pPTR;
}

void CCorutinePool::ResumeFunc(CCorutine* pNext) {
    CCorutine* pRunCorutine = m_pStackRunCorutine[m_usRunCorutineStack - 1];
    
    m_pStackRunCorutine[m_usRunCorutineStack++] = pNext;
    pNext->m_state = CoroutineState_Running;
    coctx_swap(&pRunCorutine->m_ctx, &pNext->m_ctx);
}

void CCorutinePool::YieldFunc(CCorutine* pCorutine) {
    m_usRunCorutineStack--;
#ifdef _DEBUG
    if (m_usRunCorutineStack <= 0) {
        assert(0);
    }
#endif
    CCorutine* pChange = m_pStackRunCorutine[m_usRunCorutineStack - 1];
    pCorutine->m_state = CoroutineState_Suspend;
    //no need to copy stack
    coctx_swap(&pCorutine->m_ctx, &pChange->m_ctx);
}

void CCorutinePool::FinishFunc(CCorutine* pCorutine) {
    pCorutine->m_state = CoroutineState_Death;
    m_usRunCorutineStack--;
    //ret p
    ReleaseCorutine(pCorutine);

    do {
        coctx_swap(&pCorutine->m_ctx, &m_pStackRunCorutine[m_usRunCorutineStack - 1]->m_ctx);
        assert(0);
        CCOROUTINE_LOGGER("error, finish must be no resume...");
    } while (true);
}

CCorutine* CCorutinePool::CreateCorutine() {
    CCorutine* pRet = ConstructCorutine();
    m_nCreateTimes++;
    m_poolMgr.CreateCorutine(pRet);
    return pRet;
}

CCorutine* CCorutinePool::ConstructCorutine() {
    return new CCorutineWithStack();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
CCorutinePoolMgr::CCorutinePoolMgr() {
    m_cap = 32;
    m_tail = 0;
    m_queue = (CCorutine**)CCORUTINE_MALLOC(m_cap * sizeof(CCorutine*));

    m_CheckTail = 0;
    m_checkQueue = nullptr;
    m_checkCap = 0;

    m_moreCap = m_cap;
    m_moreTail = 0;
    m_moreQueue = (CCorutine**)CCORUTINE_MALLOC(m_moreCap * sizeof(CCorutine*));
}

CCorutinePoolMgr::~CCorutinePoolMgr() {
    for (uint32_t i = 0; i < m_tail; i++) {
        delete m_queue[i];
    }
    if (m_queue){
        CCORUTINE_FREE(m_queue);
    }
}

void CCorutinePoolMgr::CreateCorutine(CCorutine* p) {
    while (m_nLock.exchange(true, std::memory_order_acquire)) {
        _mm_pause();
    }
    m_queue[m_tail++] = p;
    if (m_tail >= m_cap) {
        AddNewQueue();
    }
    m_nLock.store(false, std::memory_order_release);
}

void CCorutinePoolMgr::AddNewQueue() {
    CCorutine** new_queue = (CCorutine**)CCORUTINE_MALLOC(m_cap * 2 * sizeof(CCorutine*));
    memcpy(new_queue, m_queue, m_cap * sizeof(CCorutine*));
    m_tail = m_cap;
    m_cap *= 2;
    CCORUTINE_FREE(m_queue);
    m_queue = new_queue;
}

//! check 
void CCorutinePoolMgr::CheckAllCorutine() {
    if (m_tail == 0)
        return;
    if (m_tail != m_CheckTail) {
        while (m_nLock.exchange(true, std::memory_order_acquire)) {
            _mm_pause();
        }
        if (m_checkCap != m_cap) {
            m_checkCap = m_cap;
            if (m_checkQueue) {
                CCORUTINE_FREE(m_checkQueue);
            }
            m_checkQueue = (CCorutine**)CCORUTINE_MALLOC(m_checkCap * sizeof(CCorutine*));
        }
        m_CheckTail = m_tail;
        memcpy(m_checkQueue, m_queue, m_CheckTail * sizeof(CCorutine*));
        m_nLock.store(false, std::memory_order_release);
    }
    time_t nowTime = time(nullptr);
    for (uint32_t i = 0; i < m_CheckTail; i++) {
        if (CCLockfreequeueUnLikely(m_checkQueue[i]->IsCoroutineError(nowTime))) {
            char szBuf[64];
            m_checkQueue[i]->GetStatus(szBuf, 64);
            CCOROUTINE_LOGGER("Corutine check error, maybe loop or no wakeup state(%d) (%s)", m_checkQueue[i]->GetCoroutineState(), szBuf);
        }
    }
}

//! 加入多余的协程
void CCorutinePoolMgr::ReleaseCorutineToGlobal(CCorutine* pMore[], int nCount) {
    while (m_nLockMore.exchange(true, std::memory_order_acquire)) {
        _mm_pause();
    }
    m_moreTail += nCount;
    if (m_moreTail > m_moreCap) {
        do {
            m_moreCap *= 2;
        } while (m_moreCap <= m_moreTail);
        auto* moreQueue = (CCorutine**)CCORUTINE_MALLOC(m_moreCap * sizeof(CCorutine*));
        memcpy(moreQueue, m_moreQueue, (m_moreTail - nCount) * sizeof(CCorutine*));
        CCORUTINE_FREE(m_moreQueue);
        m_moreQueue = moreQueue;
    }
    memcpy(m_moreQueue + m_moreTail - nCount, pMore, nCount * sizeof(CCorutine*));
    m_nLockMore.store(false, std::memory_order_release);
}
//! 获取多余的协程
int CCorutinePoolMgr::GetCorutineFromGlobal(CCorutine* pMore[], int nCount) {
    while (m_nLockMore.exchange(true, std::memory_order_acquire)) {
        _mm_pause();
    }
    if (m_moreTail == 0) {
        m_nLockMore.store(false, std::memory_order_release);
        return 0;
    }
    int nRet = nCount;
    if (m_moreTail < nCount) {
        nRet = m_moreTail;
    }
    m_moreTail -= nRet;
    memcpy(pMore, m_moreQueue + m_moreTail, nRet * sizeof(CCorutine*));
    m_nLockMore.store(false, std::memory_order_release);
    return nRet;
}

bool CCorutinePoolMgr::CheckCorutinePoolMgrCorrect() {
    if (m_tail == m_moreTail) {
        return true;
    }
    return false;
}