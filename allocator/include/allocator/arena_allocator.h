#pragma once

#include <cstddef>
#include <cstdlib>

namespace allocator {

// Bump-pointer allocator over one pre-allocated block. allocate() moves a
// cursor forward; reset() rewinds the cursor to free everything at once.
// There is no per-object free().
class ArenaAllocator {
public:
  explicit ArenaAllocator(std::size_t capacity_bytes);
  ~ArenaAllocator();
  ArenaAllocator(const ArenaAllocator &) = delete;
  ArenaAllocator &operator=(const ArenaAllocator &) = delete;

  // Returns nullptr if the arena doesn't have room for `size` bytes
  // aligned to `alignment`.
  void *allocate(std::size_t size,
                 std::size_t alignment = alignof(std::max_align_t));

  // Rewinds the bump pointer to the start of the arena. Invalidates all
  // previously returned pointers.
  void reset();

  std::size_t capacity() const { return capacity_; }

  std::size_t used() const { return offset_; }

private:
  std::size_t capacity_;
  std::byte *base_;
  std::size_t offset_ = 0;
};

} // namespace allocator
