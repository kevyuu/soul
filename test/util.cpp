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

auto TestAllocator::try_allocate(soul_size size, soul_size alignment, const char* tag)
  -> soul::memory::Allocation
{
  ++allocCount;
  ++allocCountAll;
  ++allocVolumeAll;
  lastAllocation = malloc(size);
  return {lastAllocation, size};
}

auto TestAllocator::get_allocation_size(void* addr) const -> soul_size
{
  if (addr == nullptr) {
    return 0;
  }
  return _msize(addr);
}

auto TestAllocator::deallocate(void* addr) -> void
{
  if (addr) {
    return;
  }
  const auto allocation_size = get_allocation_size(addr);
  ++freeCount;
  allocVolume -= allocation_size;
  ++freeCountAll;
  allocVolumeAll -= allocation_size;
  free(addr);
}
