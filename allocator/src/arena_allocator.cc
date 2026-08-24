#include "allocator/arena_allocator.h"

#include <bit>
#include <cassert>
#include <cstdlib>
#include <format>
#include <memory>
#include <stdexcept>

namespace allocator {

ArenaAllocator::ArenaAllocator(std::size_t capacity_bytes)
    : capacity_(capacity_bytes),
      base_(static_cast<std::byte *>(std::malloc(capacity_bytes))) {
  if (base_ == nullptr) {
    throw std::runtime_error(std::format(
        "Insufficient memory to allocate Arena of size {}.", capacity_bytes));
  }
}

ArenaAllocator::~ArenaAllocator() { std::free(base_); }

void *ArenaAllocator::allocate(std::size_t size, std::size_t alignment) {
  assert(std::has_single_bit(alignment));
  if (offset_ >= capacity_) {
    return nullptr;
  }

  void *ptr = base_ + offset_;
  std::size_t space = capacity_ - offset_;
  void *aligned = std::align(alignment, size, ptr, space);
  if (aligned == nullptr) {
    return nullptr;
  }
  offset_ = static_cast<std::byte *>(aligned) - base_ + size;
  return aligned;
}

void ArenaAllocator::reset() { offset_ = 0; }

} // namespace allocator
