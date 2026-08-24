#include "allocator/slab_allocator.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <stdexcept>

namespace allocator {

namespace {

std::size_t normalize_chunk_size(std::size_t chunk_size) {
  constexpr std::size_t kAlign = alignof(std::max_align_t);
  if (chunk_size > SIZE_MAX - kAlign + 1) {
    throw std::runtime_error("Requested chunk size too large.");
  }
  std::size_t size = std::max(chunk_size, sizeof(void *));
  return (size + kAlign - 1) / kAlign * kAlign;
}

} // namespace

SlabAllocator::SlabAllocator(std::size_t chunk_size, std::size_t chunk_count)
    : chunk_size_{normalize_chunk_size(chunk_size)}, chunk_count_{chunk_count},
      free_count_{chunk_count} {
  if (chunk_count_ > SIZE_MAX / chunk_size_) {
    throw std::runtime_error("Requested chunk size and count too large.");
  }
  std::size_t total_bytes = chunk_size_ * chunk_count_;
  if (total_bytes == 0) {
    return;
  }
  pool_ = static_cast<std::byte *>(std::malloc(total_bytes));
  if (pool_ == nullptr) {
    throw std::runtime_error(std::format(
        "Insufficient memory to allocate buffer of size {}.", total_bytes));
  }

  free_list_ = reinterpret_cast<FreeNode *>(pool_);
  FreeNode *cur = free_list_;
  for (std::size_t i = 1; i < chunk_count_; ++i) {
    std::byte *address = pool_ + i * chunk_size_;
    cur->next = reinterpret_cast<FreeNode *>(address);
    cur = cur->next;
  }
  cur->next = nullptr;
}

SlabAllocator::~SlabAllocator() { std::free(pool_); }

void *SlabAllocator::allocate() {
  if (free_list_ == nullptr) {
    return nullptr;
  }

  void *chunk = free_list_;
  free_list_ = free_list_->next;
  --free_count_;
  return chunk;
}

void SlabAllocator::deallocate(void *chunk) {
  if (chunk == nullptr) {
    return;
  }
  assert(chunk >= pool_ && chunk < pool_ + chunk_count_ * chunk_size_);
  assert((static_cast<std::byte *>(chunk) - pool_) % chunk_size_ == 0);
  assert(free_count_ < chunk_count_);

  FreeNode *node = reinterpret_cast<FreeNode *>(chunk);
  node->next = free_list_;
  free_list_ = node;
  ++free_count_;
}

} // namespace allocator
