#ifndef TC_LOGGER_HPP
#define TC_LOGGER_HPP

#include "event.hpp"

#define output(var) std::hex << var
#define output_width(width, var) std::hex << std::setw(width) << std::setfill('0') << var

typedef std::queue<std::string> StringBuffer;

/** record trace in file **/
class traceLogger {
public:
  traceLogger(ThreadID threadId, CoreID coreId, const std::string& eventDir, size_t BSize = 1024) :
    threadId(threadId), BufferSize(BSize), filepos(0), logLock(logMtx) {
      if (!std::filesystem::exists(eventDir)) {
        std::filesystem::create_directory(eventDir);
      }

      if (threadId == 0) 
        filename = eventDir + "/kernel-" + std::to_string((int)coreId) + ".out";
      else
        filename = eventDir + "/trace-" + std::to_string(threadId) + ".out";
      traceFile.open(filename.c_str(), std::ios::out | std::ios::trunc);
      if (!traceFile.is_open()) {
        std::cerr << "Error opening file:" << filename << std::endl;
        assert(0); 
      }

      buffer = new eventQueue;
      assert(buffer != nullptr);
      logThread = std::thread(std::bind(&(this->flush), this));
    }

  ~traceLogger() {
    traceFile.close();
  }

  void record(traceEvent newEvent) {
    buffer->push(newEvent);
    if (buffer->size() == BufferSize || newEvent.tag == Tag::END_OF_EVENTS || newEvent.tag == Tag::SWITCH_TO_USER) {
      // flush(this);

      buildOutput(buffer);

      // bufferMtx.lock();
      // outputBuffer.push(buffer);
      // bufferMtx.unlock();

      // buffer = new eventQueue;
      // assert(buffer != nullptr);

      logCond.notify_one();
    }
  }

  uint64_t getPos() {
    return filepos;
    // return traceFile.tellp();
  }

  void join() {
    logThread.join();
  }

private:
  ThreadID threadId;

  eventQueue* buffer;
  // std::queue<eventQueue*> outputBuffer;
  size_t BufferSize;

  std::queue<StringBuffer*> outputBuffer;
  std::mutex bufferMtx;

  std::string filename;
  std::ofstream traceFile;
  uint64_t filepos;

  std::thread logThread;
  std::mutex logMtx;
  std::unique_lock<std::mutex> logLock;
  std::condition_variable logCond;

  // void APIinfo(PThread pThread) {
  //   switch(pThread.type) {
  //     case ThreadAPI::PTHREAD_CREATE:
  //     case ThreadAPI::PTHREAD_JOIN:
  //       traceFile << " " << std::dec << pThread.targetId << std::endl;
  //       break;
  //     case ThreadAPI::PTHREAD_MUTEX_LOCK:
  //     case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
  //     case ThreadAPI::PTHREAD_SPIN_LOCK:
  //     case ThreadAPI::PTHREAD_SPIN_UNLOCK:
  //     case ThreadAPI::PTHREAD_BARRIER_WAIT:
  //     case ThreadAPI::PTHREAD_BARRIER_DESTROY:
  //       traceFile << " " << output(pThread.targetAddr) << std::endl;
  //       break;
  //     case ThreadAPI::PTHREAD_BARRIER_INIT:
  //       traceFile << " " << output(pThread.targetAddr) << " " << std::dec << pThread.arg << std::endl;
  //       break;
  //     case ThreadAPI::PTHREAD_COND_WAIT:
  //     case ThreadAPI::PTHREAD_COND_SIGNAL:
  //     case ThreadAPI::UNDEFINED:
  //     default:
  //       traceFile << std::endl;
  //       break;
  //   }
  // }

  // bool writeTrace(eventQueue* events) {
  //   while(!events->empty()) {
  //     traceEvent ev = events->front();
  //     events->pop();

  //     switch (ev.tag) {
  //       case Tag::COMPUTE: 
  //         // traceFile << output_width(16, ev.tp) << " " << output_width(16, ev.sscratch) << " ";
  //         traceFile << "0 " << output(ev.ppc) << " " << ev.compEvent.iops << std::endl; // << " " << ev.compEvent.flops << " " << std::setw(8) << std::setfill('0') << ev.insn << std::endl;
  //         break;
  //       case Tag::MEMORY:
  //         // traceFile << output_width(16, ev.tp) << " " << output_width(16, ev.sscratch) << " ";
  //         traceFile << "1 " << output(ev.ppc) << " " << ev.memEvent.type << " " << output(ev.memEvent.addr) << " " << ev.memEvent.bytes << std::endl; // << " " << output_width(16, ev.memEvent.val) << std::endl;
  //         break;
  //       case Tag::COMMUNICATION:
  //       // traceFile << output_width(16, ev.tp) << " " << output_width(16, ev.sscratch) << " ";
  //         traceFile << "2 " << output(ev.ppc) << " " << output(ev.commEvent.addr) << " " << ev.commEvent.bytes << " " ; // << output_width(16, ev.commEvent.val) << " ";
  //         for (auto it = ev.commEvent.comm->begin(); it != ev.commEvent.comm->end(); it++) {
  //           traceFile << std::dec << it->first << " "  << it->second << " ";
  //         }
  //         traceFile << std::endl;
  //         delete ev.commEvent.comm;
  //         break;
  //       case Tag::PTHREAD:
  //         traceFile << "! " << output(ev.pc) << " " << (uint32_t)(ev.pThread.type);
  //         APIinfo(ev.pThread);
  //         break;
  //       case Tag::KERNEL:
  //         traceFile << "4 " << std::hex << ev.kernelEvent.iops << " " << ev.kernelEvent.flops << " " << ev.kernelEvent.mems << std::endl;
  //         break;
  //       case Tag::SWITCH_TO_KERNEL:
  //         traceFile << ev.evMark.ch << " " << std::dec << ev.evMark.info << " " << ev.evMark.eventId << " " << (int)ev.evMark.coreId << std::endl;
  //         break;  
  //       case Tag::SWITCH_TO_USER:
  //         traceFile << ev.evMark.ch << std::endl;
  //         break;
  //       case Tag::END_OF_EVENTS:
  //         traceFile << ev.evMark.ch << std::endl;
  //         return false;
  //         break;
  //       default:
  //         std::cerr << "Unexpected Thread Event Type: " << int(ev.tag) << std::endl;
  //         assert(0);
  //         break;
  //     }
  //   }
  //   return true;
  // }

  std::string buildTrace(traceEvent ev) {
    std::stringstream str;
    switch (ev.tag) {
      case Tag::COMPUTE: 
        str << "0 " << output(ev.ppc) << " " << ev.compEvent.iops << std::endl;
        break;
      case Tag::MEMORY:
        str << "1 " << output(ev.ppc) << " " << ev.memEvent.type << " " << output(ev.memEvent.addr) << " " << ev.memEvent.bytes << std::endl;
        break;
      case Tag::COMMUNICATION:
        str << "2 " << output(ev.ppc) << " " << output(ev.commEvent.addr) << " " << ev.commEvent.bytes << " " ;
        for (auto it = ev.commEvent.comm->begin(); it != ev.commEvent.comm->end(); it++) {
          str << std::dec << it->first << " "  << it->second << " ";
        }
        str << std::endl;
        delete ev.commEvent.comm;
        break;
      case Tag::PTHREAD:
        str << "! " << output(ev.pc) << " " << (uint32_t)(ev.pThread.type);
        switch(ev.pThread.type) {
          case ThreadAPI::PTHREAD_CREATE:
          case ThreadAPI::PTHREAD_JOIN:
            str << " " << std::dec << ev.pThread.targetId << std::endl;
            break;
          case ThreadAPI::PTHREAD_MUTEX_LOCK:
          case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
          case ThreadAPI::PTHREAD_SPIN_LOCK:
          case ThreadAPI::PTHREAD_SPIN_UNLOCK:
          case ThreadAPI::PTHREAD_BARRIER_WAIT:
          case ThreadAPI::PTHREAD_BARRIER_DESTROY:
            str << " " << output(ev.pThread.targetAddr) << std::endl;
            break;
          case ThreadAPI::PTHREAD_BARRIER_INIT:
            str << " " << output(ev.pThread.targetAddr) << " " << std::dec << ev.pThread.arg << std::endl;
            break;
          case ThreadAPI::PTHREAD_COND_WAIT:
          case ThreadAPI::PTHREAD_COND_SIGNAL:
          case ThreadAPI::UNDEFINED:
          default:
            str << std::endl;
            break;
        }
        break;
      case Tag::KERNEL:
        str << "4 " << std::hex << ev.kernelEvent.iops << " " << ev.kernelEvent.flops << " " << ev.kernelEvent.mems << std::endl;
        break;
      case Tag::SWITCH_TO_KERNEL:
        str << ev.evMark.ch << " " << std::dec << ev.evMark.info << " " << ev.evMark.eventId << " " << (int)ev.evMark.coreId << std::endl;
        break;  
      case Tag::SWITCH_TO_USER:
      case Tag::END_OF_EVENTS:
        str << ev.evMark.ch << std::endl;
        break;
      default:
        std::cerr << "Unexpected Thread Event Type: " << int(ev.tag) << std::endl;
        assert(0);
        break;
    }
    filepos += str.str().length();
    return str.str();
  }

  void buildOutput(eventQueue* buffer) {
    StringBuffer* sbuffer = new StringBuffer;
    assert(sbuffer != nullptr);

    while (!buffer->empty()) {
      sbuffer->push(buildTrace(buffer->front()));
      buffer->pop();
    }

    bufferMtx.lock();
    outputBuffer.push(sbuffer);
    bufferMtx.unlock();

    delete buffer;
    buffer = new eventQueue;
    assert(buffer != nullptr);
  }

  static void flush(traceLogger* logger) {
    bool work = true;
    while (work) {
      logger->logCond.wait(logger->logLock);

      logger->bufferMtx.lock();
      while (!logger->outputBuffer.empty()) {
        // eventQueue* events = logger->outputBuffer.front();
        StringBuffer* sbuffer = logger->outputBuffer.front();
        logger->outputBuffer.pop();
        logger->bufferMtx.unlock();

        // work = logger->writeTrace(events);
        // delete events;

        while (!sbuffer->empty()) {
          std::string line = sbuffer->front();
          logger->traceFile << line.c_str();
          work = !(line[0] == '$');
          sbuffer->pop();
        }

        logger->bufferMtx.lock();
      }
      logger->bufferMtx.unlock();
    }
  }
};

class testLogger {
  /**
   * record useful metadata, just for test now
   */
public:
  testLogger(const std::string& eventDir) :
    filename(eventDir + "/test.out"), testCnt(0) {
      if (!std::filesystem::exists(eventDir)) {
        std::filesystem::create_directory(eventDir);
      }

      traceFile.open(filename.c_str(), std::ios::out | std::ios::trunc);
      if (!traceFile.is_open()) {
        std::cerr << "Error opening file:" << filename << std::endl;
        assert(0); 
      }
    }
  
  ~testLogger() {
    traceFile.close();
  }

  void recordThreadInfo(ThreadID threadId, uint64_t satp, uint64_t sscratch, int type){
    if (type) {
      traceFile << "Created by pthread_create: ";
    } else {
      traceFile << "Created by fork: ";
    }
    traceFile << std::dec << threadId << " " << output(satp) << " " << output(sscratch) << std::endl;
  }

  void recordTime(int type) {
    if (type) {
      ed = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> time = ed - st;
      traceFile << " Testcase " << testCnt << " Finished! Time: " << time.count() << "s" << std::endl;
      testCnt++;
    }
    else {
      traceFile << "Trace capture started." << std::endl;
      st = std::chrono::high_resolution_clock::now();
    }
  }

  void recordNCores(CoreID coreCnt) {
    traceFile << "Running on " << (int)coreCnt << " cores." << std::endl;
  }

  void recordThreadOnCOre(ThreadID threadId, CoreID coreId) {
    traceFile << "Thread " << threadId << " Running on Core " << (int)coreId << "." << std::endl;
  }

  void recordInfo(std::string str) {
    traceFile << str << std::endl;
  }

private:
  std::string filename;
  std::ofstream traceFile;
  std::chrono::high_resolution_clock::time_point st, ed;
  int testCnt;
};

#endif