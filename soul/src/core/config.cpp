#include "core/config.h"
#include "runtime/runtime.h"

namespace soul::impl
{
  auto get_default_allocator() -> memory::Allocator* { return runtime::get_context_allocator(); }
} // namespace soul::impl
