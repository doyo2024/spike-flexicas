// See LICENSE for license details.

#include "insn_template.h"
#include "insn_macros.h"

namespace trace_capture{
  /*
   * Interface to trace capturer.
   */
  inline void traceCapture_NAME(processor_t* proc, insn_bits_t opc, insn_bits_t insn, uint64_t pc) {
    if (proc->capture) {
        trace_capture::recordEvent(opc, insn, pc);
    }
  }
}

#define DECODE_MACRO_USAGE_LOGGED 0

reg_t fast_rv32i_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 32
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}

reg_t fast_rv64i_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 64
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}

#undef DECODE_MACRO_USAGE_LOGGED
#define DECODE_MACRO_USAGE_LOGGED 1

reg_t logged_rv32i_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 32
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}

reg_t logged_rv64i_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 64
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}

#undef CHECK_REG
#define CHECK_REG(reg) require((reg) < 16)

#undef DECODE_MACRO_USAGE_LOGGED
#define DECODE_MACRO_USAGE_LOGGED 0

reg_t fast_rv32e_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 32
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}

reg_t fast_rv64e_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 64
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}

#undef DECODE_MACRO_USAGE_LOGGED
#define DECODE_MACRO_USAGE_LOGGED 1

reg_t logged_rv32e_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 32
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}

reg_t logged_rv64e_NAME(processor_t* p, insn_t insn, reg_t pc)
{
  #define xlen 64
  reg_t npc = sext_xlen(pc + insn_length(OPCODE));
  #include "insns/NAME.h"
  trace_opcode(p, OPCODE, insn);
  trace_capture::traceCapture_NAME(p, OPCODE, insn.bits(), pc);
  #undef xlen
  return npc;
}
