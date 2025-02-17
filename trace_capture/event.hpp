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
  uint64_t addr;    // pyhsical address
  uint64_t bytes;   // number of the requested bytes
  int type;         // =0 read, =1 write
  // ReqType type;     // read or write
};

struct EndMark {
  char ed = '@';
};

struct PThread {
  uint64_t addr;  // virtual address (pc) of the api
  trace_capture::ThreadAPI type;
  ThreadID targetId;
  uint64_t targetAddr;
};

struct traceEvent {
  Tag tag;
  uint64_t pc;
  uint64_t tp;        // the register tp, just used for test and debug, may be deleted soon
  insn_bits_t insn;   // the instruction, just used for test and debug, may be deleted soon
  union {
    CompEvent   compEvent;
    MemEvent    memEvent;
    EndMark     endMark;
    PThread     pThread;
  };

  using UndefTagType = std::integral_constant<Tag, Tag::UNDEFINED>;
  using CompTagType = std::integral_constant<Tag, Tag::COMPUTE>;
  using MemTagType = std::integral_constant<Tag, Tag::MEMORY>;
  using EndTagType = std::integral_constant<Tag, Tag::END_OF_ENENTS>;
  using PThreadTagType = std::integral_constant<Tag, Tag::PTHREAD>;

  static constexpr auto UndefTag = UndefTagType{};
  static constexpr auto CompTag = CompTagType{};
  static constexpr auto MemTag = MemTagType{};
  static constexpr auto EndTag = EndTagType{};
  static constexpr auto PThreadTag = PThreadTagType{};

  traceEvent(UndefTagType) noexcept
    : tag{Tag::UNDEFINED}
  {}

  traceEvent(CompTagType, uint64_t pc, uint32_t iops, uint32_t flops, insn_bits_t insn, uint64_t tp) noexcept
    : compEvent{iops, flops}, tag{Tag::COMPUTE}, pc{pc}, insn{insn}, tp{tp}
  {}

  traceEvent(MemTagType, uint64_t pc, const MemEvent memEv) noexcept
    : memEvent{memEv}, tag{Tag::MEMORY}, pc{pc}
  {}

  traceEvent(EndTagType) noexcept
    : endMark{}, tag{Tag::END_OF_ENENTS}
  {}

  traceEvent(PThreadTagType, uint64_t pc, const PThread api, uint64_t tp) noexcept
    : pThread{api}, tag{Tag::PTHREAD}, tp{tp}
  {}
};

#endif