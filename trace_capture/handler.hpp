#ifndef TC_HANDLER_HPP
#define TC_HANDLER_HPP

#include "logger.hpp"
#include "event.hpp"
#include "encoding.hpp"
#include "thread.hpp"
#include "processor.hpp"

class traceHandler {

/**
 * Handle trace for each thread.
 */

public:
  traceHandler (ThreadID threadId, const std::string& eventDir) :
    curEvent(traceEvent{traceEvent::UndefTag}), threadId(threadId), eventId(0)
  {
    logger = new traceLogger(threadId, eventDir);
  }

  void recordEv(traceEvent newEvent) {
    logger->record(newEvent);
  }

  // void recordRegs(int64_t data, int id) {
  //   regs->update(data, id);
  // }

  // int64_t getRegs(int id) {
  //   return regs->get(id);
  // }

  addr_t getPrePC() {
    return curEvent.pc;
  }

  void recordComp(uint32_t isIOP, insn_bits_t insn, uint64_t pc) {
    // if (curEv.tag != Tag::COMPUTE) {
      if (curEvent.tag != Tag::UNDEFINED)
        logger->record(curEvent);
      curEvent = traceEvent{traceEvent::CompTag, pc, isIOP, isIOP ^ 1, insn};
      eventId++;
    // } else {
    //   curEv.compEvent.iops += isIOP;
    //   curEv.compEvent.flops += isIOP ^ 1;
    // }
  }

  void recordMem(uint64_t vaddr, uint64_t addr, uint64_t bytes, int type, uint64_t pc, uint64_t val) {
    // if (curEv.tag != Tag::MEMORY || curEv.memEvent.type != type || curEv.memEvent.addr + curEv.memEvent.bytes != addr) {
      if (curEvent.tag != Tag::UNDEFINED)
        logger->record(curEvent);
      curEvent = traceEvent{traceEvent::MemTag, pc, MemEvent{vaddr, addr, bytes, val, type}};
      eventId++;
    // } else {
    //   curEv.memEvent.bytes += bytes;
    // }
  }

  void recordComm(uint64_t vaddr, uint64_t addr, uint64_t bytes, uint64_t pc, uint64_t val, CommList* comm) {
    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    curEvent = traceEvent{traceEvent::CommTag, pc, CommEvent{vaddr, addr, bytes, val, comm}};
    eventId++;
  }

  void recordEnd() {
    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    logger->record(traceEvent{traceEvent::EndTag});
  }

  void recordAPI(uint64_t pc) {    
    ThreadAPI type = trace_capture::threadAPI[pc];
    PThread api;
    api.addr = pc;
    api.type = type;

    switch(type) {
      case ThreadAPI::PTHREAD_CREATE:
        api.targetAddr = trace_capture::getArgs(10);
        api.targetId = trace_capture::threadCnt;
        // Pthreadt[api.targetAddr] = api.targetId;
        break;
      case ThreadAPI::PTHREAD_MUTEX_LOCK:
      case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
      case ThreadAPI::PTHREAD_SPIN_LOCK:
      case ThreadAPI::PTHREAD_SPIN_UNLOCK:
      case ThreadAPI::PTHREAD_BARRIER_WAIT:
        api.targetAddr = trace_capture::getArgs(10);
        break;
      case ThreadAPI::PTHREAD_JOIN:
        api.targetId = trace_capture::getIdByPthread(trace_capture::getSATP(), trace_capture::getArgs(10));
        break;
      case ThreadAPI::PTHREAD_BARRIER_INIT:
        api.targetId = trace_capture::getArgs(10);
        break;
      default:
        api.type = ThreadAPI::UNDEFINED;
        break;
    }

    if (api.type == ThreadAPI::UNDEFINED)
      return;

    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    curEvent = traceEvent(traceEvent::PThreadTag, pc, api);
    eventId++;
  }

  void recordEcall(uint64_t pc) {
    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    curEvent = traceEvent(traceEvent::EcallTag, pc, trace_capture::getArgs(17));
    eventId++;
  }

  EventID getEeventID() {
    return eventId;
  }

private:
  traceLogger* logger;    // logger for this thread
  traceEvent curEvent;    // current event for this thread
  ThreadID threadId;      // ThreadID allocated by tracer fot this thread
  EventID eventId;        // EventID of this thread
};

#endif