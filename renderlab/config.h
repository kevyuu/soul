#include "core/config.h"
#include "runtime/runtime.h"

namespace soul
{
  inline auto get_default_allocator() -> memory::Allocator*
  {
    return runtime::get_context_allocator();
  }
} // namespace soul
