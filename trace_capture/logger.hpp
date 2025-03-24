#ifndef TC_LOGGER_HPP
#define TC_LOGGER_HPP

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <queue>

#include "event.hpp"

#define output(width, var) std::hex << std::setw(width) << std::setfill('0') << var

/** record trace in file **/
class traceLogger {
public:
  traceLogger(ThreadID threadId, const std::string& eventDir, size_t BSize = 1024) :
    threadId(threadId), BufferSize(BSize) {
      if (threadId == 0) 
        filename = eventDir + "/kernel.out";
      else
        filename = eventDir + "/trace-" + std::to_string(threadId) + ".out";
      traceFile.open(filename.c_str(), std::ios::out | std::ios::trunc);
      if (!traceFile.is_open()) {
        std::cerr << "Error opening file:" << filename << std::endl;
        assert(0); 
      }
    }

  ~traceLogger() {
    traceFile.close();
  }

  void record(traceEvent newEvent) {
    buffer.push(newEvent);
    if (buffer.size() == BufferSize || newEvent.tag == Tag::END_OF_EVENTS || newEvent.tag == Tag::SWITCH_TO_USER)
      flush();
  }

  uint64_t getPos() {
    return traceFile.tellp();
  }

  // traceEvent *curEvent() {
  //   if (buffer.empty())
  //     return nullptr;
  //   return &buffer.back();
  // }

private:
  ThreadID threadId;

  std::queue<traceEvent> buffer;
  size_t BufferSize;

  std::string filename;
  std::ofstream traceFile;

  void APIinfo(PThread pThread) {
    switch(pThread.type) {
      case ThreadAPI::PTHREAD_CREATE:
      case ThreadAPI::PTHREAD_JOIN:
        traceFile << " " << std::dec << pThread.targetId << std::endl;
        break;
      case ThreadAPI::PTHREAD_MUTEX_LOCK:
      case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
      case ThreadAPI::PTHREAD_SPIN_LOCK:
      case ThreadAPI::PTHREAD_SPIN_UNLOCK:
      case ThreadAPI::PTHREAD_BARRIER_WAIT:
      case ThreadAPI::PTHREAD_BARRIER_DESTROY:
        traceFile << " " << output(16, pThread.targetAddr) << std::endl;
        break;
      case ThreadAPI::PTHREAD_BARRIER_INIT:
        traceFile << " " << output(16, pThread.targetAddr) << " " << std::dec << pThread.arg << std::endl;
        break;
      case ThreadAPI::PTHREAD_COND_WAIT:
      case ThreadAPI::PTHREAD_COND_SIGNAL:
      case ThreadAPI::UNDEFINED:
      default:
        traceFile << std::endl;
        break;
    }
  }

  void flush() {
    while (!buffer.empty()) {
      traceEvent ev = buffer.front();
      buffer.pop();

      switch (ev.tag) {
        case Tag::COMPUTE: 
          traceFile << "0 " << output(16, ev.ppc) << " " << ev.compEvent.iops << " " << ev.compEvent.flops << std::endl; // << " " << std::setw(8) << std::setfill('0') << ev.insn << std::endl;
          break;
        case Tag::MEMORY:
          traceFile << "1 " << output(16, ev.ppc) << " " << ev.memEvent.type << " " << output(16, ev.memEvent.addr) << " " << ev.memEvent.bytes << std::endl; // << " " << output(16, ev.memEvent.val) << std::endl;
          break;
        case Tag::COMMUNICATION:
          traceFile << "2 " << output(16, ev.ppc) << " " << output(16, ev.commEvent.addr) << " " << ev.commEvent.bytes << " "; // << " " << output(16, ev.commEvent.val) << ": ";
          for (auto it = ev.commEvent.comm->begin(); it != ev.commEvent.comm->end(); it++) {
            traceFile << std::dec << it->first << " "  << it->second << " ";
          }
          traceFile << std::endl;
          delete ev.commEvent.comm;
          break;
        case Tag::PTHREAD:
          traceFile << "3 " << output(16, ev.pc) << " " << (uint32_t)(ev.pThread.type);
          APIinfo(ev.pThread);
          break;
        case Tag::SWITCH_TO_KERNEL:
          traceFile << ev.evMark.ch << " " << std::dec << ev.evMark.info << " " << ev.evMark.eventId << std::endl;
          break;  
        case Tag::SWITCH_TO_USER:
        case Tag::END_OF_EVENTS:
          traceFile << ev.evMark.ch << std::endl;
          break;  
        // case Tag::ECALL:
        //   traceFile << "4 " << output(16, ev.pc) << " ECALL " << std::dec << ev.ecall.sysId << " " << ev.ecall.kernelEv << std::endl;
        //   break;
        default:
          std::cerr << "Unexpected Thread Event Type!" << std::endl;
          break;
      }
    }
  }
};

class metaLogger {
  /**
   * record useful metadata, just for test now
   */
public:
  metaLogger(const std::string& eventDir) :
    filename(eventDir + "/metadata.out") {
      traceFile.open(filename.c_str(), std::ios::out | std::ios::trunc);
      if (!traceFile.is_open()) {
        std::cerr << "Error opening file:" << filename << std::endl;
        assert(0); 
      }
    }
  
  ~metaLogger() {
    traceFile.close();
  }

  void record(ThreadID threadId, uint64_t satp, uint64_t sscratch){
    traceFile << std::dec << threadId << " " << output(16, satp) << " " << output(16, sscratch) << std::endl;
  }

private:
  std::string filename;
  std::ofstream traceFile;
};

#endif