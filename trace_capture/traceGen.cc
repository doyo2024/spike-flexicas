#include "logger.hpp"
#include "event.hpp"

#include <iostream>
#include <new>

namespace trace_capture {
  std::vector<traceLogger *> loggers;
  std::vector<traceEvent> curEvent;

  void init(ThreadID threadId, const char* eventDir) {
    traceLogger* newLogger = new traceLogger(threadId, eventDir);

    assert(newLogger != nullptr);

    loggers.push_back(newLogger);
    curEvent.push_back(traceEvent{traceEvent::UndefTag});
  }

  void destroy() {
    while (!loggers.empty()) {
      traceLogger* now = loggers.back();
      delete now;
      loggers.pop_back();
    }
  }

  #define curEv curEvent[threadId]

  void recordComp(ThreadID threadId, uint32_t isIOP) {
    if (curEv.tag != Tag::COMPUTE) {
      if (curEv.tag != Tag::UNDEFINED)
        loggers[threadId]->record(curEv);
      curEv = traceEvent{traceEvent::CompTag, isIOP, isIOP ^ 1};
    } else {
      curEv.compEvent.iops += isIOP;
      curEv.compEvent.flops += isIOP ^ 1;
    }
  }

  void recordMem(ThreadID threadId, uint64_t addr, uint64_t bytes, ReqType type) {
    if (curEv.tag != Tag::MEMORY || curEv.memEvent.type != type || curEv.memEvent.addr + curEv.memEvent.bytes != addr) {
      if (curEv.tag != Tag::UNDEFINED)
        loggers[threadId]->record(curEv);
      curEv = traceEvent{traceEvent::MemTag, MemEvent{addr, bytes, type}};
    } else {
      curEv.memEvent.bytes += bytes;
    }
  }

  void recordEnd(ThreadID threadId) {
    if (curEv.tag != Tag::UNDEFINED)
      loggers[threadId]->record(curEv);
    loggers[threadId]->record(traceEvent{traceEvent::EndTag});
  }
}