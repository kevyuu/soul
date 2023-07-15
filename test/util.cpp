#include "util.h"

int64_t TestObject::sTOCount = 0;
int64_t TestObject::sTOCtorCount = 0;
int64_t TestObject::sTODtorCount = 0;
int64_t TestObject::sTODefaultCtorCount = 0;
int64_t TestObject::sTOArgCtorCount = 0;
int64_t TestObject::sTOCopyCtorCount = 0;
int64_t TestObject::sTOMoveCtorCount = 0;
int64_t TestObject::sTOCopyAssignCount = 0;
int64_t TestObject::sTOMoveAssignCount = 0;
int TestObject::sMagicErrorCount = 0;

int TestAllocator::allocCountAll = 0;
int TestAllocator::freeCountAll = 0;
size_t TestAllocator::allocVolumeAll = 0;
void* TestAllocator::lastAllocation = nullptr;

auto TestAllocator::try_allocate(usize size, usize alignment, const char* tag)
  -> soul::memory::Allocation
{
  if (size == 0) {
    return {nullptr, 0};
  }
  ++allocCount;
  ++allocCountAll;
  lastAllocation = malloc(size);
  const auto allocation_size = get_allocation_size(lastAllocation);
  allocVolume += allocation_size;
  allocVolumeAll += allocation_size;
  return {lastAllocation, size};
}

auto TestAllocator::get_allocation_size(void* addr) const -> usize
{
  if (addr == nullptr) {
    return 0;
  }
  return _msize(addr);
}

auto TestAllocator::deallocate(void* addr) -> void
{
  if (addr == nullptr) {
    return;
  }
  const auto allocation_size = get_allocation_size(addr);
  ++freeCount;
  ++freeCountAll;
  allocVolume -= allocation_size;
  allocVolumeAll -= allocation_size;
  free(addr);
}
