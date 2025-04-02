#include "handler.hpp"
#include "shadowMemory.hpp"

#include <cstring>
#include <iostream>
#include <new>

namespace trace_capture {

  std::vector<traceHandler*> kernelHandlers;    // trace handler for kernel mode
  std::vector<traceHandler*> handlers;          // trace handler for each thread
  testLogger* tlogger;                          // record metadata of threads, just for test now

  std::vector<ThreadID> curThread;
  CoreID coreCnt;
  addr_t CLONE_END;                             // the return address after ecall 435
  addr_t pthreadtSATP;                          // the satp for the pthread_t which is followed now
  addr_t pthreadtAddr;                          // the address of pthread_t which is followed now

  int waitFork;                               // wait for fork() to return

  enum class EventType {
    UNDEFINED,
    COMP_IOP,
    COMP_FLOP,
    JUMP,
    MEMORY,
    END
  };

  int testCnt = 0;
  uint64_t instCnt = 0;
  const uint64_t instLimit = 1ull << 30;

  void newKernel(CoreID coreId) {
    traceHandler* newHandler = new traceHandler(0, coreId, traceDir + "/test-" + std::to_string(testCnt));
    assert(newHandler != nullptr);
    kernelHandlers.push_back(newHandler);
  }

  void newThread(ThreadID threadId) {
    traceHandler* newHandler = new traceHandler(threadId, 255, traceDir + "/test-" + std::to_string(testCnt));
    assert(newHandler != nullptr);
    handlers.push_back(newHandler);
  }

  static EventType eventMap[256]; 

  void init(int nproc) {
    tlogger = new testLogger(traceDir);
    
    coreCnt = nproc;
    tlogger->recordNCores(coreCnt);
    for (CoreID i = 0; i < coreCnt; i++) {
      traceRegs* newRegs = new traceRegs();
      assert(newRegs != nullptr);
      Args.push_back(newRegs);
      curThread.push_back(-1);
    }

    // map opcode with related event
    memset(eventMap, 0, sizeof(eventMap));

    eventMap[0x03] = EventType::MEMORY;
    eventMap[0x23] = EventType::MEMORY;

    eventMap[0x13] = EventType::COMP_IOP;
    eventMap[0x1b] = EventType::COMP_IOP;
    eventMap[0x2f] = EventType::COMP_IOP;
    eventMap[0x33] = EventType::COMP_IOP;
    eventMap[0x37] = EventType::COMP_IOP;
    eventMap[0x3b] = EventType::COMP_IOP;

    eventMap[0x63] = EventType::JUMP;
    eventMap[0x67] = EventType::JUMP;
    eventMap[0x6f] = EventType::JUMP;

    eventMap[0x43] = EventType::COMP_FLOP;
    eventMap[0x47] = EventType::COMP_FLOP;
    eventMap[0x4b] = EventType::COMP_FLOP;
    eventMap[0x4f] = EventType::COMP_FLOP;
    eventMap[0x53] = EventType::COMP_FLOP;
  }

  void clear() {
    tlogger->recordTime(1);
    while (!handlers.empty()) {
      traceHandler* now = handlers.back();
      now->clear();
      delete now;
      handlers.pop_back();
    }

    while (!kernelHandlers.empty()) {
      traceHandler* now = kernelHandlers.back();
      now->clear();
      delete now;
      kernelHandlers.pop_back();
    }

    clearThreadInfo();
    clearSM();

    testCnt++;
  }

  void traceExit() {
    for (auto it : Args) {
      delete it;
    }
    delete tlogger;
  }

  void reset() {
    tlogger->recordInfo("Resetting...");

    for (CoreID i = 0; i < coreCnt; i++) {
      curThread[i] = -1;
      newKernel(i);
    }

    newThread(0); 
    newThread(1); // create handler for the first thread
    threadCnt = 1;
    for (auto it : Args) {
      it->reset();
    }

    waitFork = 0;

    curThread[0] = 1;

    instCnt = 0;

    tlogger->recordTime(0);
  }

  bool updateInstCnt() {
    instCnt++;
    return instCnt >= instLimit;
  }

  void changeCore(ThreadID threadId, CoreID coreId) {
    assert(threadId > 0);
    handlers[threadId]->changeCore(kernelHandlers[coreId]);
    curThread[coreId] = threadId;
    // tlogger->recordThreadOnCOre(threadId, coreId);
  }

  inline traceHandler* getHandler(CoreID coreId) {
    assert(curThread[coreId] >= 0);
    return curThread[coreId] ? handlers[curThread[coreId]] : kernelHandlers[coreId];
  }

  bool threadCheck(addr_t pc, CoreID coreId) {
    if (curThread[coreId] == -1)
      return pc >= KERNEL_ADDR;
    else
      return (pc >= KERNEL_ADDR) && (getHandler(coreId)->getPrePC() < KERNEL_ADDR);
  }

  void threadSwitch(int type, CoreID coreId) {
    if (type) { // switch to kernel
      if (curThread[coreId] > 0)
        getHandler(coreId)->recordToKernel();
      curThread[coreId] = 0;
    } else {    // switch to user mode
      addr_t taskAddr = getSSCRATCH(coreId);   // get the value of sscratch in uer mode
      if (!taskAddr)
        return;
      getHandler(coreId)->recordToUser();
      curThread[coreId] = getIdByTaskStruct(taskAddr);
      if (curThread[coreId] == -1) {  // a new thread, or a thread that should not trace
        if (waitFork && !getArgs(10, coreId)) {
          newThread(++threadCnt);
          waitFork--;
          mapThreadId(getSSCRATCH(coreId), threadCnt);
          curThread[coreId] = threadCnt;
          tlogger->recordThreadInfo(curThread[coreId], getSATP(coreId), getSSCRATCH(coreId), 0);
          changeCore(threadCnt, coreId);
        } else {
          curThread[coreId] = getIdByPthread(getSATP(coreId), getArgs(13, coreId));   // when returned from ecall 435, the value of pthread_t will be recoreded in x13.
        
          if (curThread[coreId] != -1) {
            mapThreadId(getSSCRATCH(coreId), curThread[coreId]);
            changeCore(curThread[coreId], coreId);
            tlogger->recordThreadInfo(curThread[coreId], getSATP(coreId), getSSCRATCH(coreId), 1);
          }
        }
      } else {
        changeCore(curThread[coreId], coreId);
      }
    }
  }

  void recordComp(uint32_t isIOP, insn_bits_t insn, uint64_t pc, CoreID coreId) {
    // recordComp() must be called in compTypeCheck(), so no need to check curThread here.
    getHandler(coreId)->recordComp(isIOP, insn, pc);
  }

  void recordMem(addr_t vaddr, addr_t addr, uint64_t bytes, int type, addr_t pc, uint64_t val, CoreID coreId) {
    if (curThread[coreId] < 0)
      return;
    
    CommList* list = new CommList;
    if (walk(vaddr, curThread[coreId], getHandler(coreId)->getEeventID() + 1, bytes, ReqType(type), list)) {
      getHandler(coreId)->recordComm(vaddr, addr, bytes, pc, list, val);
    } else {
      delete list;
      getHandler(coreId)->recordMem(vaddr, addr, bytes, type, pc, val);
    }

    if (type && vaddr == pthreadtAddr) {
      uint64_t satp = getSATP(coreId);
      if (satp == pthreadtSATP)
        updateValueMap(satp, val, vaddr);
    }
  }

  void recordEnd() {
    for (int i = 0; i < coreCnt; i++)
      kernelHandlers[i]->recordEnd();
    for (int i = 1; i <= threadCnt; i++)
      handlers[i]->recordEnd();
  }

  void recordAPI(uint64_t pc, CoreID coreId) {
    if (curThread[coreId] < 0)
      return;
    
    if (threadAPI[pc] == ThreadAPI::PTHREAD_CREATE) {
      newThread(++threadCnt);
      pthreadtSATP = getSATP(coreId);
      pthreadtAddr = getArgs(10, coreId);
      updateAddrMap(pthreadtSATP, pthreadtAddr, threadCnt);
    } else if (threadAPI[pc] == ThreadAPI::FORK) {
      waitFork++;
    }
    getHandler(coreId)->recordAPI(pc);
  }

  void compTypeCheck(uint64_t opc, insn_bits_t insn, addr_t pc, CoreID coreId) {

    if (curThread[coreId] && threadCheck(pc, coreId))
      threadSwitch(1, coreId);
    if (curThread[coreId] < 0)
      return;

    if (insn_length(opc) == 4) {
      EventType ev = eventMap[opc & 0x7f];
      if (ev == EventType::COMP_IOP) {
        recordComp(1, insn, pc, coreId);
      } else if (ev == EventType::COMP_FLOP) {
        recordComp(0, insn, pc, coreId);
      }

      else if (ev != EventType::MEMORY) {
        recordComp(1, insn, pc, coreId);    // just for test
      }
    } else {
      if ((opc & 0x3) == 0x1 || opc == MATCH_C_ADD || opc == MATCH_C_JALR || opc == MATCH_C_JR || opc == MATCH_C_MV || opc == MATCH_C_SLLI)
        recordComp(1, insn, pc, coreId);
    }

    if (opc == MATCH_MRET || opc == MATCH_SRET)
      threadSwitch(0, coreId);                              // return to user mode from kernel
  }
}