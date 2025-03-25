#ifndef TC_SHADOWMEMORY_HPP
#define TC_SHADOWMEMORY_HPP

#include <unordered_map>
// #include "encoding.hpp"
#include "communication.hpp"

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
  std::queue<LLSM<SMbits>*> SMList;    // used to recycle the space of shadow memory

  bool walk(addr_t vaddr, ThreadID threadId, EventID eventId, int bytes, ReqType req, CommList* list) {
    addr_t pos = vaddr >> SMbits;
    if (SM.find(pos) == SM.end()) {
      SM[pos] = new LLSM<SMbits>;
      SMList.push(SM[pos]);
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

  void clearSM() {
    while (!SMList.empty()) {
      delete SMList.front();
      SMList.pop();
    }
    
    SM.clear();
  }
}

#endif