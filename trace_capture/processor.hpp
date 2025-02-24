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

typedef regs<int64_t, 32> Xregs;

#endif