#include "handler.hpp"
#include "shadowMemory.hpp"

#include <cstring>
#include <iostream>
#include <new>

namespace trace_capture {

  std::vector<traceHandler*> handlers;          // trace handler for each thread
  metaLogger* mlogger;                          // record metadata of threads, just for test now

  ThreadID curThread;
  addr_t CLONE_END;                             // the return address after ecall 435
  addr_t pthreadtSATP;                          // the satp for the pthread_t which is followed now
  addr_t pthreadtAddr;                          // the address of pthread_t which is followed now

  enum class EventType {
    UNDEFINED,
    COMP_IOP,
    COMP_FLOP,
    JUMP,
    MEMORY,
    END
  };

  static EventType eventMap[256]; 

  void newThread(ThreadID threadId) {
    traceHandler* newHandler;
    if (!threadId)
      newHandler = new traceHandler(NULL, threadId, traceDir);
    else
      newHandler = new traceHandler(handlers[0], threadId, traceDir);

    assert(newHandler != nullptr);

    handlers.push_back(newHandler);
    // newList();
  }

  void init() {
    traceRegs* newRegs = new traceRegs();
    mlogger = new metaLogger("/home/spike/Desktop/trace");
    assert(newRegs != nullptr);
    Args.push_back(newRegs);

    newThread(0); // create handler for kernel
    newThread(1); // create handler for the first thread
    curThread = 1;
    threadCnt = 1;
    procId = 0;

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

  void exit() {
    while (!handlers.empty()) {
      traceHandler* now = handlers.back();
      delete now;
      handlers.pop_back();
    }
  }

  bool threadCheck(addr_t pc) {
    if (curThread == -1)
      return pc >= KERNEL_ADDR;
    else
      return (pc >= KERNEL_ADDR) && (handlers[curThread]->getPrePC() < KERNEL_ADDR);
  }

  void threadSwitch(int type) {
    if (type) { // switch to kernel
      curThread = 0;
    } else {    // switch to user mode
      addr_t taskAddr = getSSCRATCH();   // get the value of sscratch in uer mode
      if (!taskAddr)
        return;
      curThread = getIdByTaskStruct(taskAddr);
      if (curThread == -1) {  // a new thread, or a thread that should not trace
        curThread = getIdByPthread(getSATP(), getArgs(13));   // when returned from ecall 435, the value of pthread_t will be recoreded in x13.
        if (curThread != -1) {
          mapThreadId(getSSCRATCH(), curThread);
          mlogger->record(curThread, getSATP(), getSSCRATCH());
        }
      }
    }
  }

  void recordComp(uint32_t isIOP, insn_bits_t insn, uint64_t pc) {
    // recordComp() must be called in compTypeCheck(), so no need to check curThread here.
    handlers[curThread]->recordComp(isIOP, insn, pc);
  }

  void recordMem(uint64_t vaddr, uint64_t addr, uint64_t bytes, int type, uint64_t pc, uint64_t val) {
    if (curThread < 0)
      return;
    
    CommList* list = new CommList;
    if (walk(vaddr, curThread, handlers[curThread]->getEeventID() + 1, bytes, ReqType(type), list)) {
      handlers[curThread]->recordComm(vaddr, addr, bytes, pc, val, list);
    } else {
      delete list;
      handlers[curThread]->recordMem(vaddr, addr, bytes, type, pc, val);
    }

    if (type && vaddr == pthreadtAddr) {
      uint64_t satp = getSATP();
      if (satp == pthreadtSATP)
        updateValueMap(satp, val, vaddr);
    }
  }

  void recordEnd() {
    for (int i = 0; i <= threadCnt; i++)
      handlers[i]->recordEnd();
  }

  void recordAPI(uint64_t pc) {
    if (curThread < 0)
      return;
    
    if (threadAPI[pc] == ThreadAPI::PTHREAD_CREATE) {
      newThread(++threadCnt);
      pthreadtSATP = getSATP();
      pthreadtAddr = getArgs(10);
      updateAddrMap(pthreadtSATP, pthreadtAddr, threadCnt);
    }
    handlers[curThread]->recordAPI(pc);
  }

  void recordEcall(uint64_t pc) {
    if (curThread < 0)
      return;
    handlers[curThread]->recordEcall(pc);
  }

  void compTypeCheck(uint64_t opc, insn_bits_t insn, uint64_t pc) {

    if (curThread && threadCheck(pc))
      threadSwitch(1);
    if (curThread < 0)
      return;

    if (insn_length(opc) == 4) {
      EventType ev = eventMap[opc & 0x7f];
      if (ev == EventType::COMP_IOP) {
        recordComp(1, insn, pc);
      } else if (ev == EventType::COMP_FLOP) {
        recordComp(0, insn, pc);
      }

      else if (ev != EventType::MEMORY) {
        recordComp(1, insn, pc);    // just for test
      }
    } else {
      if ((opc & 0x3) == 0x1 || opc == MATCH_C_ADD || opc == MATCH_C_JALR || opc == MATCH_C_JR || opc == MATCH_C_MV || opc == MATCH_C_SLLI)
        recordComp(1, insn, pc);
    }

    if (opc == MATCH_MRET || opc == MATCH_SRET)
      threadSwitch(0);                              // return to user mode from kernel
  }
}