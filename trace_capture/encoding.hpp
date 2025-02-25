#ifndef TC_ENCODING_HPP
#define TC_ENCODING_HPP

#include <cstdint>

typedef uint64_t insn_bits_t;
typedef uint64_t addr_t;

typedef uint8_t ProcID;
typedef int16_t ThreadID;
typedef uint32_t CurEventID;
typedef uint64_t EventID;

#define insn_length(x) \
  (((x) & 0x03) < 0x03 ? 2 : \
   ((x) & 0x1f) < 0x1f ? 4 : \
   ((x) & 0x3f) < 0x3f ? 6 : \
   8)

#define MATCH_C_ADD 0x9002
#define MATCH_C_BEQZ 0xc001
#define MATCH_C_BNEZ 0xe001
#define MATCH_C_J 0xa001
#define MATCH_C_JAL 0x2001
#define MATCH_C_JALR 0x9002
#define MATCH_C_JR 0x8002
#define MATCH_C_MV 0x8002
#define MATCH_C_SLLI 0x2
#define MATCH_ECALL 0x73
#define MATCH_MRET 0x30200073
#define MATCH_SRET 0x10200073

// const addr_t KERNEL_ADDR = 0x7fff00000000;
// const addr_t MACHINE_ADDR = 0x80000000;
const addr_t KERNEL_ADDR = 0x80000000;

enum class ReqType: uint8_t {
  REQ_READ,
  REQ_WRITE
};

#endif