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

typedef uint64_t EventID;

#define output(width, var) std::hex << std::setw(width) << std::setfill('0') << var

/** record trace in file **/
class traceLogger {
public:
  traceLogger(ThreadID threadId, const std::string& eventDir, size_t BSize = 1024) :
    threadId(threadId), filename(eventDir + "/trace-" + std::to_string(threadId) + ".out"),
    eventId(0), BufferSize(BSize) {
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
      case trace_capture::ThreadAPI::PTHREAD_CREATE:
      case trace_capture::ThreadAPI::PTHREAD_MUTEX_LOCK:
      case trace_capture::ThreadAPI::PTHREAD_MUTEX_UNLOCK:
      case trace_capture::ThreadAPI::PTHREAD_SPIN_LOCK:
      case trace_capture::ThreadAPI::PTHREAD_SPIN_UNLOCK:
        traceFile << " " << pThread.targetAddr << std::endl;
        break;
      case trace_capture::ThreadAPI::PTHREAD_JOIN:
        traceFile << " " << pThread.targetId << std::endl;
        break;
      case trace_capture::ThreadAPI::PTHREAD_BARRIER_INIT:
      case trace_capture::ThreadAPI::PTHREAD_BARRIER_WAIT:
      case trace_capture::ThreadAPI::PTHREAD_BARRIER_DESTROY:
      case trace_capture::ThreadAPI::PTHREAD_COND_WAIT:
      case trace_capture::ThreadAPI::PTHREAD_COND_SIGNAL:
      case trace_capture::ThreadAPI::UNDEFINED:
      default:
        traceFile << std::endl;
        break;
    }
  }

  void flush() {
    while (!buffer.empty()) {
      traceEvent ev = buffer.front();
      buffer.pop();
      if (ev.pc >= KERNEL_ADDR) {
        traceFile << "*";
        if (buffer.front().pc < KERNEL_ADDR) {
          traceFile << std::endl;
        }
        continue;
      }
      switch (ev.tag) {
        case Tag::COMPUTE: 
          traceFile << "0 " << output(16, ev.pc) << " " << ev.compEvent.iops << " " << ev.compEvent.flops << " " << std::setw(8) << std::setfill('0') << ev.insn << " " << output(16, ev.tp) << std::endl;
          break;
        case Tag::MEMORY:
          traceFile << "1 " << output(16, ev.pc) << " " << ev.memEvent.type << " " << ev.memEvent.addr << " " << ev.memEvent.bytes << std::endl;
          break;
        case Tag::END_OF_ENENTS:
          traceFile << ev.endMark.ed << std::endl;
          break;
        case Tag::PTHREAD:
          traceFile << "2 " << output(16, ev.pThread.addr) << " " << trace_capture::APItoString(ev.pThread.type) << " " << output(16, ev.tp);
          APIinfo(ev.pThread);
          break;
        default:
          std::cerr << "Unexpected Thread Event Type!" << std::endl;
          break;
      }
    }
  }
};

#endif