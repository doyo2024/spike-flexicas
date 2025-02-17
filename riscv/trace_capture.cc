#include "trace_capture.h"

namespace trace_capture {
  void recordEvent(processor_t* proc, uint64_t opc, insn_bits_t insn, uint64_t pc) {
    if (isThreadAPI(pc)) {
      state_t* state = proc->get_state();
      recordArgs(state->XPR[4], 4);       // record register tp, just for test
      for (int i = 10; i <= 17; i++) {
        recordArgs(state->XPR[i], i);
      }                                   // record the 8 registers used for function argument
      recordAPI(pc);
    } else {
      state_t* state = proc->get_state();
      recordArgs(state->XPR[4], 4);       // record register tp, just for test
      compTypeCheck(opc, insn, pc);
    }
  }
}