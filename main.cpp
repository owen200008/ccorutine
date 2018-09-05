#include "headdefine.h"
#include "debug.h"
#include "ccoroutine.h"
#include "ccoroutinepool.h"
#include "ccorutinetemplate.h"

bool BenchmarkSingle(int nRepeatTimes, int nMinThread, int nMaxThread) {
    bool bRet = true;
    auto pCBasicQueueArrayMode = new CCoroutineUnitThreadRunMode(TIMES_FAST, nRepeatTimes);
    bRet &= pCBasicQueueArrayMode->PowerOfTwoThreadCountTest(SingleCorutineTest, nMaxThread, nMinThread);
    delete pCBasicQueueArrayMode;
    if (bRet == false)
        return bRet;
    return bRet;
}

bool BenchmarkMulti(int nRepeatTimes, int nMinThread, int nMaxThread) {
    bool bRet = true;
    auto pCBasicQueueArrayMode = new CCoroutineUnitThreadRunMode(TIMES_FAST, nRepeatTimes);
    bRet &= pCBasicQueueArrayMode->PowerOfTwoThreadCountTest(MultiCorutineTest, nMaxThread, nMinThread);
    delete pCBasicQueueArrayMode;
    if (bRet == false)
        return bRet;
    return bRet;
}

bool BenchmarkDiGui(int nRepeatTimes, int nMinThread, int nMaxThread) {
    bool bRet = true;
    auto pCBasicQueueArrayMode = new CCoroutineUnitThreadRunMode(TIMES_FAST, nRepeatTimes);
    bRet &= pCBasicQueueArrayMode->PowerOfTwoThreadCountTest(DiGuiCorutineTest, nMaxThread, nMinThread);
    delete pCBasicQueueArrayMode;
    if (bRet == false)
        return bRet;
    return bRet;
}

bool BenchmarkCrossThread(int nRepeatTimes, int nMinThread, int nMaxThread) {
    bool bRet = true;
    auto pCBasicQueueArrayMode = new CCoroutineUnitThreadRunMode(TIMES_FAST, nRepeatTimes);
    bRet &= pCBasicQueueArrayMode->PowerOfTwoThreadCountTest(CrossThreadTest, nMaxThread, nMinThread);
    delete pCBasicQueueArrayMode;
    if (bRet == false)
        return bRet;
    return bRet;
}

bool BenchmarkQueueTime(int nTotalTimes, int nPushThread) {
    bool bRet = true;
    auto pCBasicQueueArrayMode = new CCoroutineUnitThreadRunModeTime(TIMES_FAST, nTotalTimes);
    bRet &= pCBasicQueueArrayMode->PowerOfTwoThreadCountTest(nPushThread);
    delete pCBasicQueueArrayMode;
    if (bRet == false)
        return bRet;
    return bRet;
}

int main(int argc, char* argv[]){
    int nTimes = 3;
    int nRepeatTimes = 5;
    int nMinThread = 4;
    int nMaxThread = 8;
    //Ä¬ÈÏ1·ÖÖÓ
    int nHeavyTestTime = 10 * 1000;
    if (argc >= 2) {
        argv[1];
    }
    if (nTimes <= 0 || nTimes > 100)
        nTimes = 3;
    if (nRepeatTimes <= 0 || nRepeatTimes >= 100)
        nRepeatTimes = 5;
    if (nMinThread <= 0 || nMinThread >= 32)
        nMinThread = 1;
    if (nMaxThread <= 0 || nMaxThread >= 256)
        nMaxThread = 8;
    if (nHeavyTestTime < 1000)
        nHeavyTestTime = 60 * 1000;

    for (int i = 0; i < 100; i++) {
        {
//            printf("/*************************************************************************/\n");
//            printf("Start SingleCorutineTest\n");
//            for (int i = 0; i < nTimes; i++) {
//                if (!BenchmarkSingle(nRepeatTimes, nMinThread, nMaxThread)) {
//                    printf("check fail!\n");
//                    break;
//                }
//            }
//            CCorutinePool::CheckCorutinePoolMgrCorrect();
//            printf("/*************************************************************************/\n");
//            printf("Start MultiCorutineTest\n");
//            for (int i = 0; i < nTimes; i++) {
//                if (!BenchmarkMulti(nRepeatTimes, nMinThread, nMaxThread)) {
//                    printf("check fail!\n");
//                    break;
//                }
//            }
//            CCorutinePool::CheckCorutinePoolMgrCorrect();
//            printf("/*************************************************************************/\n");
//            printf("Start DiGuiCorutineTest\n");
//            for (int i = 0; i < nTimes; i++) {
//                if (!BenchmarkDiGui(nRepeatTimes, nMinThread, nMaxThread)) {
//                    printf("check fail!\n");
//                    break;
//                }
//            }
//            CCorutinePool::CheckCorutinePoolMgrCorrect();
//            printf("/*************************************************************************/\n");
//            printf("Start CrossThreadTest\n");
//            for (int i = 0; i < nTimes; i++) {
//                if (!BenchmarkCrossThread(nRepeatTimes, nMinThread, nMaxThread)) {
//                    printf("check fail!\n");
//                    break;
//                }
//            }
//            CCorutinePool::CheckCorutinePoolMgrCorrect();
            printf("/*************************************************************************/\n");
            printf("Start Heavy\n");
            if (!BenchmarkQueueTime(nHeavyTestTime, nMinThread)) {
                printf("check fail!\n");
            }
            printf("/*************************************************************************/\n");
            CCorutinePool::CheckCorutinePoolMgrCorrect();
        }

    }

	getchar();
	return 0;
}

