#include "logger.hpp"
#include "event.hpp"
#include "encoding.hpp"

#include <cstring>
#include <iostream>
#include <new>

namespace trace_capture {
  std::vector<traceLogger *> loggers;
  std::vector<traceEvent> curEvent;
  ThreadID threadId;

  enum class EventType {
    UNDEFINED,
    COMP_IOP,
    COMP_FLOP,
    MEMORY,
    END
  };

  static EventType eventMap[256]; 

  void init() {
    threadId = 0;
    char eventDir[] = "/home/spike/Desktop/trace";
    traceLogger* newLogger = new traceLogger(threadId, eventDir);

    assert(newLogger != nullptr);

    loggers.push_back(newLogger);
    curEvent.push_back(traceEvent{traceEvent::UndefTag});

    // map opcode with related event
    memset(eventMap, 0, sizeof(eventMap));

    eventMap[0x13] = EventType::COMP_IOP;
    eventMap[0x1b] = EventType::COMP_IOP;
    eventMap[0x2f] = EventType::COMP_IOP;
    eventMap[0x33] = EventType::COMP_IOP;
    eventMap[0x3b] = EventType::COMP_IOP;
    eventMap[0x63] = EventType::COMP_IOP;
    eventMap[0x67] = EventType::COMP_IOP;
    eventMap[0x6f] = EventType::COMP_IOP;

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

  #define curEv curEvent[threadId]

  void recordComp(uint32_t isIOP, uint64_t pc) {
    // if (curEv.tag != Tag::COMPUTE) {
      if (curEv.tag != Tag::UNDEFINED)
        loggers[threadId]->record(curEv);
      curEv = traceEvent{traceEvent::CompTag, pc, isIOP, isIOP ^ 1};
    // } else {
    //   curEv.compEvent.iops += isIOP;
    //   curEv.compEvent.flops += isIOP ^ 1;
    // }
  }

  void recordMem(uint64_t addr, uint64_t bytes, int type, uint64_t pc) {
    // if (curEv.tag != Tag::MEMORY || curEv.memEvent.type != type || curEv.memEvent.addr + curEv.memEvent.bytes != addr) {
      if (curEv.tag != Tag::UNDEFINED)
        loggers[threadId]->record(curEv);
      curEv = traceEvent{traceEvent::MemTag, pc, MemEvent{addr, bytes, type}};
    // } else {
    //   curEv.memEvent.bytes += bytes;
    // }
  }

  void recordEnd() {
    if (curEv.tag != Tag::UNDEFINED)
      loggers[threadId]->record(curEv);
    loggers[threadId]->record(traceEvent{traceEvent::EndTag});
  }

  void recordEvent(uint64_t opc, uint64_t pc) {
    if (insn_length(opc) == 4) {
      EventType ev = eventMap[opc & 0x7f];
      if (ev == EventType::COMP_IOP) {
        recordComp(1, pc);
      } else if (ev == EventType::COMP_FLOP) {
        recordComp(0, pc);
      }
    } else {
      if ((opc & 0x3) == 0x1 || opc == MATCH_C_ADD || opc == MATCH_C_JALR || opc == MATCH_C_JR || opc == MATCH_C_MV || opc == MATCH_C_SLLI)
        recordComp(1, pc);
    }
  }
}