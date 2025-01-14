#ifndef TRACE_CAPTURE_HPP
#define TRACE_CAPTURE_HPP

// the header file expose to spike

#include "csrs.h"
#include "decode.h"
#include "encoding.h"


namespace trace_capture {

  static const unsigned int CSR_TRACE = 0x800;  // id of the csr used by trace capturer.

  extern void init();
  extern void exit();
  extern void recordEvent(uint64_t opc, uint64_t pc);
  extern void recordComp(uint32_t isIOP, uint64_t pc);
  extern void recordMem(uint64_t addr, uint64_t bytes, int type, uint64_t pc);
  extern void recordEnd();
}

/**
 * Trace CSR used by trace capturer.
 * When this CSR is set, spike starts to capture trace.
 */
class trace_csr_t: public csr_t {
public:
  trace_csr_t(processor_t* const proc, const reg_t addr, const reg_t init)
   : csr_t(proc, addr), val(init) {}

  virtual reg_t read() const noexcept override {
    return val;
  }
  
  // bool check_capture() {
  //   return this->read() == 3;
  // }
protected:
  virtual bool unlogged_write(const reg_t val) noexcept override {
    if (val) {
      this->proc->capture = true;
    } else {
      this->proc->capture = false;
      trace_capture::recordEnd();
    }
    this->val = val;
    return true;
  }
private:
  reg_t val;
};

// typedef std::shared_ptr<trace_csr_t> trace_csr_t_p;

#endif