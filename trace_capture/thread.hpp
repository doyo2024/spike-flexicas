#ifndef TC_THREAD_HPP
#define TC_THREAD_HPP

/*
 * to capture important pthread API
 */

#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>

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
    PTHREAD_COND_SIGNAL,
    FORK
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

std::vector<CurEventID> curEventId;

namespace trace_capture {

  std::unordered_map<uint64_t, ThreadAPI> threadAPI;     // address for important threadAPI
  ThreadAPI now = ThreadAPI::UNDEFINED;

  void init_pthread_addr(uint64_t addr) {
    now = ThreadAPI((uint8_t)now + 1);
    threadAPI.insert({addr, now});
  }

  void clean_pthread_addr() {
    threadAPI.clear();
  }

  bool isThreadAPI(addr_t pc) {
    auto it = threadAPI.find(pc);
    return (it != threadAPI.end());
  }

  ThreadID threadCnt;
  std::unordered_map<addr_t, ThreadID> threadMap;   // map virtual address of task struct to the threadID allocated by tracer

  void mapThreadId(addr_t taskAddr, ThreadID threadId) {
    threadMap[taskAddr] = threadId;
  }

  ThreadID getIdByTaskStruct(addr_t taskAddr) {
    auto it = threadMap.find(taskAddr);
    return (it == threadMap.end()) ? -1 : it->second;
  }

  #define pthreadInfo std::pair<uint64_t, uint64_t>

  // TODO: can't use pair as key of unordered_map, map may influnce the effect.
  std::map<pthreadInfo, ThreadID> addrToID;  // map the virtual address of a pthread_t to the threadID allocated by tracer
  std::map<pthreadInfo, ThreadID> valToID;   // map the value of a pthread_t to the threadID allocated by tracer

  void updateAddrMap(uint64_t satp, uint64_t addr, ThreadID threadId) {
    addrToID[std::make_pair(satp, addr)] = threadId;
  }

  void updateValueMap(uint64_t satp, uint64_t value, addr_t addr) {
    valToID[std::make_pair(satp, value)] = addrToID[std::make_pair(satp, addr)];
  }

  ThreadID getIdByPthread(uint64_t satp, uint64_t value) {
    auto it = valToID.find(std::make_pair(satp, value));
    return (it == valToID.end()) ? -1 : it->second;
  }

  void clearThreadInfo() {
    now = ThreadAPI::UNDEFINED;
    threadAPI.clear();
    threadMap.clear();
    addrToID.clear();
    valToID.clear();
  }
}

#endif