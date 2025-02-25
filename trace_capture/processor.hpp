#ifndef TC_PROCESSOR_HPP
#define TC_PROCESSOR_HPP

/**
 * Record information about each processor.
 */

#include <cstdint>
#include <cstring>

template<class T, uint32_t N>
class regs {
public:
  regs() {
    memset(reg, 0,  sizeof(reg));
  }

  void update(T data, int id) {
    reg[id] = data;
  }

  T get(int id) {
    return reg[id];
  }

private:
  T reg[N];
};

/**
 * 0 ~ 31 : x0 ~ x31
 * 32 : satp
 * 33 : sscratch
 */
typedef regs<int64_t, 34> traceRegs; 

namespace trace_capture {
  ProcID procId;
  std::vector<traceRegs *> Args;               // record function arguments for each core

  void recordArgs(int64_t data, int id) {
    Args[procId]->update(data, id);
  }

  void recordSATP(int64_t data) {
    Args[procId]->update(data, 32);
  }

  void recordSSCRATCH(int64_t data) {
    Args[procId]->update(data, 33);
  }

  int64_t getArgs(int id) {
    return Args[procId]->get(id);
  }

  int64_t getTP() {
    return Args[procId]->get(4);
  }

  int64_t getSATP() {
    return Args[procId]->get(32);
  }

  int64_t getSSCRATCH() {
    return Args[procId]->get(33);
  }
}

#endif