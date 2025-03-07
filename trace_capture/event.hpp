#ifndef TC_EVENT_HPP
#define TC_EVENT_HPP

#include <cstdint>

#include "encoding.hpp"   // type: insn_bits_t
#include "thread.hpp"

enum class Tag : uint8_t {
  UNDEFINED,
  COMPUTE,        // Computation event, a combination of iops and flops.
  MEMORY,         // A memory request, either read or write.
  COMMUNICATION,  // Communication between different threads, a special memory request.
  PTHREAD,        // Pthread API.
  ECALL,          // ecall
  END_OF_EVENTS   // The end of the event stream.
};

struct CompEvent {
  uint32_t iops;
  uint32_t flops;
};

struct MemEvent {
  uint64_t vaddr;   // virtual address, just for tests
  uint64_t addr;    // pyhsical address
  uint64_t bytes;   // number of the requested bytes
  uint64_t val;     // just for test
  int type;         // =0 read, =1 write
  // ReqType type;     // read or write
};

struct CommEvent {
  uint64_t vaddr;   // virtual address, just for tests
  uint64_t addr;    // pyhsical address
  uint64_t bytes;   // number of the requested bytes
  uint64_t val;     // just for test
  CommList* comm;
};

struct EndMark {
  char ed = '@';
};

struct PThread {
  // uint64_t addr;  // virtual address (pc) of the api
  ThreadAPI type;
  ThreadID targetId;  
  addr_t targetAddr;
  uint64_t arg;
};

struct EcallEvent {
  uint64_t sysId;   // system call number
  EventID kernelEv; // the EventID after switch to kernel mode
};

struct traceEvent {
  Tag tag;
  uint64_t pc;
  insn_bits_t insn;   // the instruction, just used for test and debug, may be deleted soon
  union {
    CompEvent   compEvent;
    MemEvent    memEvent;
    CommEvent   commEvent;
    EndMark     endMark;
    PThread     pThread;
    EcallEvent  ecall;
  };

  using UndefTagType = std::integral_constant<Tag, Tag::UNDEFINED>;
  using CompTagType = std::integral_constant<Tag, Tag::COMPUTE>;
  using MemTagType = std::integral_constant<Tag, Tag::MEMORY>;
  using CommTagType = std::integral_constant<Tag, Tag::COMMUNICATION>;
  using EndTagType = std::integral_constant<Tag, Tag::END_OF_EVENTS>;
  using PThreadTagType = std::integral_constant<Tag, Tag::PTHREAD>;
  using EcallTagType = std::integral_constant<Tag, Tag::ECALL>;

  static constexpr auto UndefTag = UndefTagType{};
  static constexpr auto CompTag = CompTagType{};
  static constexpr auto MemTag = MemTagType{};
  static constexpr auto CommTag = CommTagType{};
  static constexpr auto EndTag = EndTagType{};
  static constexpr auto PThreadTag = PThreadTagType{};
  static constexpr auto EcallTag = EcallTagType{};

  traceEvent(UndefTagType) noexcept
    : tag{Tag::UNDEFINED}, pc{0}
  {}

  traceEvent(CompTagType, addr_t pc, uint32_t iops, uint32_t flops, insn_bits_t insn) noexcept
    : compEvent{iops, flops}, tag{Tag::COMPUTE}, pc{pc}, insn{insn}
  {}

  traceEvent(MemTagType, addr_t pc, const MemEvent memEv) noexcept
    : memEvent{memEv}, tag{Tag::MEMORY}, pc{pc}
  {}

  traceEvent(CommTagType, addr_t pc, const CommEvent commEv) noexcept
    : commEvent{commEv}, tag{Tag::COMMUNICATION}, pc{pc}
  {}

  traceEvent(EndTagType) noexcept
    : endMark{}, tag{Tag::END_OF_EVENTS}
  {}

  traceEvent(PThreadTagType, addr_t pc, const PThread api) noexcept
    : pThread{api}, tag{Tag::PTHREAD}
  {}

  traceEvent(EcallTagType, addr_t pc, uint64_t sysID, EventID kernelEv) noexcept
    : ecall{sysID, kernelEv}, tag{Tag::ECALL}, pc{pc}
  {}
};

#endif