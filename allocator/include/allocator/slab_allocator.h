#pragma once

#include <cstddef>

namespace allocator {

// Fixed-size chunk allocator backed by an intrusive free list.
class SlabAllocator {
public:
  SlabAllocator(std::size_t chunk_size, std::size_t chunk_count);
  ~SlabAllocator();
  SlabAllocator(const SlabAllocator &) = delete;
  SlabAllocator &operator=(const SlabAllocator &) = delete;

  // Returns nullptr if every chunk is currently allocated.
  void *allocate();

  // Returns `chunk` to the free list. Passing nullptr is a no-op, mirroring
  // free(). Behavior is undefined if `chunk` did not come from this slab's
  // allocate() or has already been returned.
  void deallocate(void *chunk);

  std::size_t chunk_size() const { return chunk_size_; }

  std::size_t chunk_count() const { return chunk_count_; }

  std::size_t free_count() const { return free_count_; }

private:
  struct FreeNode {
    FreeNode *next;
  };

  std::byte *pool_ = nullptr;
  std::size_t chunk_size_;
  std::size_t chunk_count_;
  FreeNode *free_list_ = nullptr;
  std::size_t free_count_ = 0;
};

} // namespace allocator
