#ifndef TC_SHADOWMEMORY_HPP
#define TC_SHADOWMEMORY_HPP

#include <unordered_map>
#include "encoding.hpp"
#include "thread.hpp"

class SMEntry {

/**
 * Shadow Memory Entry
 */

public:
  SMEntry() {
    reset();
  }

  void reset() {
    valid = false;
  }

  void update(ThreadID newThread, ReqType newReq) {
    valid = true;
    thread = newThread;
    req = newReq;
    eventId = trace_capture::curEventId[newThread];
  }

  bool check(ThreadID newThread, ReqType newReq) {  // return ture if dependency exists
    if ((!valid) || newThread == thread) 
      return false;
    if (req == ReqType::REQ_READ && newReq == ReqType::REQ_READ)
      return false;
    return true;
  }

  CurEventID getEvent() {
    return eventId;
  }

private:
  // addr_t paddr;     // physical address
  bool valid;
  CurEventID eventId; 
  ThreadID thread;  // last visitor of the address
  ReqType req;      // request type, read or write
};

template <uint32_t N>
class LLSM {

/**
 * Last Level Shadow Memory
 */

public:
  LLSM() {
    mask = N - 1;
    reset();
  }

  void reset() {
    valid = false;
    for (int i = 0; i < N; i++)
      table[i].reset;
  }

  EventID walk(addr_t addr, ThreadID thread, ReqType req) {
    // op = 0, update; op = 1, check
    EventID ret = 0;
    addr &= mask;
    if (talbe[addr].check(thread, req))
      ret = talbe[addr]
    table[addr].update(thread, req);
    return ret;
  }
private:
  bool valid;
  addr_t mask;
  SMEntry table[N];
};

std::unordered_map<addr_t, LLSM<4096>* > SM;
const addr_t SMmask = ~0xffff;

CurEventID walk(addr_t addr, ThreadID thread, ReqType req) { // maybe deleted soon, implemented in recordComm maybe better.
  auto it = SM.find(addr & SMmask);
  if (it == SM.end()) {
    SM[addr & SMmask] = new LLSM<4096>;
  }
  return SM[addr & SMmask]->walk(addr, thread, req);
}

#endif