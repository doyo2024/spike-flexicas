#ifndef TC_ENCODING_HPP
#define TC_ENCODING_HPP

#define insn_length(x) \
  (((x) & 0x03) < 0x03 ? 2 : \
   ((x) & 0x1f) < 0x1f ? 4 : \
   ((x) & 0x3f) < 0x3f ? 6 : \
   8)

#define MATCH_C_ADD 0x9002
#define MATCH_C_JALR 0x9002
#define MATCH_C_JR 0x8002
#define MATCH_C_MV 0x8002
#define MATCH_C_SLLI 0x2

#endif