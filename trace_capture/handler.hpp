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
  traceHandler (traceHandler* kernel, ThreadID threadId, const std::string& eventDir) :
    kernel(kernel), curEvent(traceEvent{traceEvent::UndefTag}), threadId(threadId), eventId(0)
  {
    logger = new traceLogger(threadId, eventDir);
  }

  void recordEv(traceEvent newEvent) {
    logger->record(newEvent);
  }

  addr_t getPrePC() {
    return curEvent.pc;
  }

  void recordComp(uint32_t isIOP, insn_bits_t insn, uint64_t pc) {
    // if (curEv.tag != Tag::COMPUTE) {
      if (curEvent.tag != Tag::UNDEFINED)
        logger->record(curEvent);
      addr_t offset = pc - curEvent.pc;
      curEvent = traceEvent{traceEvent::CompTag, pc, trace_capture::getPC(), isIOP, isIOP ^ 1, insn};
      trace_capture::updatePC(offset);
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
      addr_t offset = pc - curEvent.pc;
      curEvent = traceEvent{traceEvent::MemTag, pc, trace_capture::getPC(), MemEvent{vaddr, addr, bytes, val, type}};
      trace_capture::updatePC(offset);
      eventId++;
    // } else {
    //   curEv.memEvent.bytes += bytes;
    // }
  }

  void recordComm(uint64_t vaddr, uint64_t addr, uint64_t bytes, uint64_t pc, uint64_t val, CommList* comm) {
    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    addr_t offset = pc - curEvent.pc;
    curEvent = traceEvent{traceEvent::CommTag, pc, trace_capture::getPC(), CommEvent{vaddr, addr, bytes, val, comm}};
    trace_capture::updatePC(offset);
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
      case ThreadAPI::PTHREAD_BARRIER_DESTROY:
        api.targetAddr = trace_capture::getArgs(10);
        break;
      case ThreadAPI::PTHREAD_JOIN:
        api.targetId = trace_capture::getIdByPthread(trace_capture::getSATP(), trace_capture::getArgs(10));
        break;
      case ThreadAPI::PTHREAD_BARRIER_INIT:
        api.targetAddr = trace_capture::getArgs(10);
        api.arg = trace_capture::getArgs(12);
        break;
      default:
        api.type = ThreadAPI::UNDEFINED;
        break;
    }

    if (api.type == ThreadAPI::UNDEFINED)
      return;

    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    curEvent = traceEvent(traceEvent::PThreadTag, trace_capture::getPC(), api);
    eventId++;
  }

  void recordToKernel() {
    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    logger->record(traceEvent{traceEvent::ToKernelTag, kernel->logger->getPos(), kernel->eventId});
    eventId++;
  }

  void recordToUser() {
    if (curEvent.tag != Tag::UNDEFINED)
      logger->record(curEvent);
    logger->record(traceEvent{traceEvent::ToUserTag});
    eventId++;
  }

  // void recordEcall(uint64_t pc) {
  //   EventID kernelEv;
  //   if (kernel == NULL) {
  //     kernelEv = eventId + 2;
  //   } else {
  //     kernelEv = kernel->getEeventID() + 1;
  //   }

  //   if (curEvent.tag != Tag::UNDEFINED)
  //     logger->record(curEvent);
  //   curEvent = traceEvent(traceEvent::EcallTag, pc, trace_capture::getArgs(17), kernelEv);
  //   eventId++;
  // }

  EventID getEeventID() {
    return eventId;
  }

private:
  traceHandler* kernel;   // a pointer to the traceHandler of kernel mode, used for the record of ecall
  traceLogger* logger;    // logger for this thread
  traceEvent curEvent;    // current event for this thread
  ThreadID threadId;      // ThreadID allocated by tracer fot this thread
  EventID eventId;        // EventID of this thread
};

#endif