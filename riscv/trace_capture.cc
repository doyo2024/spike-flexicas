#include "trace_capture.h"

namespace trace_capture {
  uint64_t START; // the entrance address
  uint64_t MAIN;  // the main function

  void recordEvent(processor_t* proc, uint64_t opc, insn_bits_t insn, uint64_t pc) {
    state_t* state = proc->get_state();

    recordArgs(state->XPR[4], 4);       // record register tp, just for test
    // recordArgs(state->satp->read(), 0); // record CSR satp, just for test right now
    recordArgs(state->csrmap[CSR_SSCRATCH]->read(), 0);       // record CSR sscratch, just for test
    recordArgs(state->XPR[10], 10);     // just for test 

    if (isThreadAPI(pc)) {
      for (int i = 10; i <= 17; i++) {
        recordArgs(state->XPR[i], i);
      }                                   // record the 8 registers used for function argument
      recordAPI(pc);
    // } else if (opc == 0x73) {
    //   state_t* state = proc->get_state();
    //   recordArgs(state->XPR[4], 4);       // record register tp, just for test
    //   recordArgs(state->XPR[17], 17);     // record a7, system call number.
    //   recordEcall(pc);
    } else {
      compTypeCheck(opc, insn, pc);
    }
  }
}