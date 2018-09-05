#ifndef INC_COROUTINEPLUS_H
#define INC_COROUTINEPLUS_H

#include "ccoroutinedef.h"


CCTHREADID_DWORD CCorutineGetThreadID();
///////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*coctx_pfn_t)(const char* s);
struct coctx_t{
#if defined(__x86_64__)
    void *regs[13];
#else
    void *regs[8];
#endif

    char *ss_sp;
    coctx_t(){
        memset(this, 0, sizeof(coctx_t));
    }
};
///////////////////////////////////////////////////////////////////////////////////////////////////////

enum CoroutineState{
    CoroutineState_Death = 0,
    CoroutineState_Ready = 1,
    CoroutineState_Running = 2,
    CoroutineState_Suspend = 3,
};

class CCorutinePool;

#define DEFAULT_CCORUTINE_STACK_SIZE 128 * 1024

//! 修改过一个版本使用tuple实现参数传递，不过必须C++14，另外效率上和当前版本效率差距比较大，因此参数采用赋值的形式
class CCorutine {

public:
    typedef void(*coroutine_func)(CCorutine* pCorutinePlus);
public:
    // Diagnostic allocations
    void* operator new(size_t nSize) {
        return CCORUTINE_MALLOC(nSize);
    }
    void operator delete(void* p) {
        CCORUTINE_FREE(p);
    }

    virtual ~CCorutine() = default;

    //! 初始化 coroutine_func
    void Create(coroutine_func pFunc);

    //! 唤醒
    template<class...T>
    void Resume(CCorutinePool* pPool, T...args){
        void* pResumeParam[sizeof...(args)] = { args... };
        m_pSwapCorutineParam = &pResumeParam;
        Resume(pPool);
    }

    //! 睡觉
    template<class...T>
    void YieldCorutine(T...args){
        void* pResumeParam[sizeof...(args)] = { args... };
        m_pSwapCorutineParam = &pResumeParam;
        YieldCorutine();
    }

    template<class T>
    T GetParam(int nParam){
        void* pPTR = ((void**)m_pSwapCorutineParam)[nParam];
        return *((T*)pPTR);
    }
    template<class T>
    T* GetParamPoint(int nParam){
        void* pPTR = ((void**)m_pSwapCorutineParam)[nParam];
        return (T*)pPTR;
    }
    ///////////////////////////////////////////////////////////////////////////////////
    //! 唤醒
    void Resume(CCorutinePool* pPool);

    //! 睡觉
    void YieldCorutine();

    CCorutinePool* GetRunPool() { return m_pRunPool; }
    CoroutineState GetCoroutineState() { return m_state; }


    //! 判断是否是死循环或者没有唤醒
    bool IsCoroutineError(time_t tmNow);

    virtual int GetStatus(char* pBuf, int nLength);
protected:
    CCorutine();

    static void MakeContextFuncLibco(CCorutine* pThis);
    //no call self
    void StartFuncLibco();
protected:
    CoroutineState							m_state;
    coroutine_func 							m_func;
    coctx_t									m_ctx;
    bool                                    m_bCreateStack;
    time_t                                  m_tmResumeTime;
    //resume param
    CCorutinePool*					        m_pRunPool;
    void*									m_pSwapCorutineParam;

    friend class CCorutinePool;
    friend class CCorutinePlusPoolMgr;
};

class CCorutineWithStack : public CCorutine {
public:
    CCorutineWithStack();
    virtual ~CCorutineWithStack() = default;
protected:
    char                                    m_szStack[DEFAULT_CCORUTINE_STACK_SIZE];
};

///////////////////////////////////////////////////////////////////////////////////////////////////


#endif
