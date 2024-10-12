#include "core/profile.h"
#include "memory/allocator.h"
#include "memory/allocators/proxy_allocator.h"

namespace soul::memory
{
  void ProfileProxy::on_pre_init(StringView name)
  {
    SOUL_MEMPROFILE_REGISTER_ALLOCATOR(name.data());
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
      name_.data(), current_alloc_.tag, allocation.addr, current_alloc_.size);
    return allocation;
  }

  auto ProfileProxy::on_pre_deallocate(const DeallocateParam& dealloc_param) -> DeallocateParam
  {
    if (dealloc_param.addr == nullptr)
    {
      return dealloc_param;
    }
    SOUL_MEMPROFILE_REGISTER_DEALLOCATION(name_.data(), dealloc_param.addr, dealloc_param.size);
    return dealloc_param;
  }

  void ProfileProxy::on_pre_cleanup()
  {
    SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(name_);
  }

} // namespace soul::memory
