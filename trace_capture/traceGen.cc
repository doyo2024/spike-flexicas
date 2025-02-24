#include "logger.hpp"
#include "event.hpp"
#include "encoding.hpp"
#include "thread.hpp"
#include "processor.hpp"

#include <cstring>
#include <iostream>
#include <new>

namespace trace_capture {

  std::vector<traceLogger *> loggers;          // loggers for each thread
  std::vector<traceEvent> curEvent;            // current event for each thread
  std::vector<Xregs *> funcArgs;               // record function arguments for each core
  ThreadID threadId;
  ProcID procId;

  enum class EventType {
    UNDEFINED,
    COMP_IOP,
    COMP_FLOP,
    JUMP,
    MEMORY,
    END
  };

  static EventType eventMap[256]; 

  void init() {
    threadId = 0;
    procId = 0;
    char eventDir[] = "/home/spike/Desktop/trace";
    traceLogger* newLogger = new traceLogger(threadId, eventDir);
    Xregs* newRegs = new Xregs();

    assert(newLogger != nullptr && newRegs != nullptr);

    loggers.push_back(newLogger);
    curEvent.push_back(traceEvent{traceEvent::UndefTag});
    funcArgs.push_back(newRegs);

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
    while (!loggers.empty()) {
      traceLogger* now = loggers.back();
      delete now;
      loggers.pop_back();
    }
  }

  void recordArgs(int64_t data, int id) {
    funcArgs[procId]->update(data, id);
  }

  int64_t getArgs(int id) {
    return funcArgs[procId]->get(id);
  }

  #define curEv curEvent[threadId]

  void recordComp(uint32_t isIOP, insn_bits_t insn, uint64_t pc) {
    // if (curEv.tag != Tag::COMPUTE) {
      if (curEv.tag != Tag::UNDEFINED)
        loggers[threadId]->record(curEv);

    if (curEv.pc >= KERNEL_ADDR && pc < KERNEL_ADDR)
      curEv = traceEvent{traceEvent::CompTag, pc, isIOP, isIOP ^ 1, insn, getArgs(10), getArgs(0)}; // output the return value of ecall, just for tests
    else

      curEv = traceEvent{traceEvent::CompTag, pc, isIOP, isIOP ^ 1, insn, getArgs(4), getArgs(0)};
    // } else {
    //   curEv.compEvent.iops += isIOP;
    //   curEv.compEvent.flops += isIOP ^ 1;
    // }
  }

  void recordMem(uint64_t vaddr, uint64_t addr, uint64_t bytes, int type, uint64_t pc, uint64_t val) {
    // if (curEv.tag != Tag::MEMORY || curEv.memEvent.type != type || curEv.memEvent.addr + curEv.memEvent.bytes != addr) {
      if (curEv.tag != Tag::UNDEFINED)
        loggers[threadId]->record(curEv);
      curEv = traceEvent{traceEvent::MemTag, pc, MemEvent{vaddr, addr, bytes, val, type}, getArgs(4), getArgs(0)};
    // } else {
    //   curEv.memEvent.bytes += bytes;
    // }
  }

  void recordEnd() {
    if (curEv.tag != Tag::UNDEFINED)
      loggers[threadId]->record(curEv);
    loggers[threadId]->record(traceEvent{traceEvent::EndTag});
  }

  void recordAPI(uint64_t pc) {    
    ThreadAPI type = threadAPI[pc];
    PThread api;
    api.addr = pc;
    api.type = type;

    switch(type) {
      case ThreadAPI::PTHREAD_CREATE:
        api.targetAddr = getArgs(10);
        api.targetId = ++threadCnt;
        threadMap[api.targetAddr] = api.targetAddr;
        break;
      case ThreadAPI::PTHREAD_MUTEX_LOCK:
      case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
      case ThreadAPI::PTHREAD_SPIN_LOCK:
      case ThreadAPI::PTHREAD_SPIN_UNLOCK:
      case ThreadAPI::PTHREAD_BARRIER_WAIT:
        api.targetAddr = getArgs(10);
        break;
      case ThreadAPI::PTHREAD_JOIN:
      case ThreadAPI::PTHREAD_BARRIER_INIT:
        api.targetId = getArgs(10);
        break;
      default:
        api.type = ThreadAPI::UNDEFINED;
        break;
    }

    if (api.type == ThreadAPI::UNDEFINED)
      return;

    if (curEv.tag != Tag::UNDEFINED)
      loggers[threadId]->record(curEv);
    curEv = traceEvent(traceEvent::PThreadTag, pc, api, getArgs(4));
  }

  void recordEcall(uint64_t pc) {
    if (curEv.tag != Tag::UNDEFINED)
      loggers[threadId]->record(curEv);
    curEv = traceEvent(traceEvent::EcallTag, pc, getArgs(17), getArgs(4));
  }

  void compTypeCheck(uint64_t opc, insn_bits_t insn, uint64_t pc) {
    // auto it = threadAPI.find(pc);
    // if (it != threadAPI.end()) {  // capture pthread API, maybe moved to other place soon.
    //   recordAPI(it->second, pc);
    // }

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
  }
}