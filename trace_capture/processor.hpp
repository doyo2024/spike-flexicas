#ifndef TC_PROCESSOR_HPP
#define TC_PROCESSOR_HPP

/**
 * Record information about each processor.
 */

#include "types.hpp"

template<class T, uint32_t N>
class regs {
public:
  regs() {
    reset();
  }

  void update(T data, int id) {
    reg[id] = data;
  }

  void add(T data, int id) {
    reg[id] += data;
  }

  T get(int id) {
    return reg[id];
  }

  void reset() {
    // Don't reset physical pc, it needs to synchronize with the pc in Spike.
    for (int i = 0; i <= 33; i++)
      reg[i] = 0;
  }

private:
  T reg[N];
};

/**
 * 0 ~ 31 : x0 ~ x31
 * 32 : satp
 * 33 : sscratch
 * 34 : physical PC
 */
typedef regs<int64_t, 35> traceRegs; 

namespace trace_capture {
  std::vector<traceRegs *> Args;               // record function arguments for each core

  void recordArgs(int64_t data, int id, CoreID coreId) {
    Args[coreId]->update(data, id);
  }

  void recordSATP(int64_t data, CoreID coreId) {
    Args[coreId]->update(data, 32);
  }

  void recordSSCRATCH(int64_t data, CoreID coreId) {
    Args[coreId]->update(data, 33);
  }

  int64_t getArgs(int id, CoreID coreId) {
    return Args[coreId]->get(id);
  }

  int64_t getSATP(CoreID coreId) {
    return Args[coreId]->get(32);
  }

  int64_t getSSCRATCH(CoreID coreId) {
    return Args[coreId]->get(33);
  }

  // addr_t physicalPC;                            // the physical address of pc
  
  void recordPC(addr_t paddr, CoreID coreId) {
    Args[coreId]->update(paddr, 34);
  }

  void updatePC(addr_t offset, CoreID coreId) {
    Args[coreId]->add(offset, 34);
  }

  addr_t getPC(CoreID coreId) {
    return Args[coreId]->get(34);
  }
}

#endif