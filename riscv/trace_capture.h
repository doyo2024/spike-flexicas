#ifndef TRACE_CAPTURE_HPP
#define TRACE_CAPTURE_HPP

// the header file expose to spike

#include "csrs.h"
#include "decode.h"
#include "encoding.h"
#include "processor.h"

namespace trace_capture {

  // static const unsigned int START = 0x10450;    // the entrance address of new process.
  extern uint64_t START; // the entrance address
  extern uint64_t MAIN;  // the main function
  extern uint64_t CLONE_END;
  extern uint64_t traceSignal;
  extern bool capture;
  static const unsigned int CSR_TRACE  = 0x800; // id of the csr used by start trace generation.
  static const unsigned int CSR_THREAD = 0x801; // id of the csr used by record addresses of pthreadAPIs.

  extern void init(int nproc);
  extern void init_pthread_addr(uint64_t addr);
  extern void reset();
  extern void clear();
  extern void traceExit();
  extern void recordMem(uint64_t vaddr, uint64_t addr, uint64_t bytes, int type, uint64_t pc, uint64_t val, uint8_t coreId);
  extern void recordEnd();
  extern void recordAPI(uint64_t pc, uint8_t coreId);
  // extern void recordEcall(uint64_t pc);
  extern void compTypeCheck(uint64_t opc, insn_bits_t insn, uint64_t pc, uint8_t coreId);

  extern void recordArgs(int64_t data, int id, uint8_t coreId);
  extern void recordSATP(int64_t data, uint8_t coreId);
  extern void recordSSCRATCH(int64_t data, uint8_t coreId);

  extern void changeCore(int16_t threaId, uint8_t coreId);
  extern void mapThreadId(addr_t taskAddr, int16_t threadId);
  extern bool isThreadAPI(uint64_t pc);

  extern void recordPC(uint64_t paddr, uint8_t coreId);  // record the physical address of pc
  extern void updatePC(uint64_t offset, uint8_t coreId);

  extern bool updateInstCnt();

  void recordEvent(processor_t* proc, uint64_t opc, insn_bits_t insn, uint64_t pc);
  void traceEnd();
}

/**
 * Trace CSR used by trace capturer.
 * The last 2 bits determine whether to start trace generation:
 * The last bit: set by start-test program when each test begins;
 * The second to last bit: set when pc points to the entrance of the test program.
 * When both bits are set, generate trace until the CSR is set to 0 again.
 */
class trace_csr_t: public csr_t {
public:
  trace_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
   : csr_t(proc, addr), val(init) {}

  virtual reg_t read() const noexcept override {
    return val;
  }

protected:
  virtual bool unlogged_write(const reg_t val) noexcept override {
    if (val) {
      this->val |= val;
      trace_capture::traceSignal |= val;
      if (trace_capture::traceSignal == 3) {
        trace_capture::capture = true;
        trace_capture::reset();
        trace_capture::mapThreadId(this->proc->get_state()->csrmap[CSR_SSCRATCH]->read(), 1);   // map the main thread to ID: 1
        trace_capture::changeCore(1, this->proc->coreId);
      }
    } else {
      this->val = 0;
      trace_capture::traceEnd();
      // trace_capture::traceSignal = 0;
      // trace_capture::capture = false;
      // trace_capture::recordEnd();
      // trace_capture::clear();
    }
    return true;
  }
private:
  reg_t val;
};

/**
 * Used to transfer the address of pthread API.
 * It will be set by start-test program after the analysis of tests' symtab finishes.
 */
class thread_csr_t: public csr_t {
public:
  thread_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
   : csr_t(proc, addr), cnt(0), val(init) {}

  virtual reg_t read() const noexcept override {
    return val;
  }
protected:
  virtual bool unlogged_write(const reg_t val) noexcept override {
    this->val = val;
    if (cnt == 0)
    //   trace_capture::CLONE_END = val;
    // else if (cnt == 1)
      trace_capture::START = val;
    else if (cnt == 1)
      trace_capture::MAIN = val;
    else
      trace_capture::init_pthread_addr(val);
    cnt++;
    return true;
  }
private:
  uint8_t cnt;
  reg_t val;
};

#endif