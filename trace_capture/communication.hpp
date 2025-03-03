#ifndef TC_COMMMUNICATION_HPP
#define TC_COMMMUNICATION_HPP

#include <vector>
#include "encoding.hpp"

namespace trace_capture {
  // std::vector<CommList *> commLists;      // buffer for each thread

  // void newList() {
  //   CommList* list = new CommList;
  //   commLists.push_back(list); 
  // }

  // CommList* getCommList(ThreadID threadId) {
  //   return commLists[threadId];
  // }

  void addComm(ThreadID threadId, CommInfo comm, CommList* list) {
    for (auto it = list->begin(); it != list->end(); it++) {
      if (it->first == comm.first && it->second == comm.second) {
        return;
      }
    }
    list->push_back(comm);
  }

  // void clearComm(ThreadID threadId) {   // clear the buffer after each communication event is recorded
  //   commLists[threadId]->clear();
  // }
}

#endif