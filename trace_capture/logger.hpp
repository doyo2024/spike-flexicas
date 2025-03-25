#ifndef TC_LOGGER_HPP
#define TC_LOGGER_HPP

#include "event.hpp"

#define output(width, var) std::hex << std::setw(width) << std::setfill('0') << var

/** record trace in file **/
class traceLogger {
public:
  traceLogger(ThreadID threadId, const std::string& eventDir, size_t BSize = 1024) :
    threadId(threadId), BufferSize(BSize), logLock(logMtx) {
      if (!std::filesystem::exists(eventDir)) {
        std::filesystem::create_directory(eventDir);
      }

      if (threadId == 0) 
        filename = eventDir + "/kernel.out";
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
      bufferMtx.lock();
      outputBuffer.push(buffer);
      bufferMtx.unlock();

      buffer = new eventQueue;
      assert(buffer != nullptr);

      logCond.notify_one();
    }
  }

  uint64_t getPos() {
    return traceFile.tellp();
  }

  void join() {
    logThread.join();
  }

private:
  ThreadID threadId;

  eventQueue* buffer;
  std::queue<eventQueue*> outputBuffer;
  size_t BufferSize;
  std::mutex bufferMtx;

  std::string filename;
  std::ofstream traceFile;

  std::thread logThread;
  std::mutex logMtx;
  std::unique_lock<std::mutex> logLock;
  std::condition_variable logCond;

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

  bool writeTrace(eventQueue* events) {
    while(!events->empty()) {
      traceEvent ev = events->front();
      events->pop();

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
          traceFile << ev.evMark.ch << std::endl;
          break;
        case Tag::END_OF_EVENTS:
          traceFile << ev.evMark.ch << std::endl;
          return false;
          break;
        default:
          std::cerr << "Unexpected Thread Event Type: " << int(ev.tag) << std::endl;
          assert(0);
          break;
      }
    }
    return true;
  }

  static void flush(traceLogger* logger) {
    bool work = true;
    while (work) {
      logger->logCond.wait(logger->logLock);

      logger->bufferMtx.lock();
      while (!logger->outputBuffer.empty()) {
        eventQueue* events = logger->outputBuffer.front();
        logger->outputBuffer.pop();
        logger->bufferMtx.unlock();

        work = logger->writeTrace(events);
        delete events;

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

  // void record(ThreadID threadId, uint64_t satp, uint64_t sscratch){
  //   traceFile << std::dec << threadId << " " << output(16, satp) << " " << output(16, sscratch) << std::endl;
  // }

  void recordTime(int type) {
    if (type) {
      ed = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> time = ed - st;
      traceFile << " Testcase " << testCnt << " Finished! Time: " << time.count() << "s" << std::endl;
      testCnt++;
    }
    else {
      st = std::chrono::high_resolution_clock::now();
    }
  }

private:
  std::string filename;
  std::ofstream traceFile;
  std::chrono::high_resolution_clock::time_point st, ed;
  int testCnt;
};

#endif