#ifndef TC_THREAD_HPP
#define TC_THREAD_HPP

/*
 * to capture important pthread API
 */

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace trace_capture {
  enum class ThreadAPI : uint8_t {
    UNDEFINED,
    PTHREAD_CREATE,
    PTHREAD_JOIN,
    PTHREAD_MUTEX_LOCK,
    PTHREAD_MUTEX_UNLOCK,
    PTHREAD_SPIN_LOCK,
    PTHREAD_SPIN_UNLOCK,
    PTHREAD_BARRIER_INIT,
    PTHREAD_BARRIER_WAIT,
    PTHREAD_BARRIER_DESTROY,
    PTHREAD_COND_WAIT,
    PTHREAD_COND_SIGNAL
  };

  const char* APItoString(ThreadAPI type) {
    switch(type) {
      case ThreadAPI::UNDEFINED:
        return "UNDEFINED";
      case ThreadAPI::PTHREAD_CREATE:
        return "PTHREAD_CREATE";
      case ThreadAPI::PTHREAD_JOIN:
        return "PTHREAD_JOIN";
      case ThreadAPI::PTHREAD_MUTEX_LOCK:
        return "PTHREAD_MUTEX_LOCK";
      case ThreadAPI::PTHREAD_MUTEX_UNLOCK:
        return "PTHREAD_MUTEX_UNLOCK";
      case ThreadAPI::PTHREAD_SPIN_LOCK:
        return "PTHREAD_SPIN_LOCK";
      case ThreadAPI::PTHREAD_SPIN_UNLOCK:
        return "PTHREAD_SPIN_UNLOCK";
      case ThreadAPI::PTHREAD_BARRIER_INIT:
        return "PTHREAD_BARRIER_INIT";
      case ThreadAPI::PTHREAD_BARRIER_WAIT:
        return "PTHREAD_BARRIER_WAIT";
      case ThreadAPI::PTHREAD_BARRIER_DESTROY:
        return "PTHREAD_BARRIER_DESTROY";
      case ThreadAPI::PTHREAD_COND_WAIT:
        return "PTHREAD_COND_WAIT";
      case ThreadAPI::PTHREAD_COND_SIGNAL:
        return "PTHREAD_COND_SIGNAL"; 
      default:
        return "UNKNOWN API";
    }
  }

  std::unordered_map<uint64_t, ThreadAPI> threadAPI;     // address for important threadAPI
  ThreadAPI now = ThreadAPI::UNDEFINED;

  void init_pthread_addr(uint64_t addr) {
    now = ThreadAPI((uint8_t)now + 1);
    threadAPI.insert({addr, now});
  }

  void clean_pthread_addr() {
    threadAPI.clear();
  }

  bool isThreadAPI(uint64_t pc) {
    auto it = threadAPI.find(pc);
    return (it != threadAPI.end());
  }

  std::unordered_map<uint64_t, ThreadID> threadMap;   // map virtual address with threadID
  uint16_t threadCnt = 0;

  std::vector<CurEventID> curEventId;
}

#endif