#ifndef TC_LOGGER_HPP
#define TC_LOGGER_HPP

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <queue>

#include "event.hpp"

typedef int16_t ThreadID;
typedef uint64_t EventID;

/** record trace in file **/
class traceLogger {
public:
  traceLogger(ThreadID threadId, const std::string& eventDir, size_t BSize = 1024) :
    threadId(threadId), filename(eventDir + "/trace-" + std::to_string(threadId) + ".out"),
    traceFile(filename.c_str()), eventId(0), BufferSize(BSize) {
      if (!traceFile) {
        std::cerr << "Error opening file:" << filename << std::endl;
        assert(0); 
      }
    }

  ~traceLogger() {
    traceFile.close();
  }

  void record(traceEvent newEvent) {
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

  void flush() {
    while (!buffer.empty()) {
      traceEvent ev = buffer.front();
      buffer.pop();
      switch (ev.tag) {
        case Tag::COMPUTE: 
          traceFile << "0 " << ev.compEvent.iops << " " << ev.compEvent.flops << std::endl;
          ++eventId;
          break;
        case Tag::MEMORY:
          traceFile << "1 " << (ev.memEvent.type == ReqType::REQ_READ ? 0 : 1) << " " << ev.memEvent.addr << " " << ev.memEvent.bytes << std::endl;
          ++eventId;
          break;
        case Tag::END_OF_ENENTS:
          traceFile << ev.endMark.ed << std::endl;
          ++eventId;
          break;
        default:
          std::cerr << "Unexpected Thread Event Type!" << std::endl;
          break;
      }
    }
  }
};

#endif