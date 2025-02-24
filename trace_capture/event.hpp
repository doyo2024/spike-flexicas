#ifndef TC_EVENT_HPP
#define TC_EVENT_HPP

#include <cstdint>

#include "encoding.hpp"   // type: insn_bits_t
#include "thread.hpp"

enum class Tag : uint8_t {
  UNDEFINED,
  COMPUTE,        // Computation event, a combination of iops and flops.
  MEMORY,         // A memory request, either read or write.
  PTHREAD,        // Pthread API.
  ECALL,          // ecall
  END_OF_ENENTS   // The end of the event stream.
};

struct CompEvent {
  uint32_t iops;
  uint32_t flops;
};

// enum class ReqType: uint8_t {
//   REQ_READ,
//   REQ_WRITE
// };

struct MemEvent {
  uint64_t vaddr;   // virtual address, just for tests
  uint64_t addr;    // pyhsical address
  uint64_t bytes;   // number of the requested bytes
  uint64_t val;     // just for test
  int type;         // =0 read, =1 write
  // ReqType type;     // read or write
};

struct EndMark {
  char ed = '@';
};

struct PThread {
  uint64_t addr;  // virtual address (pc) of the api
  trace_capture::ThreadAPI type;
  uint64_t targetId;  // currently record the pthread_t, maybe changed soon
  // ThreadID targetId;
  uint64_t targetAddr;
};

struct EcallEvent {
  uint64_t sysId; // system call number
};

struct traceEvent {
  Tag tag;
  uint64_t pc;
  uint64_t tp;        // the register tp, just used for test and debug, may be deleted soon
  uint64_t satp;      // the register satp, just used for test and debug, may be deleted soon
  insn_bits_t insn;   // the instruction, just used for test and debug, may be deleted soon
  union {
    CompEvent   compEvent;
    MemEvent    memEvent;
    EndMark     endMark;
    PThread     pThread;
    EcallEvent  ecall;
  };

  using UndefTagType = std::integral_constant<Tag, Tag::UNDEFINED>;
  using CompTagType = std::integral_constant<Tag, Tag::COMPUTE>;
  using MemTagType = std::integral_constant<Tag, Tag::MEMORY>;
  using EndTagType = std::integral_constant<Tag, Tag::END_OF_ENENTS>;
  using PThreadTagType = std::integral_constant<Tag, Tag::PTHREAD>;
  using EcallTagType = std::integral_constant<Tag, Tag::ECALL>;

  static constexpr auto UndefTag = UndefTagType{};
  static constexpr auto CompTag = CompTagType{};
  static constexpr auto MemTag = MemTagType{};
  static constexpr auto EndTag = EndTagType{};
  static constexpr auto PThreadTag = PThreadTagType{};
  static constexpr auto EcallTag = EcallTagType{};

  traceEvent(UndefTagType) noexcept
    : tag{Tag::UNDEFINED}
  {}

  traceEvent(CompTagType, uint64_t pc, uint32_t iops, uint32_t flops, insn_bits_t insn, uint64_t tp, uint64_t satp) noexcept
    : compEvent{iops, flops}, tag{Tag::COMPUTE}, pc{pc}, insn{insn}, tp{tp}, satp{satp}
  {}

  traceEvent(MemTagType, uint64_t pc, const MemEvent memEv, uint64_t tp, uint64_t satp) noexcept
    : memEvent{memEv}, tag{Tag::MEMORY}, pc{pc}, tp{tp}, satp{satp}
  {}

  traceEvent(EndTagType) noexcept
    : endMark{}, tag{Tag::END_OF_ENENTS}
  {}

  traceEvent(PThreadTagType, uint64_t pc, const PThread api, uint64_t tp) noexcept
    : pThread{api}, tag{Tag::PTHREAD}, tp{tp}
  {}

  traceEvent(EcallTagType, uint64_t pc, uint64_t sysID, uint64_t tp) noexcept
    : ecall{sysID}, tag{Tag::ECALL}, pc{pc}, tp{tp}
  {}
};

#endif