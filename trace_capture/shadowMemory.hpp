#ifndef TC_SHADOWMEMORY_HPP
#define TC_SHADOWMEMORY_HPP

#include <unordered_map>
// #include "encoding.hpp"
#include "communication.hpp"

// class SMEntry {

// /**
//  * Shadow Memory Entry
//  */

// public:
//   SMEntry() {
//     reset();
//   }

//   void reset() {
//     valid = false;
//   }

//   void update(ThreadID threadId, EventID eventId, ReqType newReq) {
//     valid = true;
//     tid = threadId;
//     eid = eventId;
//     req = newReq;
//     // eventId = trace_capture::curEventId[newThread];
//   }

//   CommInfo check(ThreadID threadId) {
//     /**
//      * Check whether communication exists.
//      * if yes, return the ThreadID and EventID of last visitor
//      * else, return 0
//      */
//     if (valid && threadId != tid)
//       return std::make_pair(tid, eid);
//     return std::make_pair(threadId, 0);
//   }

//   // CurEventID getEvent() {
//   //   return eventId;
//   // }

// private:
//   // addr_t paddr;     // physical address
//   bool valid;
//   ThreadID tid;  // last visitor of the address
//   EventID  eid;  // EventID of last visitor
//   ReqType  req;  // request type, read or write
// };

struct SMEntry {
  bool valid;
  ThreadID tid;  // last visitor of the address
  EventID  eid;  // EventID of last visitor
  // ReqType  req;  // request type, read or write
};

template <uint64_t Nbits>
class LLSM {

/**
 * Last Level Shadow Memory
 */

public:
  LLSM() {
    mask = (1 << Nbits) - 1;
    table.resize(1 << Nbits);
  }

  // void reset() {
  //   valid = false;
  //   for (int i = 0; i < N; i++)
  //     table[i].valid = false;
  // }

  // EventID walk(addr_t addr, ThreadID thread, ReqType req) {
  //   // op = 0, update; op = 1, check
  //   EventID ret = 0;
  //   addr &= mask;
  //   if (talbe[addr].check(thread, req))
  //     ret = talbe[addr]
  //   table[addr].update(thread, req);
  //   return ret;
  // }

  void update(addr_t vaddr, ThreadID threadId, EventID eventId) {
   addr_t  pos = vaddr & mask;
    table[pos].valid = true;
    table[pos].tid = threadId;
    table[pos].eid = eventId;
  }

  CommInfo check(addr_t vaddr, ThreadID threadId) {
    addr_t pos = vaddr & mask;
    if (table[pos].valid && table[pos].tid != threadId) {
      return std::make_pair(table[pos].tid, table[pos].eid);
    }
    return std::make_pair(threadId, 0);
  }

private:
  bool valid;
  addr_t mask;
  std::vector<SMEntry> table;
};

namespace trace_capture{

  std::unordered_map<addr_t, LLSM<SMbits>* > SM;
  // const addr_t SMmask = ~0xffff;

  // CurEventID walk(addr_t addr, ThreadID thread, ReqType req) { // maybe deleted soon, implemented in recordComm maybe better.
  //   auto it = SM.find(addr & SMmask);
  //   if (it == SM.end()) {
  //     SM[addr & SMmask] = new LLSM<4096>;
  //   }
  //   return SM[addr & SMmask]->walk(addr, thread, req);
  // }

  bool walk(addr_t vaddr, ThreadID threadId, EventID eventId, int bytes, ReqType req, CommList* list) {
    addr_t pos = vaddr >> SMbits;
    if (SM.find(pos) == SM.end()) {
      SM[pos] = new LLSM<SMbits>;
    }

    bool ret = false;
    if (req == ReqType::REQ_WRITE) {
      for (int i = 0; i < bytes; i++)
        SM[pos]->update(vaddr + i, threadId, eventId);
    } else {
      for (int i = 0; i < bytes; i++) {
        CommInfo comm = SM[pos]->check(vaddr, threadId);
        if (comm.first != threadId) {
          addComm(threadId, comm, list);
          ret = true;
        }
      }
    }
    return ret;
  }
}

#endif