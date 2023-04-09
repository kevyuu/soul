#include "core/dev_util.h"
#include "memory/allocator.h"
#include "memory/allocators/proxy_allocator.h"

namespace soul::memory
{
  auto ProfileProxy::on_pre_init(const char* name) -> void
  {
    SOUL_MEMPROFILE_REGISTER_ALLOCATOR(name);
    name_ = name;
  }

  auto ProfileProxy::on_pre_allocate(const AllocateParam& alloc_param) -> AllocateParam
  {
    current_alloc_ = alloc_param;
    return alloc_param;
  }

  auto ProfileProxy::on_post_allocate(const Allocation allocation) -> Allocation
  {
    SOUL_MEMPROFILE_REGISTER_ALLOCATION(
      _name, _currentAlloc.tag, allocation.addr, _currentAlloc.size);
    return allocation;
  }

  auto ProfileProxy::on_pre_deallocate(const DeallocateParam& dealloc_param) -> DeallocateParam
  {
    if (dealloc_param.addr == nullptr) {
      return dealloc_param;
    }
    SOUL_MEMPROFILE_REGISTER_DEALLOCATION(_name, deallocParam.addr, deallocParam.size);
    return dealloc_param;
  }

  auto ProfileProxy::on_pre_cleanup() -> void { SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(_name); }

} // namespace soul::memory
