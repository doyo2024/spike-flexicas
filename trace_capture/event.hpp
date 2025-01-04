#ifndef TC_EVENT_HPP
#define TC_EVENT_HPP

#include <cstdint>

enum class Tag : uint8_t {
  UNDEFINED,
  COMPUTE,        // Computation event, a combination of iops and flops.
  MEMORY,         // A memory request, either read or write.
  END_OF_ENENTS   // The end of the event stream.
};

struct CompEvent {
  uint32_t iops;
  uint32_t flops;
};

enum class ReqType: uint8_t {
  REQ_READ,
  REQ_WRITE
};

struct MemEvent {
  uint64_t addr;    // pyhsical address
  uint64_t bytes;   // number of the requested bytes
  ReqType type;     // read or write
};

struct EndMark {
  char ed = '@';
};

struct traceEvent {
  Tag tag;
  union {
    CompEvent   compEvent;
    MemEvent    memEvent;
    EndMark     endMark;
  };

  using UndefTagType = std::integral_constant<Tag, Tag::UNDEFINED>;
  using CompTagType = std::integral_constant<Tag, Tag::COMPUTE>;
  using MemTagType = std::integral_constant<Tag, Tag::MEMORY>;
  using EndTagType = std::integral_constant<Tag, Tag::END_OF_ENENTS>;

  static constexpr auto UndefTag = UndefTagType{};
  static constexpr auto CompTag = CompTagType{};
  static constexpr auto MemTag = MemTagType{};
  static constexpr auto EndTag = EndTagType{};

  traceEvent(UndefTagType) noexcept
    : tag{Tag::UNDEFINED}
  {}

  traceEvent(CompTagType, uint32_t iops, uint32_t flops) noexcept
    : compEvent{iops, flops}, tag{Tag::COMPUTE}
  {}

  traceEvent(MemTagType, const MemEvent memEv) noexcept
    : memEvent{memEv}, tag{Tag::MEMORY}
  {}

  traceEvent(EndTagType) noexcept
    : endMark{}, tag{Tag::END_OF_ENENTS}
  {}
};

#endif