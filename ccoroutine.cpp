#include "ccoroutinepool.h"
#include <time.h>


CCTHREADID_DWORD CCorutineGetThreadID() {
#ifdef _MSC_VER
    return (DWORD)::GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

#ifdef _MSC_VER
#define EIP 6
#define ESP 7
void coctx_make(coctx_t *ctx, coctx_pfn_t pfn, const void* s1) {
    memset(ctx->regs, 0, sizeof(ctx->regs));
    int *sp = (int*)(ctx->ss_sp + DEFAULT_CCORUTINE_STACK_SIZE);
    sp = (int*)((unsigned long)sp & -16L);
    sp -= 2;
    sp[1] = (int)s1;
    ctx->regs[ESP] = (char*)sp;
    ctx->regs[EIP] = (char*)pfn;
}
#elif defined(__GNUC__)
#if defined(__i386__)
#define EIP 6
#define ESP 7
void coctx_make(coctx_t *ctx, coctx_pfn_t pfn, const void* s1) {
    memset(ctx->regs, 0, sizeof(ctx->regs));
    int *sp = (int*)(ctx->ss_sp + DEFAULT_CCORUTINE_STACK_SIZE);
    sp = (int*)((unsigned long)sp & -16L);
    sp -= 2;
    sp[1] = (int)s1;
    ctx->regs[ESP] = (char*)sp;
    ctx->regs[EIP] = (char*)pfn;
}
#elif defined(__x86_64__)
#define RDI 10
#define RIP 11
#define RSP 12
void coctx_make(coctx_t *ctx, coctx_pfn_t pfn, const void *s1) {
    memset(ctx->regs, 0, sizeof(ctx->regs));
    char* sp = ctx->ss_sp + DEFAULT_CCORUTINE_STACK_SIZE;
    sp = (char*)((unsigned long)sp & -16LL);
    ctx->regs[RSP] = sp - 8;
    ctx->regs[RIP] = (char*)pfn;
    ctx->regs[RDI] = (char*)s1;
}
#endif
#endif
///////////////////////////////////////////////////////////////////////////
void CCorutine::MakeContextFuncLibco(CCorutine* pThis){
    pThis->StartFuncLibco();
}

///////////////////////////////////////////////////////////////////////////
CCorutine::CCorutine() {
    m_state = CoroutineState_Ready;
    m_func = nullptr;
    m_pSwapCorutineParam = nullptr;
    m_pRunPool = nullptr;
}

//! 初始化 coroutine_func
void CCorutine::Create(coroutine_func pFunc) {
    m_state = CoroutineState_Ready;
    m_func = pFunc;
    coctx_make(&m_ctx, (coctx_pfn_t)MakeContextFuncLibco, this);
}

void CCorutine::Resume(CCorutinePool* pPool) {
    //m_tmResumeTime = time(NULL);
    m_pRunPool = pPool;
#ifdef _DEBUG
    if (m_state != CoroutineState_Ready && m_state != CoroutineState_Suspend) {
        assert(0);
        return;
    }
#endif   
    m_pRunPool->ResumeFunc(this);
}

//! 睡觉
void CCorutine::YieldCorutine() {
#ifdef _DEBUG
    char sStackSize = 0;
    int nLength = m_ctx.ss_sp + DEFAULT_CCORUTINE_STACK_SIZE - &sStackSize;
    //10k space
    assert(nLength < DEFAULT_CCORUTINE_STACK_SIZE - 10 * 1024);
    static int nTotalUseStackSize = 0;
    if (nLength > nTotalUseStackSize) {
        CCOROUTINE_LOGGER("CCorutinePlusBase::YieldCorutine MaxStack Size %dK", (int)(nLength / 1024));
        nTotalUseStackSize = nLength;
    }
#endif
    m_pRunPool->YieldFunc(this);
}

void CCorutine::StartFuncLibco() {
    m_func(this);
    m_pRunPool->FinishFunc(this);
}

//! 判断是否是死循环或者没有唤醒
bool CCorutine::IsCoroutineError(time_t tmNow) {
    if (m_state == CoroutineState_Suspend || m_state == CoroutineState_Running)
        //5s
        return tmNow - m_tmResumeTime > 5;
    return false;
}

int CCorutine::GetStatus(char* pBuf, int nLength) {
    return ccsnprintf(pBuf, nLength, "threadid:(%d)", CCorutineGetThreadID());
}
//////////////////////////////////////////////////////////////////////////////////////////////////
CCorutineWithStack::CCorutineWithStack() {
    m_ctx.ss_sp = m_szStack;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

