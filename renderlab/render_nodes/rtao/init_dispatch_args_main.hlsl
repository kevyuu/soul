#include "render_nodes/rtao/rtao.shared.hlsl"

[[vk::push_constant]] InitDispatchArgsPC push_constant;

[numthreads(1, 1, 1)] 
void cs_main() {
  DispatchArg dispatch_arg = {0, 1, 1};
  RWByteAddressBuffer filter_dispatch_arg_buffer = get_rw_buffer(push_constant.filter_dispatch_arg_buffer);
  filter_dispatch_arg_buffer.Store(0, dispatch_arg);
}
