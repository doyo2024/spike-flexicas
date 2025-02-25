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
    threadId(threadId), eventId(0), BufferSize(BSize) {
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
    ++eventId;
    buffer.push(newEvent);
    if (buffer.size() == BufferSize || newEvent.tag == Tag::END_OF_ENENTS)
      flush();
  }

  // traceEvent *curEvent() {
  //   if (buffer.empty())
  //     return nullptr;
  //   return &buffer.back();
  // }

private:
  ThreadID threadId;
  EventID  eventId;

  std::queue<traceEvent> buffer;
  size_t BufferSize;

  std::string filename;
  std::ofstream traceFile;

  void APIinfo(PThread pThread) {
    switch(pThread.type) {
      case ThreadAPI::PTHREAD_CREATE:
      case ThreadAPI::PTHREAD_MUTEX_LOCK:
      case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
      case ThreadAPI::PTHREAD_SPIN_LOCK:
      case ThreadAPI::PTHREAD_SPIN_UNLOCK:
        traceFile << " " << pThread.targetAddr << std::endl;
        break;
      case ThreadAPI::PTHREAD_JOIN:
        traceFile << " " << pThread.targetId << std::endl;
        break;
      case ThreadAPI::PTHREAD_BARRIER_INIT:
      case ThreadAPI::PTHREAD_BARRIER_WAIT:
      case ThreadAPI::PTHREAD_BARRIER_DESTROY:
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
      // if (ev.pc >= KERNEL_ADDR) {
      //   traceFile << "*";
      //   if (buffer.front().pc < KERNEL_ADDR) {
      //     traceFile << std::endl;
      //   }
      //   continue;
      // }

      // if (ev.pc >= MACHINE_ADDR) {
      //   traceFile << "+";
      //   if (buffer.front().pc < MACHINE_ADDR) {
      //     traceFile << std::endl;
      //   }
      //   continue;
      // }

      // if (ev.tp == 0xa5760) {
      //   traceFile << "#";
      //   if (buffer.front().tp != 0xa5760) {
      //     traceFile << std::endl;
      //   }
      //   continue;
      // } // ignore the trace of the start script

      switch (ev.tag) {
        case Tag::COMPUTE: 
          traceFile << "0 " << output(16, ev.pc) << " " << output(16, ev.tp) << " " << output(16, ev.satp) << " " << ev.compEvent.iops << " " << ev.compEvent.flops << " " << std::setw(8) << std::setfill('0') << ev.insn << std::endl;
          break;
        case Tag::MEMORY:
          traceFile << "1 " << output(16, ev.pc) << " " << output(16, ev.tp) << " " << output(16, ev.satp) << " " << ev.memEvent.type << " " << output(16, ev.memEvent.vaddr) << " " << ev.memEvent.bytes << " " << output(16, ev.memEvent.val) << std::endl;
          break;
        case Tag::END_OF_ENENTS:
          traceFile << ev.endMark.ed << std::endl;
          break;
        case Tag::PTHREAD:
          traceFile << "2 " << output(16, ev.pThread.addr) << " " << std::dec << ev.pThread.targetId << " " << output(16, ev.tp)  << " " << APItoString(ev.pThread.type);
          APIinfo(ev.pThread);
          break;
        case Tag::ECALL:
          traceFile << "3 " << output(16, ev.pc) << " " << output(16, ev.tp) << " ECALL " << std::dec << ev.ecall.sysId << std::endl;
          break;
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