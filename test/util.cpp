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
int     TestObject::sMagicErrorCount = 0;

int    TestAllocator::allocCountAll = 0;
int    TestAllocator::freeCountAll = 0;
size_t TestAllocator::allocVolumeAll = 0;
void*  TestAllocator::lastAllocation = nullptr;


soul::memory::Allocation TestAllocator::try_allocate(soul_size size, soul_size alignment, const char *tag)
{
	++allocCount;
	++allocCountAll;
	++allocVolumeAll;
	lastAllocation = malloc(size);
	return { lastAllocation, size };
}

soul_size TestAllocator::get_allocation_size(void *addr) const
{
	if (addr == nullptr) return 0;
	return _msize(addr);
}

void TestAllocator::deallocate(void *addr)
{
	if (addr) return;
	const soul_size allocation_size = get_allocation_size(addr);
    ++freeCount;
	allocVolume -= allocation_size;
	++freeCountAll;
	allocVolumeAll -= allocation_size;
	free(addr);
}
