#ifndef TC_HANDLER_HPP
#define TC_HANDLER_HPP

#include "logger.hpp"
#include "event.hpp"
#include "types.hpp"
#include "thread.hpp"
#include "processor.hpp"

class traceHandler {

/**
 * Handle trace for each thread.
 */

public:
  traceHandler (ThreadID threadId, CoreID coreId, const std::string& eventDir) :
    kernel(nullptr), curEvent(traceEvent{traceEvent::UndefTag}), threadId(threadId), coreId(coreId), eventId(0),
    kEvent({0, 0, 0})
  {
    if (threadId != 0 || coreId != 255)
      logger = new traceLogger(threadId, coreId, eventDir);
    else 
      logger = nullptr;
  }

  void clear() {
    if (logger != nullptr) {
      logger->join();
      delete logger;
    }
  }

  addr_t getPrePC() {
    return curEvent.pc;
  }

  void recordComp(uint32_t isIOP, insn_bits_t insn, uint64_t pc) {
    if (!threadId) {
      kEvent.iops += isIOP;
      kEvent.flops += isIOP ^ 1;
      return;
    }
    // if (curEvent.tag != Tag::UNDEFINED)
    //   logger->record(curEvent);
    // addr_t offset = pc - curEvent.pc;
    curEvent = traceEvent{traceEvent::CompTag, pc, trace_capture::getPC(coreId), isIOP, isIOP ^ 1, insn};
    logger->record(curEvent);
    // curEvent.tp = trace_capture::getArgs(4, coreId);
    // curEvent.sscratch = trace_capture::getSSCRATCH(coreId);
    // trace_capture::updatePC(offset);
    eventId++;
  }

  void recordMem(addr_t vaddr, addr_t addr, uint64_t bytes, int type, addr_t pc, uint64_t val) {
    if (!threadId) {
      kEvent.mems++;
      return;
    }
    // if (curEvent.tag != Tag::UNDEFINED)
    //   logger->record(curEvent);
    // addr_t offset = pc - curEvent.pc;
    curEvent = traceEvent{traceEvent::MemTag, pc, trace_capture::getPC(coreId), MemEvent{vaddr, addr, bytes, val, type}};
    logger->record(curEvent);
    // curEvent.tp = trace_capture::getArgs(4, coreId);
    // curEvent.sscratch = trace_capture::getSSCRATCH(coreId);
    // trace_capture::updatePC(offset);
    eventId++;
  }

  void recordComm(addr_t vaddr, addr_t addr, uint64_t bytes, uint64_t pc, CommList* comm, uint64_t val) {
    if (!threadId) {
      logger->record(traceEvent(traceEvent::InKernelTag, kEvent));
      kEvent = {0, 0, 0};
    }
    //  else if (curEvent.tag != Tag::UNDEFINED)
    //   logger->record(curEvent);
    // addr_t offset = pc - curEvent.pc;
    curEvent = traceEvent{traceEvent::CommTag, pc, trace_capture::getPC(coreId), CommEvent{vaddr, addr, bytes, val, comm}};
    logger->record(curEvent);
    // curEvent.tp = trace_capture::getArgs(4, coreId);
    // curEvent.sscratch = trace_capture::getSSCRATCH(coreId);
    // trace_capture::updatePC(offset);
    eventId++;
  }

  void recordEnd() {
    if (!threadId) {
      logger->record(traceEvent(traceEvent::InKernelTag, kEvent));
      kEvent = {0, 0, 0};
    } 
    // else if (curEvent.tag != Tag::UNDEFINED)
    //   logger->record(curEvent);
    logger->record(traceEvent{traceEvent::EndTag});
  }

  void recordAPI(uint64_t pc) {    
    ThreadAPI type = trace_capture::threadAPI[pc];
    PThread api;
    api.type = type;

    switch(type) {
      case ThreadAPI::PTHREAD_CREATE:
        api.targetAddr = trace_capture::getArgs(10, coreId);
        api.targetId = trace_capture::threadCnt;
        break;
      case ThreadAPI::PTHREAD_MUTEX_LOCK:
      case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
      case ThreadAPI::PTHREAD_SPIN_LOCK:
      case ThreadAPI::PTHREAD_SPIN_UNLOCK:
      case ThreadAPI::PTHREAD_BARRIER_WAIT:
      case ThreadAPI::PTHREAD_BARRIER_DESTROY:
        api.targetAddr = trace_capture::getArgs(10, coreId);
        break;
      case ThreadAPI::PTHREAD_JOIN:
        api.targetId = trace_capture::getIdByPthread(trace_capture::getSATP(coreId), trace_capture::getArgs(10, coreId));
        break;
      case ThreadAPI::PTHREAD_BARRIER_INIT:
        api.targetAddr = trace_capture::getArgs(10, coreId);
        api.arg = trace_capture::getArgs(12, coreId);
        break;
      default:
        api.type = ThreadAPI::UNDEFINED;
        break;
    }

    if (api.type == ThreadAPI::UNDEFINED)
      return;

    // if (curEvent.tag != Tag::UNDEFINED)
    //   logger->record(curEvent);
    curEvent = traceEvent(traceEvent::PThreadTag, trace_capture::getPC(coreId), api);
    logger->record(curEvent);
    eventId++;
  }

  void recordToKernel() {
    // if (curEvent.tag != Tag::UNDEFINED)
    //   logger->record(curEvent);
    logger->record(traceEvent{traceEvent::ToKernelTag, kernel->logger->getPos(), kernel->eventId, kernel->coreId});
    eventId++;
  }

  void recordToUser() {
    assert(!threadId);
    logger->record(traceEvent(traceEvent::InKernelTag, kEvent));
    kEvent = {0, 0, 0};
    logger->record(traceEvent{traceEvent::ToUserTag});
    eventId++;
  }

  EventID getEeventID() {
    return eventId;
  }

  void changeCore(traceHandler* newCore) {
    kernel = newCore;
    coreId = newCore->coreId;
  }

private:
  traceHandler* kernel;   // a pointer to the traceHandler of kernel mode, used for the record of ecall
  traceLogger* logger;    // logger for this thread
  traceEvent curEvent;    // current event for this thread
  ThreadID threadId;      // ThreadID allocated by tracer fot this thread
  CoreID  coreId;         // on which core now
  EventID eventId;        // EventID of this thread
  InKernel  kEvent;       // used to compress kernel event;
};

#endif