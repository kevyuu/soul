// NOLINTBEGIN(hicpp-signed-bitwise)
// NOLINTBEGIN(cppcoreguidelines-init-variables)
// NOLINTBEGIN(readability-implicit-bool-conversion)

#include <cstddef>
#include <limits>
#include <random>
#include <span>
#include <string>
#include <variant>

#include "core/cstring.h"
#include "core/log.h"
#include "core/panic.h"
#include "core/profile.h"
#include "core/util.h"
#include "core/vector.h"
#include "gpu/intern/bindless_descriptor_allocator.h"
#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_graph_execution.h"
#include "gpu/render_graph.h"
#include "gpu/system.h"
#include "gpu/type.h"
#include "math/math.h"
#include "memory/allocator.h"
#include "runtime/scope_allocator.h"
#include "runtime/system.h"

// TODO(kevyuu): find out why we cannot put these includes after system include
#include <Windows.h>
#include <dxc/dxcapi.h>
#include <vulkan/vulkan_core.h>
#include <wrl/client.h>

#include <volk.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

static constexpr const char* RESOURCE_HLSL = R"HLSL(

#define SOULSL_CONST_FUNCTION

typedef uint u32;
typedef bool b8;

namespace soul
{
  typedef vector <uint, 2> vec2ui32;

  typedef vector <float, 2> vec2f;
  typedef vector <float, 3> vec3f;
  typedef vector <float, 4> vec4f;

  typedef matrix <float, 3, 3> mat3f;
  typedef matrix <float, 4, 4> mat4f;
}

namespace soulsl
{
  static const uint UINT_MAX = 4294967295;
  struct DescriptorID
  {
      uint id;

      b8 is_null() {
          return id == UINT_MAX;
      }

      b8 is_valid() {
          return id != UINT_MAX;
      }
  };
  static const DescriptorID DESCRIPTOR_ID_NULL = { UINT_MAX };

  typedef float float1;
  typedef vector <float, 2> float2;
  typedef vector <float, 3> float3;
  typedef vector <float, 4> float4;

  typedef double double1;
  typedef vector <double, 2> double2;
  typedef vector <double, 3> double3;
  typedef vector <double, 4> double4;

  typedef uint uint1;
  typedef vector <uint, 2> uint2;
  typedef vector <uint, 3> uint3;
  typedef vector <uint, 4> uint4;

  typedef matrix <float, 3, 3> float3x3;
  typedef matrix <float, 4, 4> float4x4;

  typedef uint64_t address;
  typedef uint64_t duint;

}

[[vk::binding(0, 0)]] ByteAddressBuffer global_buffer_arr[];
[[vk::binding(0, 1)]] SamplerState global_sampler_arr[];
[[vk::binding(0, 2)]] Texture2D global_texture_2d_arr[];
[[vk::binding(0, 2)]] Texture3D global_texture_3d_arr[];
[[vk::binding(0, 2)]] TextureCube global_texture_cube_arr[];
[[vk::binding(0, 3)]] RWTexture2D<float4> global_rw_texture_2d_float4_arr[];

template<typename T>
T get_buffer(soulsl::DescriptorID descriptor_id, uint offset) {
	return global_buffer_arr[descriptor_id.id].Load<T>(offset);
}

template<typename T>
T get_buffer_array(soulsl::DescriptorID descriptor_id, uint index) {
  return global_buffer_arr[descriptor_id.id].Load<T>(index * sizeof(T));
}

SamplerState get_sampler(soulsl::DescriptorID descriptor_id) {
	return global_sampler_arr[descriptor_id.id];
}

Texture2D get_texture_2d(soulsl::DescriptorID descriptor_id) {
	return global_texture_2d_arr[descriptor_id.id];
}

Texture3D get_texture_3d(soulsl::DescriptorID descriptor_id) {
  return global_texture_3d_arr[descriptor_id.id];
}

TextureCube get_texture_cube(soulsl::DescriptorID descriptor_id) {
  return global_texture_cube_arr[descriptor_id.id];
}

RWTexture2D<float4> get_rw_texture_2d_float4(soulsl::DescriptorID descriptor_id) {
  return global_rw_texture_2d_float4_arr[descriptor_id.id];
}

)HLSL";
static constexpr auto RESOURCE_HLSL_SIZE = std::char_traits<char>::length(RESOURCE_HLSL);

static constexpr const char* RESOURCE_RT_EXT_HLSL = R"HLSL(

[[vk::binding(0, 4)]] RaytracingAccelerationStructure global_as_arr[];

RaytracingAccelerationStructure get_as(soulsl::DescriptorID descriptor_id) {
  return global_as_arr[descriptor_id.id];
  return global_as_arr[descriptor_id.id];
}

)HLSL";

static constexpr auto RESOURCE_RT_EXT_HLSL_SIZE =
  std::char_traits<char>::length(RESOURCE_RT_EXT_HLSL);

namespace soul::gpu
{
  using namespace impl;

  namespace wrl = Microsoft::WRL;

  struct DxcSession {
    wrl::ComPtr<IDxcLibrary> library;
    wrl::ComPtr<IDxcCompiler3> compiler;
    wrl::ComPtr<IDxcUtils> utils;
    wrl::ComPtr<IDxcIncludeHandler> default_include_handler;
  };

  namespace
  {
    auto pick_surface_extent(const VkSurfaceCapabilitiesKHR capabilities, const vec2ui32 dimension)
      -> VkExtent2D
    {
      SOUL_LOG_INFO("Picking vulkan swap extent");
      if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        SOUL_LOG_INFO(
          "vulkanPickSwapExtent | Extent = {} {}",
          capabilities.currentExtent.width,
          capabilities.currentExtent.height);
        return capabilities.currentExtent;
      }
      return {
        .width = std::max(
          capabilities.minImageExtent.width,
          std::min(capabilities.maxImageExtent.width, dimension.x)),
        .height = std::max(
          capabilities.minImageExtent.height,
          std::min(capabilities.maxImageExtent.height, dimension.y))};
    };

    auto pick_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
      -> VkSurfaceFormatKHR
    {
      runtime::ScopeAllocator<> scope_allocator("GPU::System::init::pickSurfaceFormat");
      SOUL_LOG_INFO("Picking surface format.");
      Vector<VkSurfaceFormatKHR> formats(&scope_allocator);
      u32 format_count;
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
      SOUL_ASSERT(0, format_count != 0, "Surface format count is zero!");
      formats.resize(format_count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
      for (const VkSurfaceFormatKHR surface_format : formats) {
        if (
          surface_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
          surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          return surface_format;
        }
      }
      return formats[0];
    };

    auto vma_alloc_callback(
      void* user_data,
      const size_t size,
      const size_t alignment,
      VkSystemAllocationScope /* allocation_scope */) -> void*
    {
      auto* const allocator = static_cast<memory::Allocator*>(user_data);
      return allocator->allocate(size, alignment);
    }

    auto vma_reallocation_callback(
      void* user_data,
      void* addr,
      const size_t size,
      const size_t alignment,
      VkSystemAllocationScope /* scope */) -> void*
    {
      auto* const allocator = static_cast<memory::Allocator*>(user_data);

      if (addr != nullptr) {
        allocator->deallocate(addr);
      }
      return allocator->allocate(size, alignment);
    }

    auto vma_free_callback(void* user_data, void* addr) -> void
    {
      if (addr == nullptr) {
        return;
      }
      auto* const allocator = static_cast<memory::Allocator*>(user_data);
      allocator->deallocate(addr);
    }

    auto VKAPI_CALL debug_callback(
      const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
      [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT message_type,
      const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
      void* /* user_data */) -> VkBool32
    {
      switch (message_severity) {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
        break;
      }
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
        SOUL_LOG_INFO("VkDebugUtils: {}", callback_data->pMessage);
        break;
      }
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        SOUL_LOG_WARN("VkDebugUtils: {}", callback_data->pMessage);
        break;
      }
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        SOUL_LOG_ERROR("VkDebugUtils: {}", callback_data->pMessage);
        SOUL_PANIC("Vulkan Error!, VkDebugUtils: {}", callback_data->pMessage);
        break;
      }
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: {
        SOUL_NOT_IMPLEMENTED();
        break;
      }
      }
      return VK_FALSE;
    }

    // NOLINTBEGIN(cert-err33-c)
    inline auto load_file(const char* filepath, memory::Allocator& allocator) -> CString
    {
      FILE* file = nullptr;
      const auto err = fopen_s(&file, filepath, "rb");
      if (err != 0) {
        SOUL_PANIC("Fail to open file %s", filepath);
      }
      SCOPE_EXIT(fclose(file));

      fseek(file, 0, SEEK_END);
      const auto fsize = ftell(file);
      fseek(file, 0, SEEK_SET); /* same as rewind(f); */

      auto string = CString::WithSize(fsize, &allocator);
      fread(string.data(), 1, fsize, file);

      return string;
    }
    // NOLINTEND(cert-err33-c)
  } // namespace

  auto create_dxc_session() -> DxcSession
  {
    DxcSession dxc_session;
    if (const auto result =
          DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(dxc_session.library.GetAddressOf()));
        FAILED(result)) {
      SOUL_PANIC("Fail to create dxc library instance");
    }

    if (const auto result =
          DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxc_session.compiler.GetAddressOf()));
        FAILED(result)) {
      SOUL_PANIC("Fail to create dxc compiler instance");
    }

    if (const auto result =
          DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxc_session.utils.GetAddressOf()));
        FAILED(result)) {
      SOUL_PANIC("Fail to create dxc utils instance");
    }

    if (const auto result = dxc_session.utils->CreateDefaultIncludeHandler(
          dxc_session.default_include_handler.GetAddressOf());
        FAILED(result)) {
      SOUL_PANIC("Fail to create dxc inlcude handler");
    }

    return dxc_session;
  }

  auto get_dxc_session() -> DxcSession*
  {
    static DxcSession dxc_global_session = create_dxc_session();
    return &dxc_global_session;
  }

  auto System::init(const Config& config) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    config_ = config;
    runtime::ScopeAllocator<> scope_allocator("GPU::System::Init");

    _db.current_frame = 0;
    _db.frame_counter = 0;

    _db.sampler_map.reserve(128);

    SOUL_ASSERT(
      0,
      config.wsi != nullptr,
      "Invalid configuration value | wsi = nullptr, headless rendering is not supported yet");
    SOUL_ASSERT(
      0,
      config.thread_count > 0,
      "Invalid configuration value | threadCount = {}",
      config.thread_count);
    SOUL_ASSERT(
      0,
      config.max_frame_in_flight > 0,
      "Invalid configuration value | maxFrameInFlight = {}",
      config.max_frame_in_flight);

    SOUL_VK_CHECK(volkInitialize(), "Volk initialization fail!");
    SOUL_LOG_INFO("Volk initialization sucessful");

    auto create_instance = []() -> VkInstance {
      const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Soul",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "Soul",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3,
      };

      static constexpr const char* INSTANCE_REQUIRED_EXTENSIONS[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(SOUL_OS_WINDOWS)
        "VK_KHR_win32_surface",
#endif // SOUL_PLATFORM_OS_WIN32
#ifdef SOUL_OS_APPLE
        "VK_MVK_macos_surface",
#endif // SOUL_
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
      };

#ifdef SOUL_VULKAN_VALIDATION_ENABLE
      static constexpr const char* REQUIRED_LAYERS[] = {
        "VK_LAYER_KHRONOS_validation",
      };
      auto is_required_layers_supported = []() -> b8 {
        SOUL_LOG_INFO("Check vulkan layer support.");
        u32 layer_count;

        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        Vector<VkLayerProperties> available_layers;
        available_layers.resize(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
        for (const char* required_layer : REQUIRED_LAYERS) {
          auto layer_found = false;
          for (const auto& layer : available_layers) {
            if (strcmp(required_layer, layer.layerName) == 0) {
              layer_found = true;
              break;
            }
          }
          if (!layer_found) {
            SOUL_LOG_INFO("Validation layer {} not found!", required_layer);
            return false;
          }
        }
        available_layers.cleanup();

        return true;
      };

      SOUL_ASSERT(0, is_required_layers_supported(), "");

      constexpr u32 required_layers_count = std::size(REQUIRED_LAYERS);
#else
      constexpr u32 required_layers_count = 0;
      static constexpr const char* const* REQUIRED_LAYERS = nullptr;
#endif

      const VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = required_layers_count,
        .ppEnabledLayerNames = REQUIRED_LAYERS,
        .enabledExtensionCount = std::size(INSTANCE_REQUIRED_EXTENSIONS),
        .ppEnabledExtensionNames = INSTANCE_REQUIRED_EXTENSIONS,
      };

      VkInstance instance;
      SOUL_VK_CHECK(
        vkCreateInstance(&instance_create_info, nullptr, &instance),
        "Vulkan instance creation fail!");
      SOUL_LOG_INFO("Vulkan instance creation succesful");
      volkLoadInstanceOnly(instance);
      SOUL_LOG_INFO(
        "Instance version = {}, {}",
        VK_VERSION_MAJOR(volkGetInstanceVersion()),
        VK_VERSION_MINOR(volkGetInstanceVersion()));
      return instance;
    };
    _db.instance = create_instance();

    auto create_debug_utils_messenger = [](VkInstance instance) -> VkDebugUtilsMessengerEXT {
      VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
      SOUL_LOG_INFO("Creating vulkan debug utils messenger");
      const VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = nullptr};
      SOUL_VK_CHECK(
        vkCreateDebugUtilsMessengerEXT(
          instance, &debug_messenger_create_info, nullptr, &debug_messenger),
        "Vulkan debug messenger creation fail!");
      SOUL_LOG_INFO("Vulkan debug messenger creation sucesfull");
      return debug_messenger;
    };
    _db.debug_messenger = create_debug_utils_messenger(_db.instance);

    _db.surface = config.wsi->create_vulkan_surface(_db.instance);

    static constexpr const char* DEVICE_REQUIRED_EXTENSIONS[] = {
      VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME,
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
      VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
      VK_KHR_RAY_QUERY_EXTENSION_NAME,
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
      VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
      VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };
    static constexpr uint32_t DEVICE_REQUIRED_EXTENSION_COUNT =
      std::size(DEVICE_REQUIRED_EXTENSIONS);
    auto pick_physical_device = [](Database* db) -> FlagMap<QueueType, u32> {
      SOUL_LOG_INFO("Picking vulkan physical device.");
      db->physical_device = VK_NULL_HANDLE;
      u32 device_count = 0;
      vkEnumeratePhysicalDevices(db->instance, &device_count, nullptr);
      SOUL_ASSERT(0, device_count > 0, "There is no device with vulkan support!");

      Vector<VkPhysicalDevice> devices;
      devices.resize(device_count);
      vkEnumeratePhysicalDevices(db->instance, &device_count, devices.data());

      Vector<VkSurfaceFormatKHR> formats;
      Vector<VkPresentModeKHR> present_modes;

      db->physical_device = VK_NULL_HANDLE;
      auto best_score = -1;

      FlagMap<QueueType, u32> queue_family_indices{};

      for (VkPhysicalDevice device : devices) {

        VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_features = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
          .pNext = nullptr};
        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
          .pNext = &accel_features};
        VkPhysicalDeviceFeatures2 device_features{
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &indexing_features};
        vkGetPhysicalDeviceFeatures2(device, &device_features);
        const auto bindless_supported = indexing_features.descriptorBindingPartiallyBound &&
                                        indexing_features.runtimeDescriptorArray;
        if (!bindless_supported) {
          continue;
        }

        VkPhysicalDeviceProperties physical_device_properties;
        VkPhysicalDeviceFeatures physical_device_features;
        vkGetPhysicalDeviceProperties(device, &physical_device_properties);
        vkGetPhysicalDeviceFeatures(device, &physical_device_features);

        const auto api_version = physical_device_properties.apiVersion;
        const auto driver_version = physical_device_properties.driverVersion;
        const auto vendor_id = physical_device_properties.vendorID;
        const auto device_id = physical_device_properties.deviceID;

        SOUL_LOG_INFO(
          "Devices\n"
          " -- Name = {}\n"
          " -- Vendor = {:#x}\n"
          " -- Device ID = {:#x}\n"
          " -- Api Version = {}.{}.{}\n"
          " -- Driver Version = {}.{}.{}\n",
          physical_device_properties.deviceName,
          vendor_id,
          device_id,
          VK_VERSION_MAJOR(api_version),
          VK_VERSION_MINOR(api_version),
          VK_VERSION_PATCH(api_version),
          VK_VERSION_MAJOR(driver_version),
          VK_VERSION_MINOR(driver_version),
          VK_VERSION_PATCH(driver_version));

        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        Vector<VkExtensionProperties> available_extensions;
        available_extensions.resize(extension_count);
        vkEnumerateDeviceExtensionProperties(
          device, nullptr, &extension_count, available_extensions.data());

        auto is_extension_available =
          [&available_extensions](const char* required_extension) -> b8 {
          return std::ranges::any_of(
            available_extensions, [required_extension](const auto& properties) {
              return strcmp(properties.extensionName, required_extension);
            });
        };
        const auto all_extension_found = std::any_of(
          DEVICE_REQUIRED_EXTENSIONS,
          DEVICE_REQUIRED_EXTENSIONS + DEVICE_REQUIRED_EXTENSION_COUNT,
          is_extension_available);
        if (!all_extension_found) {
          continue;
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, db->surface, &db->surface_caps);

        u32 format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, db->surface, &format_count, nullptr);
        SOUL_LOG_INFO(" -- Format count = {}", format_count);
        if (format_count == 0) {
          continue;
        }
        formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, db->surface, &format_count, formats.data());

        u32 present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, db->surface, &present_mode_count, nullptr);
        SOUL_LOG_INFO(" -- Present mode count = {}", present_mode_count);
        if (present_mode_count == 0) {
          continue;
        }
        present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, db->surface, &present_mode_count, present_modes.data());

        u32 queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        Vector<VkQueueFamilyProperties> queue_families;
        queue_families.resize(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(
          device, &queue_family_count, queue_families.data());

        const auto not_found_family_index = soul::cast<u32>(queue_families.size());

        auto graphics_queue_family_index = not_found_family_index;
        // Try to find queue that support present, graphics and compute
        for (u32 j = 0; j < queue_family_count; j++) {
          VkBool32 present_support = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(device, j, db->surface, &present_support);

          const VkQueueFamilyProperties& queue_props = queue_families[j];
          if (VkQueueFlags required_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
              present_support && queue_props.queueCount > 0 &&
              (queue_props.queueFlags & required_flags) == required_flags) {
            graphics_queue_family_index = j;
          }
        }

        auto compute_queue_family_index = not_found_family_index;
        for (u32 j = 0; j < queue_family_count; j++) {
          const VkQueueFamilyProperties& queue_props = queue_families[j];
          VkQueueFlags required_flags = VK_QUEUE_COMPUTE_BIT;
          if (
            j != graphics_queue_family_index && queue_props.queueCount > 0 &&
            (queue_props.queueFlags & required_flags) == required_flags) {
            compute_queue_family_index = j;
            break;
          }
        }

        auto transfer_queue_family_index = not_found_family_index;
        for (u32 j = 0; j < queue_family_count; j++) {
          const VkQueueFamilyProperties& queue_props = queue_families[j];
          VkQueueFlags required_flags = VK_QUEUE_TRANSFER_BIT;
          if (
            j != graphics_queue_family_index && j != compute_queue_family_index &&
            (queue_props.queueFlags & required_flags) == required_flags) {
            transfer_queue_family_index = j;
            break;
          }
        }

        queue_families.cleanup();

        if (graphics_queue_family_index == not_found_family_index) {
          SOUL_LOG_INFO(" -- Graphics queue family not found");
          continue;
        }
        if (compute_queue_family_index == not_found_family_index) {
          SOUL_LOG_INFO(" -- Compute queue family not found");
          continue;
        }
        if (transfer_queue_family_index == not_found_family_index) {
          SOUL_LOG_INFO("-- Transfer queue family not found");
          continue;
        }

        auto score = 0;
        if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
          score += 100;
        }

        SOUL_LOG_INFO("-- Score = {}", score);
        if (score > best_score) {
          db->physical_device = device;
          queue_family_indices[QueueType::GRAPHIC] = graphics_queue_family_index;
          queue_family_indices[QueueType::TRANSFER] = transfer_queue_family_index;
          queue_family_indices[QueueType::COMPUTE] = compute_queue_family_index;
          best_score = score;
        }
      }

      SOUL_ASSERT(
        0,
        db->physical_device != VK_NULL_HANDLE,
        "Cannot find physical device that satisfy the requirements.");

      vkGetPhysicalDeviceProperties(db->physical_device, &db->physical_device_properties);
      VkPhysicalDeviceProperties2 physical_device_properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &db->as_properties};
      vkGetPhysicalDeviceProperties2(db->physical_device, &physical_device_properties2);
      vkGetPhysicalDeviceMemoryProperties(
        db->physical_device, &db->physical_device_memory_properties);
      vkGetPhysicalDeviceFeatures(db->physical_device, &db->physical_device_features);

      const auto api_version = db->physical_device_properties.apiVersion;
      const auto driver_version = db->physical_device_properties.driverVersion;
      const auto vendor_id = db->physical_device_properties.vendorID;
      const auto device_id = db->physical_device_properties.deviceID;

      SOUL_LOG_INFO(
        "Selected device\n"
        " -- Name = {}\n"
        " -- Vendor = {:#x}\n"
        " -- Device ID = {:#x}\n"
        " -- Api Version = {:#x}\n"
        " -- Driver Version = {:#x}\n"
        " -- Graphics queue family index = {}\n"
        " -- Transfer queue family index = {}\n"
        " -- Compute queue family index = {}\n",
        db->physical_device_properties.deviceName,
        vendor_id,
        device_id,
        api_version,
        driver_version,
        queue_family_indices[QueueType::GRAPHIC],
        queue_family_indices[QueueType::TRANSFER],
        queue_family_indices[QueueType::COMPUTE]);

      SOUL_ASSERT(0, db->physical_device != VK_NULL_HANDLE, "Fail to find a suitable GPU!");
      return queue_family_indices;
    };

    _db.queue_family_indices = pick_physical_device(&_db);

    auto create_device_and_queue =
      [](VkPhysicalDevice physicalDevice, FlagMap<QueueType, u32>& queue_family_indices)
      -> std::tuple<VkDevice, FlagMap<QueueType, u32>> {
      SOUL_LOG_INFO("Creating vulkan logical device");

      auto graphicsQueueCount = 1;
      auto queueIndex = FlagMap<QueueType, u32>::init_fill(0);

      if (queue_family_indices[QueueType::COMPUTE] == queue_family_indices[QueueType::GRAPHIC]) {
        graphicsQueueCount++;
        queueIndex[QueueType::COMPUTE] = queueIndex[QueueType::GRAPHIC] + 1;
      }
      if (queue_family_indices[QueueType::TRANSFER] == queue_family_indices[QueueType::GRAPHIC]) {
        graphicsQueueCount++;
        queueIndex[QueueType::TRANSFER] = queueIndex[QueueType::COMPUTE] + 1;
      }

      VkDeviceQueueCreateInfo queue_create_info[4] = {};
      f32 priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};
      u32 queue_create_info_count = 1;

      queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info[0].queueFamilyIndex = queue_family_indices[QueueType::GRAPHIC];
      queue_create_info[0].queueCount = graphicsQueueCount;
      queue_create_info[0].pQueuePriorities = priorities;

      if (queue_family_indices[QueueType::GRAPHIC] != queue_family_indices[QueueType::COMPUTE]) {
        queue_create_info[queue_create_info_count].sType =
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[queue_create_info_count].queueFamilyIndex =
          queue_family_indices[QueueType::COMPUTE];
        queue_create_info[queue_create_info_count].queueCount = 1;
        queue_create_info[queue_create_info_count].pQueuePriorities =
          priorities + queue_create_info_count;
        queue_create_info_count++;
      }

      if (queue_family_indices[QueueType::TRANSFER] != queue_family_indices[QueueType::GRAPHIC]) {
        queue_create_info[queue_create_info_count].sType =
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[queue_create_info_count].queueFamilyIndex =
          queue_family_indices[QueueType::TRANSFER];
        queue_create_info[queue_create_info_count].queueCount = 1;
        queue_create_info[queue_create_info_count].pQueuePriorities =
          priorities + queue_create_info_count;
        queue_create_info_count++;
      }

      // TODO(kevinyu): This is temporary hack to avoid molten vk bug when queue family is not
      // sorted in queueCreateInfo
      queue_create_info[3] = queue_create_info[0];
      queue_create_info[0] = queue_create_info[1];
      queue_create_info[1] = queue_create_info[2];
      queue_create_info[2] = queue_create_info[3];

      VkPhysicalDeviceFeatures device_features = {
        .geometryShader = VK_TRUE,
        .multiDrawIndirect = VK_TRUE,
        .fillModeNonSolid = VK_TRUE,
        .fragmentStoresAndAtomics = VK_TRUE,
        .shaderInt64 = VK_TRUE,
      };

      VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .rayQuery = VK_TRUE,
      };

      VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = &ray_query_features,
        .rayTracingPipeline = VK_TRUE,
      };

      VkPhysicalDeviceAccelerationStructureFeaturesKHR as_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = &rt_pipeline_features,
        .accelerationStructure = VK_TRUE,
        .descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE,
      };

      VkPhysicalDeviceVulkan13Features device_1_3_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &as_features,
        .synchronization2 = VK_TRUE,
      };

      VkPhysicalDeviceVulkan12Features device_1_2_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &device_1_3_features,
        .descriptorIndexing = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
        .vulkanMemoryModel = VK_TRUE,
      };

      const VkPhysicalDeviceFeatures2 device_features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &device_1_2_features,
        .features = device_features,
      };

      const VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_features2,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_info,
        .enabledExtensionCount = DEVICE_REQUIRED_EXTENSION_COUNT,
        .ppEnabledExtensionNames = DEVICE_REQUIRED_EXTENSIONS,
        .pEnabledFeatures = nullptr,
      };

      VkDevice device;
      SOUL_VK_CHECK(
        vkCreateDevice(physicalDevice, &device_create_info, nullptr, &device),
        "Vulkan logical device creation fail!");
      SOUL_LOG_INFO("Vulkan logical device creation sucessful");
      return std::make_tuple(device, queueIndex);
    };

    auto [device, queueIndex] =
      create_device_and_queue(_db.physical_device, _db.queue_family_indices);
    _db.device = device;
    volkLoadDevice(device);
    for (const auto queue_type : FlagIter<QueueType>()) {
      _db.queues[queue_type].init(
        device, _db.queue_family_indices[queue_type], queueIndex[queue_type]);
    }

    auto create_swapchain = [this](Database* db, WSI* wsi) {
      _db.swapchain.wsi = wsi;
      const auto framebuffer_size = wsi->get_framebuffer_size();
      db->swapchain.format = pick_surface_format(db->physical_device, db->surface);
      db->swapchain.extent = pick_surface_extent(db->surface_caps, framebuffer_size);
      db->swapchain.image_count = [db]() {
        auto image_count = db->surface_caps.minImageCount + 1;
        if (db->surface_caps.maxImageCount > 0 && image_count > db->surface_caps.maxImageCount) {
          image_count = db->surface_caps.maxImageCount;
        }
        return image_count;
      }();

      SOUL_LOG_INFO("Swapchain image count = {}", db->swapchain.image_count);

      const VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = db->surface,
        .minImageCount = db->swapchain.image_count,
        .imageFormat = db->swapchain.format.format,
        .imageColorSpace = db->swapchain.format.colorSpace,
        .imageExtent = db->swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &_db.queue_family_indices[QueueType::GRAPHIC],
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
        .clipped = VK_TRUE,
      };
      SOUL_VK_CHECK(
        vkCreateSwapchainKHR(db->device, &swapchain_info, nullptr, &db->swapchain.vk_handle),
        "Fail to create vulkan swapchain!");
      vkGetSwapchainImagesKHR(
        db->device, db->swapchain.vk_handle, &db->swapchain.image_count, nullptr);
      db->swapchain.images.resize(db->swapchain.image_count);
      vkGetSwapchainImagesKHR(
        db->device,
        db->swapchain.vk_handle,
        &db->swapchain.image_count,
        db->swapchain.images.data());

      db->swapchain.image_views.reserve(db->swapchain.image_count);
      std::ranges::transform(
        db->swapchain.images,
        std::back_inserter(db->swapchain.image_views),
        [format = db->swapchain.format.format, device = db->device](VkImage image) {
          VkImageView image_view;
          const VkImageViewCreateInfo image_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components =
              {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
              },
            .subresourceRange =
              {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
              },
          };
          SOUL_VK_CHECK(
            vkCreateImageView(device, &image_view_info, nullptr, &image_view),
            "Fail to create swapchain imageview");
          return image_view;
        });

      db->swapchain.textures.reserve(db->swapchain.image_count);
      std::ranges::transform(
        db->swapchain.images,
        db->swapchain.image_views,
        std::back_inserter(db->swapchain.textures),
        [db, this, image_sharing_mode = swapchain_info.imageSharingMode](
          VkImage image, VkImageView image_view) {
          const auto texture_id = TextureID(db->texture_pool.create(Texture{
            .desc = TextureDesc::d2(
              "Swapchain Texture",
              TextureFormat::BGRA8,
              1,
              {},
              {},
              vec2ui32(db->swapchain.extent.width, db->swapchain.extent.height)),
            .vk_handle = image,
            .view = {.vk_handle = image_view},
            .sharing_mode = image_sharing_mode}));
          return texture_id;
        });

      SOUL_LOG_INFO("Vulkan swapchain creation sucessful");
    };
    create_swapchain(&_db, config.wsi);

    auto init_allocator = [](Database* db, const Config& config) {
      const VmaVulkanFunctions vulkan_functions = {
        .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = vkAllocateMemory,
        .vkFreeMemory = vkFreeMemory,
        .vkMapMemory = vkMapMemory,
        .vkUnmapMemory = vkUnmapMemory,
        .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = vkBindBufferMemory,
        .vkBindImageMemory = vkBindImageMemory,
        .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
        .vkCreateBuffer = vkCreateBuffer,
        .vkDestroyBuffer = vkDestroyBuffer,
        .vkCreateImage = vkCreateImage,
        .vkDestroyImage = vkDestroyImage,
        .vkCmdCopyBuffer = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR = vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
      };

      const VkAllocationCallbacks allocation_callbacks = {
        &(db->vulkan_cpu_allocator),
        vma_alloc_callback,
        vma_reallocation_callback,
        vma_free_callback,
        nullptr,
        nullptr};

      const VmaAllocatorCreateInfo allocator_info = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = db->physical_device,
        .device = db->device,
        .preferredLargeHeapBlockSize = 0,
        .pAllocationCallbacks = &allocation_callbacks,
        .pDeviceMemoryCallbacks = nullptr,
        .pVulkanFunctions = &vulkan_functions,
        .instance = db->instance,
      };

      vmaCreateAllocator(&allocator_info, &db->gpu_allocator);

      db->linear_pools.resize(db->physical_device_memory_properties.memoryTypeCount);
      for (u32 memory_index = 0; memory_index < db->linear_pools.size(); memory_index++) {
        const VmaPoolCreateInfo pool_create_info = {
          .memoryTypeIndex = memory_index,
          .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
          .blockSize = config.transient_pool_size,
          .maxBlockCount = 1,
        };
        vmaCreatePool(db->gpu_allocator, &pool_create_info, &db->linear_pools[memory_index]);
      }

      SOUL_LOG_INFO("Vulkan init allocator sucessful");
    };
    init_allocator(&_db, config);
    _db.descriptor_allocator.init(_db.device);
    init_frame_context(config);
    calculate_gpu_properties();
    begin_frame();
    SOUL_FLUSH_LOGS();
  }

  auto System::init_frame_context(const Config& config) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_LOG_INFO("Frame Context Init");
    _db.frame_contexts.reserve(config.max_frame_in_flight);
    for (auto i = 0; i < _db.frame_contexts.capacity(); i++) {
      _db.frame_contexts.emplace_back(&_db.cpu_allocator);
    }
    std::ranges::for_each(_db.frame_contexts, [this, config](FrameContext& frame_context) {
      frame_context.command_pools.init(_db.device, _db.queues, config.thread_count);
      frame_context.gpu_resource_initializer.init(_db.gpu_allocator, &frame_context.command_pools);
      frame_context.gpu_resource_finalizer.init();
      frame_context.frame_end_semaphore = TimelineSemaphore::null();
      frame_context.image_available_semaphore = create_binary_semaphore();
      frame_context.render_finished_semaphore = create_binary_semaphore();
    });
  }

  auto System::get_gpu_properties() const -> const GPUProperties& { return _db.gpu_properties; }

  auto System::get_queue_data_from_queue_flags(QueueFlags flags) const -> QueueData
  {
    QueueData queue_data;
    const auto& queues = _db.queues;
    flags.for_each([&](QueueType type) {
      queue_data.indices[queue_data.count++] = queues[type].get_family_index();
    });
    return queue_data;
  }

  auto System::create_texture(const TextureDesc& desc) -> TextureID
  {
    SOUL_PROFILE_ZONE();

    SOUL_ASSERT(0, desc.layer_count >= 1, "");

    const VkFormat format = vk_cast(desc.format);
    const QueueData queue_data = get_queue_data_from_queue_flags(desc.queue_flags);
    const VkImageCreateFlags image_create_flags =
      desc.type == TextureType::CUBE
        ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
        : VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    const VkImageCreateInfo image_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .flags = image_create_flags,
      .imageType = vk_cast(desc.type),
      .format = format,
      .extent = get_vk_extent_3d(desc.extent),
      .mipLevels = desc.mip_levels,
      .arrayLayers = desc.layer_count,
      .samples = vk_cast(desc.sample_count),
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = vk_cast(desc.usage_flags),
      .sharingMode = queue_data.count == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
      .queueFamilyIndexCount = queue_data.count,
      .pQueueFamilyIndices = queue_data.indices,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    const VmaAllocationCreateInfo alloc_info = {.usage = VMA_MEMORY_USAGE_GPU_ONLY};

    VmaAllocation allocation;
    VkImage vk_handle;
    SOUL_VK_CHECK(
      vmaCreateImage(_db.gpu_allocator, &image_info, &alloc_info, &vk_handle, &allocation, nullptr),
      "Fail to create image");

    auto image_aspect = vk_cast_format_to_aspect_flags(desc.format);
    if (image_aspect & VK_IMAGE_ASPECT_STENCIL_BIT) {
      SOUL_LOG_WARN(
        "Texture creation with stencil format detected. Current version will remove the aspect "
        "stencil bit so the texture cannot be used for depth stencil. The reason is because Vulkan "
        "spec stated that descriptor cannot have more than one aspect.");
      image_aspect &= ~(VK_IMAGE_ASPECT_STENCIL_BIT);
    }

    const VkImageViewCreateInfo image_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = vk_handle,
      .viewType = vk_cast_to_image_view_type(desc.type),
      .format = format,
      .components = {},
      .subresourceRange = {image_aspect, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS},
    };

    VkImageView view_vk_handle;
    SOUL_VK_CHECK(
      vkCreateImageView(_db.device, &image_view_info, nullptr, &view_vk_handle),
      "Fail to create image view");

    const auto texture_id = TextureID(_db.texture_pool.create(Texture{
      .desc = desc,
      .vk_handle = vk_handle,
      .allocation = allocation,
      .view = {.vk_handle = view_vk_handle},
      .sharing_mode = image_info.sharingMode,
    }));
    Texture& texture = *_db.texture_pool.get(texture_id.id);

    if (desc.usage_flags.test(TextureUsage::SAMPLED)) {
      texture.view.sampled_image_gpu_handle =
        _db.descriptor_allocator.create_sampled_image_descriptor(texture.view.vk_handle);
    }
    if (desc.usage_flags.test(TextureUsage::STORAGE)) {
      texture.view.storage_image_gpu_handle =
        _db.descriptor_allocator.create_storage_image_descriptor(texture.view.vk_handle);
    }

    if (desc.name != nullptr) {
      const auto tex_name = CString::Format("{}({})", desc.name, _db.frame_counter);
      const VkDebugUtilsObjectNameInfoEXT image_name_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_IMAGE,
        reinterpret_cast<u64>(texture.vk_handle),
        tex_name.data(),
      };

      vkSetDebugUtilsObjectNameEXT(_db.device, &image_name_info);
    }

    return texture_id;
  }

  auto System::create_texture(const TextureDesc& desc, const TextureLoadDesc& load_desc)
    -> TextureID
  {
    SOUL_ASSERT(0, load_desc.data != nullptr, "");
    SOUL_ASSERT(0, load_desc.data_size != 0, "");
    SOUL_ASSERT(0, load_desc.regions.size() != 0, "");

    TextureDesc new_desc = desc;
    new_desc.usage_flags |= {TextureUsage::TRANSFER_DST};
    new_desc.queue_flags |= {QueueType::TRANSFER};
    if (load_desc.generate_mipmap) {
      new_desc.usage_flags |= {TextureUsage::TRANSFER_SRC};
    }

    const TextureID texture_id = create_texture(new_desc);
    auto& texture = get_texture(texture_id);

    get_frame_context().gpu_resource_initializer.load(texture, load_desc);

    if (load_desc.generate_mipmap && desc.mip_levels > 1) {
      get_frame_context().gpu_resource_initializer.generate_mipmap(texture);
    }

    return texture_id;
  }

  auto System::create_texture(const TextureDesc& desc, const ClearValue clear_value) -> TextureID
  {
    TextureDesc new_desc = desc;
    new_desc.usage_flags |= {TextureUsage::TRANSFER_DST};
    new_desc.queue_flags |= {QueueType::GRAPHIC};

    TextureID texture_id = create_texture(new_desc);
    auto& texture = get_texture(texture_id);

    get_frame_context().gpu_resource_initializer.clear(texture, clear_value);

    return texture_id;
  }

  void System::flush_texture(const TextureID texture_id, const TextureUsageFlags usage_flags)
  {
    get_frame_context().gpu_resource_finalizer.finalize(get_texture(texture_id), usage_flags);
  }

  auto System::get_texture_mip_levels(const TextureID texture_id) const -> u32
  {
    return get_texture(texture_id).desc.mip_levels;
  }

  auto System::get_texture_desc(const TextureID texture_id) const -> const TextureDesc&
  {
    return get_texture(texture_id).desc;
  }

  auto System::destroy_texture_descriptor(TextureID texture_id) -> void
  {
    auto destroy_texture_view_descriptor = [this](const TextureView& texture_view) {
      _db.descriptor_allocator.destroy_sampled_image_descriptor(
        texture_view.sampled_image_gpu_handle);
      _db.descriptor_allocator.destroy_storage_image_descriptor(
        texture_view.storage_image_gpu_handle);
    };
    const auto& texture = get_texture(texture_id);
    destroy_texture_view_descriptor(texture.view);
    const auto view_count = texture.desc.mip_levels * texture.desc.layer_count;
    if (texture.views != nullptr) {
      std::for_each_n(texture.views, view_count, destroy_texture_view_descriptor);
    }
  }

  auto System::destroy_texture(TextureID id) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    get_frame_context().garbages.textures.push_back(id);
  }

  auto System::get_texture(const TextureID texture_id) -> Texture& // NOLINT
  {
    return *_db.texture_pool.get(texture_id.id);
  }

  auto System::get_texture(const TextureID texture_id) const -> const Texture&
  {
    const Texture* texture = _db.texture_pool.get(texture_id.id);
    return *texture;
  }

  auto System::get_texture_view(const TextureID texture_id, const u32 level, const u32 layer)
    -> TextureView
  {
    auto& texture = get_texture(texture_id);
    SOUL_ASSERT(0, level < texture.desc.mip_levels, "");

    const auto layer_count = texture.desc.type == TextureType::D2_ARRAY ? texture.desc.extent.z : 1;

    if (texture.desc.mip_levels == 1) {
      return texture.view;
    }
    if (texture.views == nullptr) {
      texture.views = _db.cpu_allocator.allocate_array<TextureView>(
        soul::cast<u64>(layer_count) * texture.desc.mip_levels);
      std::fill_n(texture.views, texture.desc.mip_levels, TextureView());
    }
    const usize view_idx = layer * texture.desc.mip_levels + level;
    if (texture.views[view_idx].vk_handle == VK_NULL_HANDLE) {
      TextureView& texture_view = texture.views[level];
      const VkImageViewCreateInfo image_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = texture.vk_handle,
        .viewType = vk_cast_to_image_view_type(texture.desc.type),
        .format = vk_cast(texture.desc.format),
        .components = {},
        .subresourceRange =
          {vk_cast_format_to_aspect_flags(texture.desc.format), level, 1, layer, 1},
      };
      SOUL_VK_CHECK(
        vkCreateImageView(_db.device, &image_view_info, nullptr, &texture_view.vk_handle),
        "Fail to create image view");
      if (texture.desc.usage_flags.test(TextureUsage::SAMPLED)) {
        texture_view.sampled_image_gpu_handle =
          _db.descriptor_allocator.create_sampled_image_descriptor(texture_view.vk_handle);
      }
      if (texture.desc.usage_flags.test(TextureUsage::STORAGE)) {
        texture_view.storage_image_gpu_handle =
          _db.descriptor_allocator.create_storage_image_descriptor(texture_view.vk_handle);
      }
    }
    return texture.views[level];
  }

  auto System::get_texture_view(
    const TextureID texture_id, const SubresourceIndex subresource_index) -> TextureView
  {
    return get_texture_view(
      texture_id, subresource_index.get_level(), subresource_index.get_layer());
  }

  auto System::get_texture_view(
    const TextureID texture_id, const std::optional<SubresourceIndex> subresource_index)
    -> TextureView
  {
    if (subresource_index) {
      return get_texture_view(texture_id, *subresource_index);
    }
    return get_texture(texture_id).view;
  }

  auto System::get_blas_size_requirement(const BlasBuildDesc& build_desc) -> usize
  {
    const auto size_info = get_as_build_size_info(build_desc);
    return size_info.accelerationStructureSize;
  }

  auto System::create_blas(const BlasDesc& desc, const BlasGroupID blas_group_id) -> BlasID
  {
    runtime::ScopeAllocator<> scope_allocator("create_blas(const BlasDesc&, BlasGroupID)");
    CString as_storage_name(&scope_allocator);
    if (desc.name != nullptr) {
      as_storage_name.appendf("{}_storage_buffer", desc.name);
    }

    const BufferDesc as_buffer_desc = {
      .size = desc.size,
      .usage_flags = {BufferUsage::AS_STORAGE},
      .queue_flags = {QueueType::GRAPHIC, QueueType::COMPUTE},
      .name = as_storage_name.data(),
    };
    const auto storage_buffer_id = create_buffer(as_buffer_desc);
    const VkAccelerationStructureCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .buffer = get_buffer(storage_buffer_id).vk_handle,
      .size = desc.size,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
    };
    VkAccelerationStructureKHR vk_handle;
    vkCreateAccelerationStructureKHR(_db.device, &create_info, nullptr, &vk_handle);

    if (desc.name != nullptr) {
      CString as_name(&scope_allocator);
      as_name.appendf("{}({})", desc.name, _db.frame_counter);
      const VkDebugUtilsObjectNameInfoEXT as_name_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR,
        reinterpret_cast<u64>(vk_handle),
        as_name.data(),
      };
      vkSetDebugUtilsObjectNameEXT(_db.device, &as_name_info);
    }

    const auto blas_id = BlasID(_db.blas_pool.create(Blas{
      .desc = desc,
      .vk_handle = vk_handle,
      .buffer = storage_buffer_id,
      .cache_state = ResourceCacheState(),
    }));

    add_to_blas_group(blas_id, blas_group_id);
    return blas_id;
  }

  auto System::destroy_blas(BlasID blas_id) -> void
  {
    remove_from_blas_group(blas_id);
    const auto& blas = get_blas(blas_id);
    auto& frame_context = get_frame_context();
    frame_context.garbages.as_vk_handles.push_back(blas.vk_handle);
    destroy_buffer(blas.buffer);
    _db.blas_pool.destroy(blas_id.id);
  }

  auto System::get_blas(BlasID blas_id) const -> const Blas&
  {
    return *_db.blas_pool.get(blas_id.id);
  }

  auto System::get_blas(BlasID blas_id) -> Blas& { return *_db.blas_pool.get(blas_id.id); }

  auto System::get_gpu_address(BlasID blas_id) const -> GPUAddress
  {
    const VkAccelerationStructureDeviceAddressInfoKHR addr_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
      .accelerationStructure = get_blas(blas_id).vk_handle};

    return GPUAddress(vkGetAccelerationStructureDeviceAddressKHR(_db.device, &addr_info));
  }

  auto System::create_blas_group(const char* name) -> BlasGroupID
  {
    auto id = _db.blas_group_pool.create();
    _db.blas_group_pool.get(id)->name = name;
    return BlasGroupID(id);
  }

  auto System::destroy_blas_group(BlasGroupID blas_group_id) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    auto& blas_group = get_blas_group(blas_group_id);
    auto& frame_context = get_frame_context();
    for (const auto& blas_id : blas_group.blas_list) {
      auto& blas = get_blas(blas_id);
      frame_context.garbages.as_vk_handles.push_back(blas.vk_handle);
      destroy_buffer(blas.buffer);
    }
    _db.blas_group_pool.destroy(blas_group_id.id);
  }

  auto System::get_blas_group(BlasGroupID blas_group_id) const -> const BlasGroup&
  {
    return *_db.blas_group_pool.get(blas_group_id.id);
  }

  auto System::get_blas_group(BlasGroupID blas_group_id) -> BlasGroup&
  {
    return *_db.blas_group_pool.get(blas_group_id.id);
  }

  auto System::get_tlas_size_requirement(const TlasBuildDesc& build_desc) -> usize
  {
    const auto size_info = get_as_build_size_info(build_desc);
    return size_info.accelerationStructureSize;
  }

  auto System::create_tlas(const TlasDesc& desc) -> TlasID
  {
    runtime::ScopeAllocator<> scope_allocator("gpu::System::create_tlas(const TlasDesc& desc)");
    CString as_storage_name(&scope_allocator);
    if (desc.name != nullptr) {
      as_storage_name.appendf("{}_storage_buffer", desc.name);
    }
    const BufferDesc as_buffer_desc = {
      .size = desc.size,
      .usage_flags = {BufferUsage::AS_STORAGE},
      .queue_flags = {QueueType::GRAPHIC, QueueType::COMPUTE},
      .name = as_storage_name.data()};
    const auto as_storage_buffer_id = create_buffer(as_buffer_desc);

    const VkAccelerationStructureCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .buffer = get_buffer(as_storage_buffer_id).vk_handle,
      .size = desc.size,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };
    VkAccelerationStructureKHR vk_handle;
    vkCreateAccelerationStructureKHR(_db.device, &create_info, nullptr, &vk_handle);

    const auto descriptor_id = _db.descriptor_allocator.create_as_descriptor(vk_handle);

    const auto tlas_id = TlasID(_db.tlas_pool.create(Tlas{
      .desc = desc,
      .vk_handle = vk_handle,
      .buffer = as_storage_buffer_id,
      .descriptor_id = descriptor_id,
      .cache_state = ResourceCacheState(),
    }));
    return tlas_id;
  }

  auto System::destroy_tlas(TlasID tlas_id) -> void
  {
    auto& tlas = get_tlas(tlas_id);
    auto& frame_context = get_frame_context();
    frame_context.garbages.as_vk_handles.push_back(tlas.vk_handle);
    frame_context.garbages.as_descriptors.push_back(tlas.descriptor_id);
    destroy_buffer(tlas.buffer);
    _db.tlas_pool.destroy(tlas_id.id);
  }

  auto System::get_tlas(TlasID tlas_id) const -> const Tlas&
  {
    return *_db.tlas_pool.get(tlas_id.id);
  }

  auto System::get_tlas(TlasID tlas_id) -> Tlas& { return *_db.tlas_pool.get(tlas_id.id); }

  auto System::create_buffer(const BufferDesc& desc) -> BufferID
  {
    SOUL_ASSERT_MAIN_THREAD();
    return create_buffer(desc, false);
  }

  auto System::create_buffer(const BufferDesc& desc, const void* data) -> BufferID
  {
    SOUL_PROFILE_ZONE();
    BufferDesc new_desc = desc;
    new_desc.usage_flags |= {BufferUsage::TRANSFER_DST};
    new_desc.queue_flags |= {QueueType::TRANSFER};

    const BufferID buffer_id = create_buffer(new_desc);
    auto& buffer = get_buffer(buffer_id);

    get_frame_context().gpu_resource_initializer.load(buffer, data);

    return buffer_id;
  }

  auto System::create_transient_buffer(const BufferDesc& desc) -> BufferID
  {
    const auto buffer_id = create_buffer(desc, true);
    get_frame_context().garbages.buffers.push_back(buffer_id);

    return buffer_id;
  }

  auto System::is_owned_by_presentation_engine(TextureID texture_id) -> b8
  {
    if (get_swapchain_texture() != texture_id) {
      return false;
    }
    return get_frame_context().image_available_semaphore.state == BinarySemaphore::State::SIGNALLED;
  }

  auto System::create_buffer(const BufferDesc& desc, const b8 use_linear_pool) -> BufferID
  {
    SOUL_ASSERT(0, desc.size > 0, "");
    SOUL_ASSERT(0, desc.usage_flags.any(), "");

    auto queue_flags = desc.queue_flags;
    const auto usage_flags = desc.usage_flags;

    if (usage_flags.test(BufferUsage::AS_BUILD_INPUT)) {
      queue_flags |= {QueueType::COMPUTE};
    }

    const QueueData queue_data = get_queue_data_from_queue_flags(queue_flags);
    SOUL_ASSERT(0, queue_data.count > 0, "");
    const VkBufferCreateInfo buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = desc.size,
      .usage = vk_cast(desc.usage_flags),
      .sharingMode = queue_data.count > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = queue_data.count,
      .pQueueFamilyIndices = queue_data.indices,
    };

    auto alloc_create_info = [](const BufferDesc& buffer_desc) -> VmaAllocationCreateInfo {
      if (buffer_desc.memory_option) {
        return {
          .requiredFlags = vk_cast(buffer_desc.memory_option->required),
          .preferredFlags = vk_cast(buffer_desc.memory_option->preferred)};
      }
      return {.usage = VMA_MEMORY_USAGE_AUTO};
    }(desc);

    if (use_linear_pool) {
      u32 memory_index;
      vmaFindMemoryTypeIndexForBufferInfo(
        _db.gpu_allocator, &buffer_info, &alloc_create_info, &memory_index);
      alloc_create_info = {.pool = _db.linear_pools[memory_index]};
    }

    std::optional<VkDeviceSize> alignment = std::nullopt;
    if (desc.usage_flags.test(BufferUsage::AS_SCRATCH_BUFFER)) {
      alignment = _db.as_properties.minAccelerationStructureScratchOffsetAlignment;
    }

    VkBuffer vk_handle;
    VmaAllocation allocation;
    if (alignment) {
      SOUL_VK_CHECK(
        vmaCreateBufferWithAlignment(
          _db.gpu_allocator,
          &buffer_info,
          &alloc_create_info,
          alignment.value(),
          &vk_handle,
          &allocation,
          nullptr),
        "Fail to create buffer");
    } else {
      SOUL_VK_CHECK(
        vmaCreateBuffer(
          _db.gpu_allocator, &buffer_info, &alloc_create_info, &vk_handle, &allocation, nullptr),
        "Fail to create buffer");
    }
    const auto buffer_id = BufferID(_db.buffer_pool.create(Buffer{
      .desc = desc, .vk_handle = vk_handle, .allocation = allocation

    }));
    Buffer& buffer = get_buffer(buffer_id);

    if (buffer.desc.usage_flags.test(BufferUsage::STORAGE)) {
      buffer.storage_buffer_gpu_handle =
        _db.descriptor_allocator.create_storage_buffer_descriptor(buffer.vk_handle);
    }
    vmaGetAllocationMemoryProperties(
      _db.gpu_allocator, buffer.allocation, &buffer.memory_property_flags);

    if (desc.name != nullptr) {
      runtime::ScopeAllocator<> scope_allocator("Buffer name");
      CString buffer_name(&scope_allocator);
      buffer_name.appendf("{}(f{})", desc.name, _db.frame_counter);

      const VkDebugUtilsObjectNameInfoEXT image_name_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_BUFFER,
        reinterpret_cast<u64>(buffer.vk_handle),
        buffer_name.data(),
      };
      vkSetDebugUtilsObjectNameEXT(_db.device, &image_name_info);
    }

    return buffer_id;
  }

  auto System::create_staging_buffer(const usize size) -> BufferID
  {
    return create_transient_buffer({
      .size = size,
      .usage_flags = {BufferUsage::TRANSFER_SRC},
      .queue_flags = {QueueType::TRANSFER},
      .memory_option =
        MemoryOption{
          .required = {MemoryProperty::HOST_VISIBLE},
          .preferred = {MemoryProperty::DEVICE_LOCAL},
        },
    });
  }

  auto System::get_gpu_allocator() -> VmaAllocator { return _db.gpu_allocator; }

  auto System::acquire_swapchain() -> VkResult
  {
    auto& frame_context = get_frame_context();
    u32 swapchain_index;
    SOUL_PROFILE_ZONE_WITH_NAME("Acquire Next Image KHR");
    const auto result = vkAcquireNextImageKHR(
      _db.device,
      _db.swapchain.vk_handle,
      UINT64_MAX,
      frame_context.image_available_semaphore.vk_handle,
      VK_NULL_HANDLE,
      &swapchain_index);
    if (result < 0) {
      return result;
    }

    frame_context.swapchain_index = swapchain_index;
    const TextureID swapchain_texture_id = _db.swapchain.textures[swapchain_index];
    auto& swapchain_texture = get_texture(swapchain_texture_id);
    swapchain_texture.cache_state.commit_acquire_swapchain();
    swapchain_texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    frame_context.image_available_semaphore.state = BinarySemaphore::State::SIGNALLED;

    return result;
  }

  auto System::wait_sync_counter(TimelineSemaphore sync_counter) -> void
  {
    const VkSemaphoreWaitInfo wait_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
      .flags = 0,
      .semaphoreCount = 1,
      .pSemaphores = &sync_counter.vk_handle,
      .pValues = &sync_counter.counter,
    };
    vkWaitSemaphores(_db.device, &wait_info, UINT64_MAX);
  }

  auto System::flush_buffer(const BufferID buffer_id) -> void
  {
    get_frame_context().gpu_resource_finalizer.finalize(get_buffer(buffer_id));
  }

  auto System::destroy_buffer_descriptor(BufferID buffer_id) -> void
  {
    _db.descriptor_allocator.destroy_storage_buffer_descriptor(
      get_buffer(buffer_id).storage_buffer_gpu_handle);
  }

  auto System::destroy_buffer(BufferID id) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    get_frame_context().garbages.buffers.push_back(id);
  }

  auto System::get_buffer(const BufferID buffer_id) -> Buffer&
  {
    SOUL_ASSERT(0, !buffer_id.is_null(), "");
    return *_db.buffer_pool.get(buffer_id.id);
  }

  auto System::get_buffer(const BufferID buffer_id) const -> const Buffer&
  {
    SOUL_ASSERT(0, !buffer_id.is_null(), "");
    return *_db.buffer_pool.get(buffer_id.id);
  }

  auto System::get_gpu_address(BufferID buffer_id) const -> GPUAddress
  {
    const VkBufferDeviceAddressInfo addr_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = get_buffer(buffer_id).vk_handle,
    };

    return GPUAddress(vkGetBufferDeviceAddress(_db.device, &addr_info));
  }

  auto System::get_frame_context() -> FrameContext&
  {
    return _db.frame_contexts[_db.current_frame % _db.frame_contexts.size()];
  }

  auto System::request_pipeline_state(
    const GraphicPipelineStateDesc& desc,
    VkRenderPass render_pass,
    const TextureSampleCount sample_count) -> PipelineStateID
  {
    // TODO(kevinyu): Do we need to hash renderPass as well?
    const auto key = PipelineStateKey::From(GraphicPipelineStateKey{desc, sample_count});
    if (const auto id = _db.pipeline_state_cache.find(key); id != PipelineStateCache::NULLVAL) {
      return PipelineStateID(id);
    }

    auto create_graphic_pipeline_state = [this](
                                           const GraphicPipelineStateDesc& desc,
                                           VkRenderPass render_pass,
                                           const TextureSampleCount sample_count) -> PipelineState {
      runtime::ScopeAllocator<> scope_allocator("create_graphic_pipeline_state");

      const Program& program = get_program(desc.program_id);

      Vector<VkPipelineShaderStageCreateInfo> shader_stage_infos(&scope_allocator);

      for (const auto& shader : program.shaders) {
        if (shader.vk_handle == VK_NULL_HANDLE) {
          continue;
        }
        const VkPipelineShaderStageCreateInfo shader_stage_info = {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = vk_cast(shader.stage),
          .module = shader.vk_handle,
          .pName = shader.entry_point.data(),
        };
        shader_stage_infos.push_back(shader_stage_info);
      }

      static auto PRIMITIVE_TOPOLOGY_MAP = FlagMap<Topology, VkPrimitiveTopology>::from_val_list(
        {VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
         VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
         VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
         VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
         VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
         VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN});

      const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = PRIMITIVE_TOPOLOGY_MAP[desc.input_layout.topology],
        .primitiveRestartEnable = VK_FALSE,
      };

      const VkViewport viewport = {
        .x = desc.viewport.x,
        .y = desc.viewport.y,
        .width = desc.viewport.width,
        .height = desc.viewport.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
      };

      const VkRect2D scissor = {
        .offset = {desc.scissor.offset.x, desc.scissor.offset.y},
        .extent = {desc.scissor.extent.x, desc.scissor.extent.y},
      };

      const VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
      };

      const VkPipelineRasterizationStateCreateInfo rasterizer_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk_cast(desc.raster.polygon_mode),
        .cullMode = vk_cast(desc.raster.cull_mode),
        .frontFace = vk_cast(desc.raster.front_face),
        .depthBiasEnable =
          (desc.depth_bias.slope != 0.0f || desc.depth_bias.constant != 0.0f) ? VK_TRUE : VK_FALSE,
        .depthBiasConstantFactor = desc.depth_bias.constant,
        .depthBiasSlopeFactor = desc.depth_bias.slope,
        .lineWidth = desc.raster.line_width,
      };

      const VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = vk_cast(sample_count),
        .sampleShadingEnable = VK_FALSE,
      };

      VkVertexInputAttributeDescription attr_descs[MAX_INPUT_PER_SHADER];
      u32 attr_desc_count = 0;
      for (u32 i = 0; i < MAX_INPUT_PER_SHADER; i++) {
        const auto& input_attribute = desc.input_attributes[i];
        if (input_attribute.type == VertexElementType::DEFAULT) {
          continue;
        }
        attr_descs[attr_desc_count].format = vk_cast(input_attribute.type, input_attribute.flags);
        attr_descs[attr_desc_count].binding = desc.input_attributes[i].binding;
        attr_descs[attr_desc_count].location = i;
        attr_descs[attr_desc_count].offset = desc.input_attributes[i].offset;
        attr_desc_count++;
      }

      VkVertexInputBindingDescription input_binding_descs[MAX_INPUT_BINDING_PER_SHADER];
      u32 input_binding_desc_count = 0;
      for (u32 i = 0; i < MAX_INPUT_BINDING_PER_SHADER; i++) {
        if (desc.input_bindings[i].stride == 0) {
          continue;
        }
        input_binding_descs[input_binding_desc_count] = {
          i, desc.input_bindings[i].stride, VK_VERTEX_INPUT_RATE_VERTEX};
        input_binding_desc_count++;
      }

      const VkPipelineVertexInputStateCreateInfo inputStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = attr_desc_count > 0 ? input_binding_desc_count : 0,
        .pVertexBindingDescriptions = attr_desc_count > 0 ? input_binding_descs : nullptr,
        .vertexAttributeDescriptionCount = attr_desc_count,
        .pVertexAttributeDescriptions = attr_descs,
      };

      VkPipelineColorBlendAttachmentState color_blend_attachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
      std::transform(
        desc.color_attachments,
        desc.color_attachments + MAX_COLOR_ATTACHMENT_PER_SHADER,
        color_blend_attachments,
        [](const auto& attachment) -> VkPipelineColorBlendAttachmentState {
          return {
            .blendEnable = attachment.blend_enable ? VK_TRUE : VK_FALSE,
            .srcColorBlendFactor = vk_cast(attachment.src_color_blend_factor),
            .dstColorBlendFactor = vk_cast(attachment.dst_color_blend_factor),
            .colorBlendOp = vk_cast(attachment.color_blend_op),
            .srcAlphaBlendFactor = vk_cast(attachment.src_alpha_blend_factor),
            .dstAlphaBlendFactor = vk_cast(attachment.dst_alpha_blend_factor),
            .alphaBlendOp = vk_cast(attachment.alpha_blend_op),
            .colorWriteMask = soul::cast<u32>(attachment.color_write ? 0xf : 0x0),
          };
        });

      const VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = desc.color_attachment_count,
        .pAttachments = color_blend_attachments,
        .blendConstants = {},
      };

      const VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = desc.depth_stencil_attachment.depth_test_enable,
        .depthWriteEnable = desc.depth_stencil_attachment.depth_write_enable,
        .depthCompareOp = vk_cast(desc.depth_stencil_attachment.depth_compare_op),
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
      };

      const VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = soul::cast<u32>(shader_stage_infos.size()),
        .pStages = shader_stage_infos.data(),
        .pVertexInputState = &inputStateInfo,
        .pInputAssemblyState = &input_assembly_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_info,
        .pColorBlendState = &color_blending,
        .pDynamicState = nullptr,
        .layout = get_bindless_pipeline_layout(),
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
      };

      VkPipeline pipeline;
      {
        SOUL_PROFILE_ZONE_WITH_NAME("vkCreateGraphicsPipelines");
        SOUL_VK_CHECK(
          vkCreateGraphicsPipelines(
            _db.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline),
          "Fail to create graphics pipelines");
      }

      shader_stage_infos.cleanup();
      return {pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, desc.program_id};
    };
    return PipelineStateID(_db.pipeline_state_cache.create(
      key, create_graphic_pipeline_state, desc, render_pass, sample_count));
  }

  auto System::request_pipeline_state(const ComputePipelineStateDesc& desc) -> PipelineStateID
  {
    const auto key = PipelineStateKey::From(ComputePipelineStateKey{desc});
    if (const auto id = _db.pipeline_state_cache.find(key); id != PipelineStateCache::NULLVAL) {
      return PipelineStateID(id);
    }

    auto create_compute_pipeline_state =
      [this](const ComputePipelineStateDesc& desc) -> PipelineState {
      const auto& program = get_program(desc.program_id);

      SOUL_ASSERT(0, program.shaders.size() == 1, "");
      const Shader& compute_shader = program.shaders[0];
      SOUL_ASSERT(0, compute_shader.stage == ShaderStage::COMPUTE, "");
      const VkPipelineShaderStageCreateInfo compute_shader_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader.vk_handle,
        .pName = compute_shader.entry_point.data()};

      const VkComputePipelineCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = compute_shader_stage_create_info,
        .layout = get_bindless_pipeline_layout()};

      VkPipeline pipeline;
      {
        SOUL_PROFILE_ZONE_WITH_NAME("vkCreateGraphicsPipelines");
        SOUL_VK_CHECK(
          vkCreateComputePipelines(_db.device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline),
          "Fail to create compute pipelines");
      }
      return {pipeline, VK_PIPELINE_BIND_POINT_COMPUTE, desc.program_id};
    };

    return PipelineStateID(
      _db.pipeline_state_cache.create(key, create_compute_pipeline_state, desc));
  }

  auto System::get_pipeline_state(PipelineStateID pipeline_state_id) -> const PipelineState&
  {
    return *_db.pipeline_state_cache.get(pipeline_state_id.id);
  }

  auto System::get_bindless_pipeline_layout() const -> VkPipelineLayout
  {
    return _db.descriptor_allocator.get_pipeline_layout();
  }

  auto System::get_bindless_descriptor_sets() const -> BindlessDescriptorSets
  {
    return _db.descriptor_allocator.get_bindless_descriptor_sets();
  }

  auto System::create_program(const ProgramDesc& program_desc) -> Result<ProgramID, Error>
  {
    SOUL_ASSERT(0, program_desc.entry_points.size() != 0, "");
    runtime::ScopeAllocator<> scope_allocator("gpu::System::create_program(const ProgramDesc&");

    const auto program_id = ProgramID(_db.programs.create());
    Program& program = _db.programs[program_id.id];

    // ReSharper disable once CppLocalVariableMayBeConst
    auto* session = get_dxc_session();

    Vector<const CString*> shader_sources(&scope_allocator);
    shader_sources.reserve(program_desc.sources.size());
    Vector<CString> shader_file_sources(&scope_allocator);
    shader_file_sources.reserve(program_desc.sources.size());

    for (const auto& source : program_desc.sources) {
      if (std::holds_alternative<ShaderFile>(source)) {
        const auto& shader_file = std::get<ShaderFile>(source);
        const std::filesystem::path& path = shader_file.path;
        const std::filesystem::path full_path = [&path, &program_desc]() -> std::filesystem::path {
          if (path.is_absolute()) {
            SOUL_ASSERT(
              0, std::filesystem::exists(path), "%s does not exist", path.string().c_str());
            return canonical(path);
          }
          for (const auto& search_path : program_desc.search_paths) {
            auto full_path_candidate = search_path / path;
            if (exists(full_path_candidate)) {
              return canonical(full_path_candidate);
            }
          }
          SOUL_ASSERT(0, false, "Cannot find file {} in any search path", path.string().c_str());
          return "";
        }();
        auto full_path_str = full_path.string();
        shader_file_sources.push_back(load_file(full_path_str.c_str(), scope_allocator));
        shader_sources.push_back(&shader_file_sources.back());
      } else {
        const auto& shader_string = std::get<ShaderString>(source);
        shader_sources.push_back(&shader_string.source);
      }
    }

    auto total_source_size = std::accumulate(
      shader_sources.begin(),
      shader_sources.end(),
      usize(0),
      [](const usize prev_size, const CString* source) -> usize {
        return prev_size + source->size();
      });

    CString full_source_string(&scope_allocator);
    total_source_size += RESOURCE_HLSL_SIZE;
    total_source_size += RESOURCE_RT_EXT_HLSL_SIZE;
    full_source_string.reserve(total_source_size);
    full_source_string.append(RESOURCE_HLSL);
    full_source_string.append(RESOURCE_RT_EXT_HLSL);
    for (const auto* shader_source : shader_sources) {
      full_source_string.append(*shader_source);
    }
    SOUL_LOG_DEBUG("Full source : {}", full_source_string.data());

    auto code_page = soul::cast<u32>(CP_ACP);
    wrl::ComPtr<IDxcBlobEncoding> source_blob;
    if (auto result = session->utils->CreateBlob(
          full_source_string.data(),
          soul::cast<u32>(full_source_string.size()),
          code_page,
          source_blob.GetAddressOf());
        FAILED(result)) {
      SOUL_PANIC("Fail to create source blob");
    }

    for (const auto& entry_point : program_desc.entry_points) {
      const auto stage = entry_point.stage;
      const char* entry_point_name = entry_point.name;
      if (entry_point_name == nullptr) {
        continue;
      }

      // Tell the compiler to output SPIR-V
      Vector<LPCWSTR> arguments;

      arguments.push_back(L"-HV");
      arguments.push_back(L"2021");
      arguments.push_back(L"-spirv");

      // memory layout for resources
      arguments.push_back(L"-fvk-use-dx-layout");
      // Vulkan version
      arguments.push_back(L"-fspv-target-env=vulkan1.3");
      // useful extensions
      arguments.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
      arguments.push_back(L"-fspv-extension=SPV_KHR_ray_tracing");
      arguments.push_back(L"-fspv-extension=SPV_KHR_ray_query");

      const auto entry_point_name_size = strlen(entry_point.name) + 1;
      auto* entry_point_wide_chars = scope_allocator.allocate_array<wchar_t>(entry_point_name_size);
      size_t entry_point_out_size;
      const auto result = mbstowcs_s(
        &entry_point_out_size,
        entry_point_wide_chars,
        entry_point_name_size,
        entry_point.name,
        entry_point_name_size);
      SOUL_ASSERT(0, result != -1, "Invalid multibyte characters");
      arguments.push_back(L"-E");
      arguments.push_back(entry_point_wide_chars);

      static constexpr auto target_profile_map = FlagMap<ShaderStage, LPCWSTR>::from_key_val_list({
        {ShaderStage::VERTEX, L"vs_6_5"},
        {ShaderStage::FRAGMENT, L"ps_6_5"},
        {ShaderStage::COMPUTE, L"cs_6_5"},
        {ShaderStage::GEOMETRY, L"gs_6_5"},
        {ShaderStage::RAYGEN, L"lib_6_5"},
        {ShaderStage::CLOSEST_HIT, L"lib_6_5"},
        {ShaderStage::MISS, L"lib_6_5"},
      });
      arguments.push_back(L"-T");
      arguments.push_back(target_profile_map[stage]);

      if (program_desc.search_paths.size() > 0) {
        arguments.push_back(L"-I");
      }

      std::vector<std::wstring> search_paths;
      for (const auto& search_path : program_desc.search_paths) {
        search_paths.push_back(search_path.wstring());
        arguments.push_back(search_paths.back().c_str());
      }

      DxcBuffer source_buffer = {
        .Ptr = source_blob->GetBufferPointer(),
        .Size = source_blob->GetBufferSize(),
        .Encoding = 0,
      };

      // Compile shader
      wrl::ComPtr<IDxcResult> result_op;
      auto hres = session->compiler->Compile(
        &source_buffer,
        arguments.data(),
        soul::cast<uint32_t>(arguments.size()),
        session->default_include_handler.Get(),
        IID_PPV_ARGS(result_op.GetAddressOf()));

      if (SUCCEEDED(hres)) {
        result_op->GetStatus(&hres);
      }

      wrl::ComPtr<IDxcBlobUtf8> pErrors = nullptr;
      // Note that d3dcompiler would return null if no errors or warnings are present.
      // IDxcCompiler3::Compile will always return an error buffer,
      // but its length will be zero if there are no warnings or errors.
      if (
        SUCCEEDED(result_op->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr)) &&
        pErrors != nullptr && pErrors->GetStringLength() != 0) {
        wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer()); // NOLINT
      }

      // Get compilation result
      wrl::ComPtr<IDxcBlob> code;
      result_op->GetResult(&code);

      // Create a Vulkan shader module from the compilation result
      const VkShaderModuleCreateInfo shader_module_ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code->GetBufferSize(),
        .pCode = soul::cast<u32*>(code->GetBufferPointer())};
      VkShaderModule shader_module;
      vkCreateShaderModule(_db.device, &shader_module_ci, nullptr, &shader_module);
      program.shaders.push_back(Shader{
        .stage = stage,
        .vk_handle = shader_module,
        .entry_point = CString::From(entry_point.name),
      });
    }

    return Result<ProgramID, Error>::ok(program_id);
  }

  auto System::get_program(ProgramID program_id) const -> const Program&
  {
    SOUL_ASSERT(0, program_id.is_valid(), "");
    return _db.programs[program_id.id];
  }

  auto System::get_program(ProgramID program_id) -> Program&
  {
    SOUL_ASSERT(0, program_id.is_valid(), "");
    return _db.programs[program_id.id];
  }

  auto System::create_shader_table(const ShaderTableDesc& shader_table_desc) -> ShaderTableID
  {
    runtime::ScopeAllocator<> scope_allocator(
      "gpu::System::create_shader_table(const ShaderTableDesc&)");
    const auto& program = get_program(shader_table_desc.program_id);
    const auto id = ShaderTableID(_db.shader_table_pool.create());
    auto& shader_table = get_shader_table(id);

    SBOVector<VkRayTracingShaderGroupCreateInfoKHR> sh_group_infos;

    const VkRayTracingShaderGroupCreateInfoKHR sh_raygen_group_create_info = {
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
      .generalShader = shader_table_desc.raygen_group.entry_point,
      .closestHitShader = VK_SHADER_UNUSED_KHR,
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = VK_SHADER_UNUSED_KHR,
    };
    sh_group_infos.push_back(sh_raygen_group_create_info);

    for (const auto& miss_group : shader_table_desc.miss_groups) {
      const VkRayTracingShaderGroupCreateInfoKHR sh_group_create_info = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader = miss_group.entry_point,
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
      };
      sh_group_infos.push_back(sh_group_create_info);
    }

    for (const auto hit_group : shader_table_desc.hit_groups) {
      const VkRayTracingShaderGroupCreateInfoKHR sh_group_create_info = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        .generalShader = VK_SHADER_UNUSED_KHR,
        .closestHitShader = hit_group.closest_hit_entry_point,
        .anyHitShader = hit_group.any_hit_entry_point,
        .intersectionShader = hit_group.intersection_entry_point,
      };
      sh_group_infos.push_back(sh_group_create_info);
    }

    SBOVector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
    for (const auto& shader : program.shaders) {
      if (shader.vk_handle == VK_NULL_HANDLE) {
        continue;
      }
      const VkPipelineShaderStageCreateInfo shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = vk_cast(shader.stage),
        .module = shader.vk_handle,
        .pName = shader.entry_point.data(),
      };
      shader_stage_infos.push_back(shader_stage_info);
    }

    const VkRayTracingPipelineCreateInfoKHR rt_pipeline_create_info = {
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
      .stageCount = soul::cast<u32>(shader_stage_infos.size()),
      .pStages = shader_stage_infos.data(),
      .groupCount = soul::cast<u32>(sh_group_infos.size()),
      .pGroups = sh_group_infos.data(),
      .maxPipelineRayRecursionDepth = shader_table_desc.max_recursion_depth,
      .layout = get_bindless_pipeline_layout(),
    };
    vkCreateRayTracingPipelinesKHR(
      _db.device, {}, {}, 1, &rt_pipeline_create_info, nullptr, &shader_table.pipeline);

    if (shader_table_desc.name != nullptr) {
      CString pipeline_name(&scope_allocator);
      pipeline_name.appendf("{}_pipeline({})", shader_table_desc.name, _db.frame_counter);
      const VkDebugUtilsObjectNameInfoEXT pipeline_name_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_PIPELINE,
        reinterpret_cast<u64>(shader_table.pipeline),
        pipeline_name.data(),
      };
      vkSetDebugUtilsObjectNameEXT(_db.device, &pipeline_name_info);
    }

    const auto total_group_count =
      1 + shader_table_desc.miss_groups.size() + shader_table_desc.hit_groups.size();

    const auto handle_size = _db.ray_tracing_properties.shaderGroupHandleSize;
    const auto sbt_size = total_group_count * handle_size;
    auto shader_handle_storage = scope_allocator.allocate_array<u8>(sbt_size);

    SOUL_VK_CHECK(
      vkGetRayTracingShaderGroupHandlesKHR(
        _db.device, shader_table.pipeline, 0, total_group_count, sbt_size, shader_handle_storage),
      "Fail to get ray tracing shader group handles");

    const auto handle_size_aligned =
      util::align_up(handle_size, _db.ray_tracing_properties.shaderGroupHandleAlignment);
    const auto handle_base_alignment = _db.ray_tracing_properties.shaderGroupBaseAlignment;

    FlagMap<ShaderGroup, usize> group_counts = {};
    group_counts[ShaderGroup::RAYGEN] = 1;
    group_counts[ShaderGroup::MISS] = shader_table_desc.miss_groups.size();
    group_counts[ShaderGroup::HIT] = shader_table_desc.hit_groups.size();
    group_counts[ShaderGroup::CALLABLE] = 0;

    auto strides = FlagMap<ShaderGroup, usize>::init_fill(handle_size_aligned);
    strides[ShaderGroup::RAYGEN] = util::align_up(handle_size_aligned, handle_base_alignment);

    const auto group_names = FlagMap<ShaderGroup, const char*>::from_key_val_list({
      {ShaderGroup::RAYGEN, "raygen"},
      {ShaderGroup::MISS, "miss"},
      {ShaderGroup::HIT, "hit"},
      {ShaderGroup::CALLABLE, "callable"},
    });

    usize current_storage_offset = 0;
    for (auto shader_group : FlagIter<ShaderGroup>()) {
      const auto buffer_size = group_counts[shader_group] * strides[shader_group];
      if (buffer_size == 0) {
        continue;
      }
      auto records = scope_allocator.allocate_array<u8>(buffer_size);
      memcpy(
        records,
        shader_handle_storage + current_storage_offset,
        handle_size * group_counts[shader_group]);
      current_storage_offset += handle_size * group_counts[shader_group];

      CString buffer_name(&scope_allocator);
      if (shader_table_desc.name != nullptr) {
        buffer_name.appendf("{}_{}", shader_table_desc.name, group_names[shader_group]);
      }

      const BufferDesc buffer_desc = {
        .size = buffer_size,
        .usage_flags = {BufferUsage::SHADER_BINDING_TABLE},
        .queue_flags = {QueueType::COMPUTE},
        .name = buffer_name.data()};
      shader_table.buffers[shader_group] = create_buffer(buffer_desc, records);
      flush_buffer(shader_table.buffers[shader_group]);

      shader_table.vk_regions[shader_group] = {
        .deviceAddress = get_gpu_address(shader_table.buffers[shader_group]).id,
        .stride = strides[shader_group],
        .size = buffer_size};
    }

    return id;
  }

  auto System::destroy_shader_table(ShaderTableID shader_table_id) -> void
  {
    auto& shader_table = get_shader_table(shader_table_id);
    auto& frame_context = get_frame_context();
    frame_context.garbages.pipelines.push_back(shader_table.pipeline);
    for (const auto buffer_id : shader_table.buffers) {
      destroy_buffer(buffer_id);
    }
    _db.shader_table_pool.destroy(shader_table_id.id);
  }

  auto System::get_shader_table(ShaderTableID shader_table_id) const -> const ShaderTable&
  {
    return *_db.shader_table_pool.get(shader_table_id.id);
  }

  auto System::get_shader_table(ShaderTableID shader_table_id) -> ShaderTable&
  {
    return *_db.shader_table_pool.get(shader_table_id.id);
  }

  auto System::request_sampler(const SamplerDesc& desc) -> SamplerID
  {
    const auto hash_key = util::hash_fnv1(&desc);
    if (_db.sampler_map.is_exist(hash_key)) {
      return _db.sampler_map[hash_key];
    }

    const VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = vk_cast(desc.mag_filter),
      .minFilter = vk_cast(desc.min_filter),
      .mipmapMode = vk_cast_mipmap_filter(desc.mipmap_filter),
      .addressModeU = vk_cast(desc.wrap_u),
      .addressModeV = vk_cast(desc.wrap_v),
      .addressModeW = vk_cast(desc.wrap_w),
      .anisotropyEnable = cast<VkBool32>(desc.anisotropy_enable),
      .maxAnisotropy = desc.max_anisotropy,
      .compareEnable = desc.compare_enable ? VK_TRUE : VK_FALSE,
      .compareOp = vk_cast(desc.compare_op),
      .minLod = 0,
      .maxLod = VK_LOD_CLAMP_NONE,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
    };

    VkSampler sampler;
    SOUL_VK_CHECK(
      vkCreateSampler(_db.device, &sampler_create_info, nullptr, &sampler),
      "Fail to create sampler");
    const DescriptorID descriptor_id = _db.descriptor_allocator.create_sampler_descriptor(sampler);
    const SamplerID sampler_id = {sampler, descriptor_id};
    _db.sampler_map.add(hash_key, sampler_id);

    return sampler_id;
  }

  auto System::request_render_pass(const RenderPassKey& key) -> VkRenderPass
  {
    SOUL_PROFILE_ZONE();
    SOUL_ASSERT_MAIN_THREAD();
    if (_db.render_pass_maps.contains(key)) {
      return _db.render_pass_maps[key];
    }

    auto attachment_flag_to_load_op = [](const AttachmentFlags flags) -> VkAttachmentLoadOp {
      if (flags & ATTACHMENT_CLEAR_BIT) {
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
      }
      if ((flags & ATTACHMENT_FIRST_PASS_BIT) && !(flags & ATTACHMENT_EXTERNAL_BIT)) {
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      }

      return VK_ATTACHMENT_LOAD_OP_LOAD;
    };

    auto attachment_flag_to_store_op = [](const AttachmentFlags flags) -> VkAttachmentStoreOp {
      if ((flags & ATTACHMENT_LAST_PASS_BIT) && !(flags & ATTACHMENT_EXTERNAL_BIT)) {
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
      }
      return VK_ATTACHMENT_STORE_OP_STORE;
    };

    VkAttachmentDescription
      attachments[MAX_COLOR_ATTACHMENT_PER_SHADER * 2 + MAX_INPUT_ATTACHMENT_PER_SHADER + 1] = {};
    VkAttachmentReference attachment_refs[MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
    u8 attachment_count = 0;

    for (const auto color_attachment : key.color_attachments) {
      if (color_attachment.flags & ATTACHMENT_ACTIVE_BIT) {
        VkAttachmentDescription& attachment = attachments[attachment_count];
        attachment.format = vk_cast(color_attachment.format);
        attachment.samples = vk_cast(color_attachment.sampleCount);
        const auto flags = color_attachment.flags;

        attachment.loadOp = attachment_flag_to_load_op(flags);
        attachment.storeOp = attachment_flag_to_store_op(flags);

        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachment_refs[attachment_count].attachment = attachment_count;
        attachment_refs[attachment_count].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachment_count++;
      }
    }
    const auto color_attachment_count = attachment_count;

    for (auto input_attachment : key.input_attachments) {
      const Attachment& attachment_key = input_attachment;
      if (attachment_key.flags & ATTACHMENT_ACTIVE_BIT) {
        VkAttachmentDescription& attachment = attachments[attachment_count];
        attachment.format = vk_cast(attachment_key.format);
        attachment.samples = vk_cast(input_attachment.sampleCount);

        const auto flags = attachment_key.flags;

        attachment.loadOp = attachment_flag_to_load_op(flags);
        attachment.storeOp = attachment_flag_to_store_op(flags);

        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachment_refs[attachment_count].attachment = attachment_count;
        attachment_refs[attachment_count].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        attachment_count++;
      }
    }
    const u8 input_attachment_count = attachment_count - color_attachment_count;

    for (const auto resolve_attachment : key.resolve_attachments) {
      if ((resolve_attachment.flags & ATTACHMENT_ACTIVE_BIT) != 0) {
        VkAttachmentDescription& attachment = attachments[attachment_count];
        attachment.format = vk_cast(resolve_attachment.format);
        attachment.samples = vk_cast(resolve_attachment.sampleCount);
        const auto flags = resolve_attachment.flags;

        attachment.loadOp = attachment_flag_to_load_op(flags);
        attachment.storeOp = attachment_flag_to_store_op(flags);

        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachment_refs[attachment_count].attachment = attachment_count;
        attachment_refs[attachment_count].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachment_count++;
      }
    }
    const u8 resolve_attachment_count =
      attachment_count - (input_attachment_count + color_attachment_count);

    SOUL_ASSERT(
      0, resolve_attachment_count == color_attachment_count || resolve_attachment_count == 0, "");
    VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = input_attachment_count,
      .pInputAttachments = attachment_refs + color_attachment_count,
      .colorAttachmentCount = color_attachment_count,
      .pColorAttachments = attachment_refs,
      .pResolveAttachments = resolve_attachment_count > 0
                               ? attachment_refs + color_attachment_count + input_attachment_count
                               : nullptr};

    if ((key.depth_attachment.flags & ATTACHMENT_ACTIVE_BIT) != 0) {
      const auto flags = key.depth_attachment.flags;
      VkAttachmentDescription& attachment = attachments[attachment_count];
      attachment.format = vk_cast(key.depth_attachment.format);
      attachment.samples = vk_cast(key.depth_attachment.sampleCount);

      attachment.loadOp = attachment_flag_to_load_op(flags);
      attachment.storeOp = attachment_flag_to_store_op(flags);

      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentReference& depthAttachmentRef = attachment_refs[attachment_count];
      depthAttachmentRef.attachment = attachment_count;
      depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      subpass.pDepthStencilAttachment = &depthAttachmentRef;
      attachment_count++;
    }

    const VkRenderPassCreateInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = attachment_count,
      .pAttachments = attachments,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 0,
      .pDependencies = nullptr,
    };

    VkRenderPass render_pass;
    SOUL_VK_CHECK(
      vkCreateRenderPass(_db.device, &render_pass_info, nullptr, &render_pass),
      "Fail to create render pass");

    _db.render_pass_maps.insert(key, render_pass);

    return render_pass;
  }

  auto System::create_framebuffer(const VkFramebufferCreateInfo& info) -> VkFramebuffer
  {
    SOUL_PROFILE_ZONE();
    VkFramebuffer framebuffer;
    SOUL_VK_CHECK(
      vkCreateFramebuffer(_db.device, &info, nullptr, &framebuffer), "Fail to create framebuffer");
    return framebuffer;
  }

  auto System::destroy_framebuffer(VkFramebuffer framebuffer) -> void
  {
    get_frame_context().garbages.frame_buffers.push_back(framebuffer);
  }

  auto System::create_event() -> VkEvent
  {
    SOUL_ASSERT_MAIN_THREAD();
    VkEvent event;
    const VkEventCreateInfo event_info = {.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};
    SOUL_VK_CHECK(vkCreateEvent(_db.device, &event_info, nullptr, &event), "Fail to create event");
    return event;
  }

  auto System::destroy_event(VkEvent event) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    get_frame_context().garbages.events.push_back(event);
  }

  auto System::create_binary_semaphore() -> BinarySemaphore
  {
    SOUL_ASSERT_MAIN_THREAD();
    BinarySemaphore semaphore = {VK_NULL_HANDLE};
    const VkSemaphoreCreateInfo semaphore_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    SOUL_VK_CHECK(
      vkCreateSemaphore(_db.device, &semaphore_info, nullptr, &semaphore.vk_handle),
      "Fail to create semaphore");
    return semaphore;
  }

  auto System::destroy_binary_semaphore(BinarySemaphore semaphore) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    get_frame_context().garbages.semaphores.push_back(semaphore);
  }

  auto System::calculate_gpu_properties() -> void
  {
    _db.gpu_properties = {
      .limit =
        {
          .color_sample_count_flags =
            soul_cast(_db.physical_device_properties.limits.framebufferColorSampleCounts),
          .depth_sample_count_flags =
            soul_cast(_db.physical_device_properties.limits.framebufferDepthSampleCounts),
        },
    };
  }

  auto System::get_as_build_size_info(const TlasBuildDesc& build_desc)
    -> VkAccelerationStructureBuildSizesInfoKHR
  {
    const VkAccelerationStructureGeometryInstancesDataKHR as_instance = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, .data = {}};

    const VkAccelerationStructureGeometryKHR as_geometry = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
      .geometry = {.instances = as_instance},
    };

    const VkAccelerationStructureBuildGeometryInfoKHR build_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags = vk_cast(build_desc.build_flags),
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .geometryCount = 1,
      .pGeometries = &as_geometry,
    };

    VkAccelerationStructureBuildSizesInfoKHR size_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(
      _db.device,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &build_info,
      &build_desc.instance_count,
      &size_info);

    return size_info;
  }

  auto System::get_as_build_size_info(const BlasBuildDesc& build_desc)
    -> VkAccelerationStructureBuildSizesInfoKHR
  {
    auto as_geometries =
      Vector<VkAccelerationStructureGeometryKHR>::WithSize(build_desc.geometry_count);
    std::ranges::transform(
      build_desc.geometry_descs,
      build_desc.geometry_descs + build_desc.geometry_count,
      as_geometries.begin(),
      [](const RTGeometryDesc& geometry_desc) -> VkAccelerationStructureGeometryKHR {
        const auto geometry_data =
          [](const RTGeometryDesc& desc) -> VkAccelerationStructureGeometryDataKHR {
          if (desc.type == RTGeometryType::TRIANGLE) {
            const auto& triangle_desc = desc.content.triangles;
            return {
              .triangles = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = vk_cast(triangle_desc.vertex_format),
                .vertexStride = triangle_desc.vertex_stride,
                .maxVertex = triangle_desc.vertex_count,
                .indexType = vk_cast(triangle_desc.index_type),
              }};
          }
          SOUL_ASSERT(0, desc.type == RTGeometryType::AABB, "");
          const auto& aabb_desc = desc.content.aabbs;
          return {
            .aabbs = {
              .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
              .stride = aabb_desc.stride}};
        }(geometry_desc);

        return {
          .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
          .geometryType = vk_cast(geometry_desc.type),
          .geometry = geometry_data,
          .flags = vk_cast(geometry_desc.flags),
        };
      });

    const VkAccelerationStructureBuildGeometryInfoKHR build_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .flags = vk_cast(build_desc.flags),
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .geometryCount = soul::cast<u32>(as_geometries.size()),
      .pGeometries = as_geometries.data(),
    };

    auto max_primitives_counts = Vector<u32>::WithSize(build_desc.geometry_count);
    std::transform(
      build_desc.geometry_descs,
      build_desc.geometry_descs + build_desc.geometry_count,
      max_primitives_counts.begin(),
      [](const RTGeometryDesc& desc) -> u32 {
        if (desc.type == RTGeometryType::TRIANGLE) {
          return desc.content.triangles.index_count / 3;
        }
        SOUL_ASSERT(0, desc.type == RTGeometryType::AABB, "");
        return desc.content.aabbs.count;
      });

    VkAccelerationStructureBuildSizesInfoKHR size_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(
      _db.device,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &build_info,
      max_primitives_counts.data(),
      &size_info);
    return size_info;
  }

  auto System::get_as_build_size_info(
    const VkAccelerationStructureBuildGeometryInfoKHR& build_info, const u32* max_primitives_counts)
    -> VkAccelerationStructureBuildSizesInfoKHR
  {
    VkAccelerationStructureBuildSizesInfoKHR size_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(
      _db.device,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &build_info,
      max_primitives_counts,
      &size_info);
    return size_info;
  }

  auto System::add_to_blas_group(BlasID blas_id, BlasGroupID blas_group_id) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    auto& blas = get_blas(blas_id);
    auto& blas_group = get_blas_group(blas_group_id);
    blas.group_data = {.group_id = blas_group_id, .index = blas_group.blas_list.add(blas_id)};
  }

  auto System::remove_from_blas_group(BlasID blas_id) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    auto& blas = get_blas(blas_id);
    auto& blas_group = get_blas_group(blas.group_data.group_id);
    auto& blas_list = blas_group.blas_list;
    if (blas_list.back() != blas_id) {
      auto& last_blas = get_blas(blas_list.back());
      last_blas.group_data.index = blas.group_data.index;
      blas_list[blas.group_data.index] = blas_list.back();
    }
    blas.group_data.group_id = BlasGroupID::null();
    blas_list.pop_back();
  }

  auto CommandQueue::init(VkDevice device, u32 family_index, u32 queue_index) -> void
  {
    device_ = device;
    family_index_ = family_index;
    vkGetDeviceQueue(device_, family_index_, queue_index, &vk_handle_);
    init_timeline_semaphore();
  }

  auto CommandQueue::wait(Semaphore semaphore, VkPipelineStageFlags wait_stages) -> void
  {
    if (std::holds_alternative<BinarySemaphore*>(semaphore)) {
      wait(std::get<BinarySemaphore*>(semaphore), wait_stages);
    } else {
      wait(std::get<TimelineSemaphore>(semaphore), wait_stages);
    }
  }

  auto CommandQueue::wait(BinarySemaphore* semaphore, const VkPipelineStageFlags wait_stages)
    -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_ASSERT(0, wait_stages != 0, "");

    if (!commands_.empty()) {
      flush();
    }

    SOUL_ASSERT(0, semaphore->state == BinarySemaphore::State::SIGNALLED, "");
    semaphore->state = BinarySemaphore::State::WAITED;
    wait_semaphores_.push_back(semaphore->vk_handle);
    wait_stages_.push_back(wait_stages);
    wait_timeline_values_.push_back(0);
  }

  auto CommandQueue::wait(TimelineSemaphore semaphore, VkPipelineStageFlags wait_stages) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_ASSERT(0, wait_stages != 0, "");

    if (!commands_.empty()) {
      flush();
    }

    wait_semaphores_.push_back(semaphore.vk_handle);
    wait_stages_.push_back(wait_stages);
    wait_timeline_values_.push_back(semaphore.counter);
  }

  auto CommandQueue::get_timeline_semaphore() -> TimelineSemaphore
  {
    if (!wait_semaphores_.empty() || !commands_.empty()) {
      flush();
    }
    return {family_index_, timeline_semaphore_, current_timeline_values_};
  }

  auto CommandQueue::submit(
    const PrimaryCommandBuffer command_buffer, BinarySemaphore* binary_semaphore) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_PROFILE_ZONE();

    SOUL_VK_CHECK(vkEndCommandBuffer(command_buffer.get_vk_handle()), "Fail to end command buffer");

    commands_.push_back(command_buffer.get_vk_handle());

    // TODO : Fix this
    if (binary_semaphore != nullptr && binary_semaphore->vk_handle != VK_NULL_HANDLE) {
      binary_semaphore->state = BinarySemaphore::State::SIGNALLED;
      flush(binary_semaphore);
    }
  }

  auto CommandQueue::flush(BinarySemaphore* binary_semaphore) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_PROFILE_ZONE();
    SOUL_ASSERT(0, wait_semaphores_.size() == wait_stages_.size(), "");
    SOUL_ASSERT(0, wait_semaphores_.size() == wait_timeline_values_.size(), "");

    current_timeline_values_++;

    const u32 binary_semaphore_count =
      binary_semaphore == nullptr || binary_semaphore->is_null() ? 0 : 1;

    VkSemaphore signal_semaphores[MAX_SIGNAL_SEMAPHORE + 1];
    if (binary_semaphore != nullptr) {
      signal_semaphores[0] = binary_semaphore->vk_handle;
    }
    signal_semaphores[binary_semaphore_count] = timeline_semaphore_;

    u64 signal_semaphore_values[MAX_SIGNAL_SEMAPHORE + 1];
    std::fill_n(signal_semaphore_values, binary_semaphore_count, 0);
    signal_semaphore_values[binary_semaphore_count] = current_timeline_values_;

    const VkTimelineSemaphoreSubmitInfo timeline_submit_info = {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
      .waitSemaphoreValueCount = soul::cast<u32>(wait_timeline_values_.size()),
      .pWaitSemaphoreValues = wait_timeline_values_.data(),
      .signalSemaphoreValueCount = binary_semaphore_count + 1,
      .pSignalSemaphoreValues = signal_semaphore_values,
    };

    const VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = &timeline_submit_info,
      .waitSemaphoreCount = soul::cast<u32>(wait_semaphores_.size()),
      .pWaitSemaphores = wait_semaphores_.data(),
      .pWaitDstStageMask = wait_stages_.data(),
      .commandBufferCount = soul::cast<u32>(commands_.size()),
      .pCommandBuffers = commands_.data(),
      .signalSemaphoreCount = binary_semaphore_count + 1,
      .pSignalSemaphores = signal_semaphores,
    };

    SOUL_VK_CHECK(
      vkQueueSubmit(vk_handle_, 1, &submit_info, VK_NULL_HANDLE), "Fail to submit queue");
    commands_.clear();
    wait_semaphores_.clear();
    wait_timeline_values_.clear();
    wait_stages_.clear();
  }

  auto CommandQueue::present(
    VkSwapchainKHR swapchain, u32 swapchain_index, BinarySemaphore* semaphore) -> void
  {
    semaphore->state = BinarySemaphore::State::WAITED;
    const VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &(semaphore->vk_handle),
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &swapchain_index,
    };
    SOUL_VK_CHECK(vkQueuePresentKHR(vk_handle_, &present_info), "Fail to present queue");
  }

  auto CommandQueue::is_waiting_binary_semaphore() const -> b8
  {
    return std::ranges::any_of(
      wait_timeline_values_, [](u64 timeline_value) { return timeline_value == 0; });
  }

  auto CommandQueue::is_waiting_timeline_semaphore() const -> b8
  {
    return std::ranges::any_of(
      wait_timeline_values_, [](u64 timeline_value) { return timeline_value != 0; });
  }

  auto CommandQueue::init_timeline_semaphore() -> void
  {
    const VkSemaphoreTypeCreateInfo type_create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      .initialValue = 0};
    const VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &type_create_info};
    vkCreateSemaphore(device_, &create_info, nullptr, &timeline_semaphore_);
    current_timeline_values_ = 0;
  }

  auto System::execute(const RenderGraph& render_graph) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_PROFILE_ZONE();

    RenderGraphExecution execution(
      &render_graph,
      this,
      runtime::get_context_allocator(),
      &_db.queues,
      &get_frame_context().command_pools);
    execution.init();
    get_frame_context().gpu_resource_initializer.flush(_db.queues, *this);
    get_frame_context().gpu_resource_finalizer.flush(
      get_frame_context().command_pools, _db.queues, *this);
    execution.run();
    execution.cleanup();
  }

  auto System::flush() -> void
  {
    for (auto& queue : _db.queues) {
      queue.flush();
    }
    vkDeviceWaitIdle(_db.device);
  }

  auto System::flush_frame() -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_PROFILE_ZONE();
    end_frame();
    begin_frame();
  }

  auto System::begin_frame() -> void
  {
    SOUL_PROFILE_ZONE();
    SOUL_ASSERT_MAIN_THREAD();

    _db.current_frame += 1;
    _db.current_frame %= _db.frame_contexts.size();

    FrameContext& frame_context = get_frame_context();
    {
      SOUL_PROFILE_ZONE_WITH_NAME("Wait fence");
      if (frame_context.frame_end_semaphore.is_valid()) {
        const VkSemaphoreWaitInfo wait_info = {
          .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
          .semaphoreCount = 1,
          .pSemaphores = &frame_context.frame_end_semaphore.vk_handle,
          .pValues = &frame_context.frame_end_semaphore.counter,
        };
        SOUL_VK_CHECK(
          vkWaitSemaphores(_db.device, &wait_info, UINT64_MAX), "Fail to wait semaphore");
      }
    }

    {
      SOUL_PROFILE_ZONE_WITH_NAME("Reset command pools");
      frame_context.command_pools.reset();
    }

    frame_context.gpu_resource_initializer.reset();

    auto& garbages = frame_context.garbages;

    {
      // clear swapchain garbage
      for (const auto image_view : garbages.swapchain.image_views) {
        vkDestroyImageView(_db.device, image_view, nullptr);
      }
      garbages.swapchain.image_views.resize(0);
      if (garbages.swapchain.vk_handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_db.device, garbages.swapchain.vk_handle, nullptr);
      }
    }

    {
      SOUL_PROFILE_ZONE_WITH_NAME("Destroy images");
      auto destroy_texture_view = [this](TextureView& texture_view) {
        vkDestroyImageView(_db.device, texture_view.vk_handle, nullptr);
      };
      for (const auto texture_id : garbages.textures) {
        destroy_texture_descriptor(texture_id);
        auto& texture = get_texture(texture_id);
        vmaDestroyImage(_db.gpu_allocator, texture.vk_handle, texture.allocation);
        destroy_texture_view(texture.view);
        const auto view_count = texture.desc.mip_levels * texture.desc.layer_count;
        if (texture.views != nullptr) {
          std::for_each_n(texture.views, view_count, destroy_texture_view);
          _db.cpu_allocator.deallocate_array(texture.views, view_count);
          texture.views = nullptr;
        }
        _db.texture_pool.destroy(texture_id.id);
      }
      garbages.textures.resize(0);
    }

    {
      SOUL_PROFILE_ZONE_WITH_NAME("Destroy buffers");
      for (const BufferID buffer_id : garbages.buffers) {
        destroy_buffer_descriptor(buffer_id);
        const auto& buffer = get_buffer(buffer_id);
        vmaDestroyBuffer(_db.gpu_allocator, buffer.vk_handle, buffer.allocation);
        _db.buffer_pool.destroy(buffer_id.id);
      }
      garbages.buffers.resize(0);
    }

    {
      SOUL_PROFILE_ZONE_WITH_NAME("Destroy acceleration structures");
      for (const auto as_vk_handle : garbages.as_vk_handles) {
        vkDestroyAccelerationStructureKHR(_db.device, as_vk_handle, nullptr);
      }
      garbages.as_vk_handles.clear();
    }

    {
      SOUL_PROFILE_ZONE_WITH_NAME("Destroy as descriptors");
      for (const auto as_descriptor_id : garbages.as_descriptors) {
        _db.descriptor_allocator.destroy_as_descriptor(as_descriptor_id);
      }
      garbages.as_descriptors.clear();
    }

    {
      SOUL_PROFILE_ZONE_WITH_NAME("Destroy render passes");
      for (const auto renderPass : garbages.render_passes) {
        vkDestroyRenderPass(_db.device, renderPass, nullptr);
      }
      garbages.render_passes.resize(0);
    }

    for (const auto framebuffer : garbages.frame_buffers) {
      vkDestroyFramebuffer(_db.device, framebuffer, nullptr);
    }
    garbages.frame_buffers.resize(0);

    for (const auto pipeline : garbages.pipelines) {
      vkDestroyPipeline(_db.device, pipeline, nullptr);
    }
    garbages.pipelines.resize(0);

    for (const auto event : garbages.events) {
      vkDestroyEvent(_db.device, event, nullptr);
    }
    garbages.events.resize(0);

    for (const auto semaphore : garbages.semaphores) {
      vkDestroySemaphore(_db.device, semaphore.vk_handle, nullptr);
    }
    garbages.semaphores.resize(0);

    _db.pipeline_state_cache.on_new_frame();

    const auto result = acquire_swapchain();
    SOUL_ASSERT(0, result == VK_SUCCESS, "");
  }

  auto System::end_frame() -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    SOUL_PROFILE_ZONE();
    FrameContext& frame_context = get_frame_context();
    const auto swapchain_index = frame_context.swapchain_index;

    auto& swapchain_texture = get_texture(_db.swapchain.textures[swapchain_index]);
    {
      SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::LastSubmission");
      frame_context.gpu_resource_initializer.flush(_db.queues, *this);
      frame_context.gpu_resource_finalizer.flush(
        get_frame_context().command_pools, _db.queues, *this);

      SOUL_ASSERT(0, swapchain_texture.cache_state.queue_owner == QueueType::GRAPHIC, "");
      // TODO: Handle when swapchain texture is untouch (ResourceOwner is PRESENTATION_ENGINE)
      const auto cmd_buffer =
        frame_context.command_pools.request_command_buffer(QueueType::GRAPHIC);

      const VkImageMemoryBarrier image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = swapchain_texture.layout,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_texture.vk_handle,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = VK_REMAINING_MIP_LEVELS,
          .baseArrayLayer = 0,
          .layerCount = VK_REMAINING_ARRAY_LAYERS,
        }};

      vkCmdPipelineBarrier(
        cmd_buffer.get_vk_handle(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &image_barrier);

      auto sync_queue_to_graphic = [this](const QueueType queue_type) {
        const auto sync_counter = _db.queues[queue_type].get_timeline_semaphore();
        wait_sync_counter(sync_counter);
        _db.queues[QueueType::GRAPHIC].wait(
          _db.queues[queue_type].get_timeline_semaphore(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      };

      {
        SOUL_PROFILE_ZONE_WITH_NAME("Sync Transfer");
        sync_queue_to_graphic(QueueType::TRANSFER);
      }

      {
        SOUL_PROFILE_ZONE_WITH_NAME("Sync Compute");
        sync_queue_to_graphic(QueueType::COMPUTE);
      }

      if (frame_context.image_available_semaphore.state == BinarySemaphore::State::SIGNALLED) {
        _db.queues[QueueType::GRAPHIC].wait(
          &frame_context.image_available_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
      }

      const auto swapchain_owner = swapchain_texture.cache_state.queue_owner;
      _db.queues[swapchain_owner].submit(cmd_buffer, &frame_context.render_finished_semaphore);
      swapchain_texture.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    {
      SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::QueuePresent");
      _db.queues[QueueType::GRAPHIC].present(
        _db.swapchain.vk_handle,
        frame_context.swapchain_index,
        &frame_context.render_finished_semaphore);
    }

    frame_context.frame_end_semaphore = _db.queues[QueueType::GRAPHIC].get_timeline_semaphore();
    _db.frame_counter++;
  }

  auto System::recreate_swapchain() -> void
  {
    const auto framebuffer_size = _db.swapchain.wsi->get_framebuffer_size();
    auto& frame_context = get_frame_context();
    destroy_binary_semaphore(frame_context.image_available_semaphore);
    frame_context.image_available_semaphore = create_binary_semaphore();
    vkDeviceWaitIdle(_db.device);

    SOUL_LOG_INFO(
      "Recreate swapchain. Framebuffer Size = {} {}", framebuffer_size.x, framebuffer_size.y);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_db.physical_device, _db.surface, &_db.surface_caps);
    auto& swapchain_garbage = get_frame_context().garbages.swapchain;
    swapchain_garbage.vk_handle = _db.swapchain.vk_handle;

    _db.swapchain.extent = pick_surface_extent(_db.surface_caps, framebuffer_size);

    const VkSwapchainCreateInfoKHR swapchain_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = _db.surface,
      .minImageCount = _db.swapchain.image_count,
      .imageFormat = _db.swapchain.format.format,
      .imageColorSpace = _db.swapchain.format.colorSpace,
      .imageExtent = _db.swapchain.extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &_db.queue_family_indices[QueueType::GRAPHIC],
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = _db.swapchain.vk_handle,
    };
    SOUL_VK_CHECK(
      vkCreateSwapchainKHR(_db.device, &swapchain_info, nullptr, &_db.swapchain.vk_handle),
      "Fail to create vulkan swapchain!");
    vkGetSwapchainImagesKHR(
      _db.device, _db.swapchain.vk_handle, &_db.swapchain.image_count, nullptr);
    vkGetSwapchainImagesKHR(
      _db.device, _db.swapchain.vk_handle, &_db.swapchain.image_count, _db.swapchain.images.data());

    for (VkImageView image_view : _db.swapchain.image_views) {
      swapchain_garbage.image_views.push_back(image_view);
    }
    std::ranges::transform(
      _db.swapchain.images,
      _db.swapchain.image_views.begin(),
      [format = _db.swapchain.format.format, device = _db.device](VkImage image) {
        VkImageView image_view;
        const VkImageViewCreateInfo image_view_info = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = image,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = format,
          .components =
            {VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY},
          .subresourceRange =
            {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
            },
        };
        SOUL_VK_CHECK(
          vkCreateImageView(device, &image_view_info, nullptr, &image_view),
          "Fail to create swapchain imageview");
        return image_view;
      });

    for (const auto texture_id : _db.swapchain.textures) {
      _db.texture_pool.destroy(texture_id.id);
    }
    std::ranges::transform(
      _db.swapchain.images,
      _db.swapchain.image_views,
      _db.swapchain.textures.begin(),
      [this, image_sharing_mode = swapchain_info.imageSharingMode](
        VkImage image, VkImageView image_view) {
        return TextureID(_db.texture_pool.create(Texture{
          .desc = TextureDesc::d2(
            "Swapchain Texture",
            TextureFormat::BGRA8,
            1,
            {},
            {},
            vec2ui32(_db.swapchain.extent.width, _db.swapchain.extent.height)),
          .vk_handle = image,
          .view = {.vk_handle = image_view},
          .sharing_mode = image_sharing_mode}));
      });

    SOUL_LOG_INFO("Vulkan swapchain recreation sucessful");

    const auto result = acquire_swapchain();
    SOUL_ASSERT(0, result == VK_SUCCESS, "");
  }

  auto System::get_swapchain_extent() -> vec2ui32
  {
    return {_db.swapchain.extent.width, _db.swapchain.extent.height};
  }

  auto System::get_swapchain_texture() -> TextureID
  {
    return _db.swapchain.textures[get_frame_context().swapchain_index];
  }

  auto System::get_ssbo_descriptor_id(BufferID buffer_id) const -> DescriptorID
  {
    return get_buffer(buffer_id).storage_buffer_gpu_handle;
  }

  auto System::get_srv_descriptor_id(
    TextureID texture_id, std::optional<SubresourceIndex> subresource_index) -> DescriptorID
  {
    return get_texture_view(texture_id, subresource_index).sampled_image_gpu_handle;
  }

  auto System::get_uav_descriptor_id(
    TextureID texture_id, std::optional<SubresourceIndex> subresource_index) -> DescriptorID
  {
    return get_texture_view(texture_id, subresource_index).storage_image_gpu_handle;
  }

  auto System::get_sampler_descriptor_id(SamplerID sampler_id) const -> DescriptorID
  {
    return sampler_id.descriptorID;
  }

  auto System::get_as_descriptor_id(TlasID tlas_id) const -> DescriptorID
  {
    return get_tlas(tlas_id).descriptor_id;
  }

  auto SecondaryCommandBuffer::end() -> void { vkEndCommandBuffer(vk_handle_); }

  auto CommandPool::init(
    VkDevice device, const VkCommandBufferLevel level, const u32 queue_family_index) -> void
  {
    SOUL_ASSERT(0, device != VK_NULL_HANDLE, "Device is invalid!");
    device_ = device;
    level_ = level;
    const VkCommandPoolCreateInfo cmd_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = queue_family_index,
    };
    vkCreateCommandPool(device_, &cmd_pool_create_info, nullptr, &vk_handle_);
    count_ = 0;
  }

  auto CommandPool::request() -> VkCommandBuffer
  {
    if (allocated_buffers_.size() == count_) {
      VkCommandBuffer cmd_buffer;
      VkCommandBufferAllocateInfo alloc_info = {};
      alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      alloc_info.commandPool = vk_handle_;
      alloc_info.level = level_;
      alloc_info.commandBufferCount = 1;

      SOUL_VK_CHECK(
        vkAllocateCommandBuffers(device_, &alloc_info, &cmd_buffer),
        "Fail to allocate command buffers");
      allocated_buffers_.push_back(cmd_buffer);
    }

    VkCommandBuffer cmd_buffer = allocated_buffers_[count_];
    count_++;
    return cmd_buffer;
  }

  auto CommandPool::reset() -> void
  {
    vkResetCommandPool(device_, vk_handle_, 0);
    count_ = 0;
  }

  auto CommandPools::init(VkDevice device, const CommandQueues& queues, usize thread_count) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    runtime::push_allocator(allocator_);

    secondary_pools_.resize(thread_count);
    for (auto& pool : secondary_pools_) {
      pool.init(
        device, VK_COMMAND_BUFFER_LEVEL_SECONDARY, queues[QueueType::GRAPHIC].get_family_index());
    }

    primary_pools_.resize(thread_count);
    for (auto& pools : primary_pools_) {
      for (const auto queue_type : FlagIter<QueueType>()) {
        pools[queue_type].init(
          device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, queues[queue_type].get_family_index());
      }
    }

    runtime::pop_allocator();
  }

  auto CommandPools::reset() -> void
  {
    for (auto& pools : primary_pools_) {
      for (auto& pool : pools) {
        pool.reset();
      }
    }

    for (auto& pool : secondary_pools_) {
      pool.reset();
    }
  }

  auto CommandPools::request_command_buffer(const QueueType queue_type) -> PrimaryCommandBuffer
  {
    VkCommandBuffer cmd_buffer = primary_pools_[runtime::get_thread_id()][queue_type].request();
    const VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    SOUL_VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &begin_info), "");
    return PrimaryCommandBuffer(cmd_buffer);
  }

  auto CommandPools::request_secondary_command_buffer(
    VkRenderPass render_pass, const uint32_t subpass, VkFramebuffer framebuffer)
    -> SecondaryCommandBuffer
  {
    const auto cmd_buffer = secondary_pools_[runtime::get_thread_id()].request();
    const VkCommandBufferInheritanceInfo inheritance_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
      .renderPass = render_pass,
      .subpass = subpass,
      .framebuffer = framebuffer,
    };

    const VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
               VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
      .pInheritanceInfo = &inheritance_info,
    };

    vkBeginCommandBuffer(cmd_buffer, &begin_info);

    return SecondaryCommandBuffer(cmd_buffer);
  }

  auto GPUResourceInitializer::get_thread_context() -> GPUResourceInitializer::ThreadContext&
  {
    return thread_contexts_[runtime::get_thread_id()];
  }

  auto GPUResourceInitializer::get_staging_buffer(usize size)
    -> GPUResourceInitializer::StagingBuffer
  {
    const VkBufferCreateInfo staging_buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    const VmaAllocationCreateInfo staging_alloc_info = {.usage = VMA_MEMORY_USAGE_CPU_ONLY};

    StagingBuffer staging_buffer = {};
    SOUL_VK_CHECK(
      vmaCreateBuffer(
        gpu_allocator_,
        &staging_buffer_info,
        &staging_alloc_info,
        &staging_buffer.vk_handle,
        &staging_buffer.allocation,
        nullptr),
      "");
    get_thread_context().staging_buffers.push_back(staging_buffer);
    return staging_buffer;
  }

  auto GPUResourceInitializer::get_transfer_command_buffer() -> PrimaryCommandBuffer
  {
    if (get_thread_context().transfer_command_buffer.is_null()) {
      get_thread_context().transfer_command_buffer =
        command_pools_->request_command_buffer(QueueType::TRANSFER);
    }
    return get_thread_context().transfer_command_buffer;
  }

  auto GPUResourceInitializer::get_mipmap_gen_command_buffer() -> PrimaryCommandBuffer
  {
    if (get_thread_context().mipmap_gen_command_buffer.is_null()) {
      get_thread_context().mipmap_gen_command_buffer =
        command_pools_->request_command_buffer(QueueType::GRAPHIC);
    }
    return get_thread_context().mipmap_gen_command_buffer;
  }

  auto GPUResourceInitializer::get_clear_command_buffer() -> PrimaryCommandBuffer
  {
    if (get_thread_context().clear_command_buffer.is_null()) {
      get_thread_context().clear_command_buffer =
        command_pools_->request_command_buffer(QueueType::GRAPHIC);
    }
    return get_thread_context().clear_command_buffer;
  }

  auto GPUResourceInitializer::load_staging_buffer(
    const StagingBuffer& buffer, const void* data, usize size) -> void
  {
    void* mapped_data;
    vmaMapMemory(gpu_allocator_, buffer.allocation, &mapped_data);
    memcpy(mapped_data, data, size);
    vmaUnmapMemory(gpu_allocator_, buffer.allocation);
  }

  auto GPUResourceInitializer::load_staging_buffer(
    const StagingBuffer& buffer,
    const void* data,
    const usize count,
    const usize type_size,
    const usize stride) -> void
  {
    void* mapped_data;
    vmaMapMemory(gpu_allocator_, buffer.allocation, &mapped_data);
    const auto* src_base = soul::cast<byte*>(mapped_data);
    const auto* dst_base = soul::cast<byte*>(data);
    for (usize i = 0; i < count; i++) {
      memcpy(
        soul::cast<void*>(src_base + (i * stride)),
        soul::cast<void*>(dst_base + (i * type_size)),
        type_size);
    }
    vmaUnmapMemory(gpu_allocator_, buffer.allocation);
  }

  auto GPUResourceInitializer::init(VmaAllocator gpu_allocator, CommandPools* command_pools) -> void
  {
    gpu_allocator_ = gpu_allocator;
    command_pools_ = command_pools;
    thread_contexts_.resize(runtime::get_thread_count());
    for (auto& context : thread_contexts_) {
      memory::Allocator* default_allocator = get_default_allocator();
      context.staging_buffers = Vector<StagingBuffer>(default_allocator);
    }
  }

  auto GPUResourceInitializer::load(Buffer& buffer, const void* data) -> void
  {
    SOUL_ASSERT(
      0, buffer.cache_state.queue_owner == QueueType::NONE, "Buffer must be uninitialized!");
    const auto buffer_size = buffer.desc.size;
    if (buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      SOUL_ASSERT(0, buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "");
      void* mapped_data;
      vmaMapMemory(gpu_allocator_, buffer.allocation, &mapped_data);
      memcpy(mapped_data, data, buffer.desc.size);
      vmaUnmapMemory(gpu_allocator_, buffer.allocation);
    } else {
      const StagingBuffer staging_buffer = get_staging_buffer(buffer_size);
      load_staging_buffer(staging_buffer, data, 1, buffer.desc.size, buffer.desc.size);

      const VkBufferCopy copy_region = {.size = buffer_size};

      const auto* name = "Unknown texture";
      const VkDebugUtilsLabelEXT pass_label = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        name,
        {},
      };

      VkCommandBuffer transfer_command_buffer = get_transfer_command_buffer().get_vk_handle();
      vkCmdBeginDebugUtilsLabelEXT(transfer_command_buffer, &pass_label);
      vkCmdCopyBuffer(
        transfer_command_buffer, staging_buffer.vk_handle, buffer.vk_handle, 1, &copy_region);
      vkCmdEndDebugUtilsLabelEXT(transfer_command_buffer);

      buffer.cache_state.commit_access(
        QueueType::TRANSFER, {PipelineStage::TRANSFER}, {AccessType::TRANSFER_WRITE});
    }
  }

  auto GPUResourceInitializer::load(Texture& texture, const TextureLoadDesc& load_desc) -> void
  {
    SOUL_ASSERT(0, texture.layout == VK_IMAGE_LAYOUT_UNDEFINED, "Texture must be uninitialized!");
    SOUL_ASSERT(
      0, texture.cache_state.queue_owner == QueueType::NONE, "Texture must be uninitialized!");

    const auto staging_buffer = get_staging_buffer(load_desc.data_size);
    load_staging_buffer(staging_buffer, load_desc.data, load_desc.data_size);

    const VkImageMemoryBarrier before_transfer_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = texture.vk_handle,
      .subresourceRange = {
        .aspectMask = vk_cast_format_to_aspect_flags(texture.desc.format),
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
      }};

    const auto transfer_command_buffer = get_transfer_command_buffer().get_vk_handle();

    vkCmdPipelineBarrier(
      transfer_command_buffer,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &before_transfer_barrier);

    for (const auto region_load : load_desc.regions) {

      const VkBufferImageCopy copy_region = {
        .bufferOffset = region_load.buffer_offset,
        .bufferRowLength = region_load.buffer_row_length,
        .bufferImageHeight = region_load.buffer_image_height,
        .imageSubresource =
          {
            .aspectMask = vk_cast_format_to_aspect_flags(texture.desc.format),
            .mipLevel = region_load.subresource.mip_level,
            .baseArrayLayer = region_load.subresource.base_array_layer,
            .layerCount = region_load.subresource.layer_count,
          },
        .imageOffset =
          {.x = region_load.offset.x, .y = region_load.offset.y, .z = region_load.offset.z},
        .imageExtent = {
          .width = region_load.extent.x,
          .height = region_load.extent.y,
          .depth = region_load.extent.z,
        }};
      // SOUL_ASSERT(0, region_load.extent.z == 1, "3D texture is not supported yet");

      vkCmdCopyBufferToImage(
        transfer_command_buffer,
        staging_buffer.vk_handle,
        texture.vk_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy_region);
    }

    texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    texture.cache_state.commit_access(
      QueueType::TRANSFER, {PipelineStage::TRANSFER}, {AccessType::TRANSFER_WRITE});
  }

  auto GPUResourceInitializer::clear(Texture& texture, ClearValue clear_value) -> void
  {
    const VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = texture.vk_handle,
      .subresourceRange =
        {
          .aspectMask = vk_cast_format_to_aspect_flags(texture.desc.format),
          .baseMipLevel = 0,
          .levelCount = VK_REMAINING_MIP_LEVELS,
          .baseArrayLayer = 0,
          .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };

    VkCommandBuffer clear_command_buffer = get_clear_command_buffer().get_vk_handle();
    vkCmdPipelineBarrier(
      clear_command_buffer,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

    const VkImageSubresourceRange range = {
      .aspectMask = vk_cast_format_to_aspect_flags(texture.desc.format),
      .levelCount = texture.desc.mip_levels,
      .layerCount = 1};
    if (range.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
      const VkClearDepthStencilValue vk_clear_value = {
        clear_value.depth_stencil.depth, clear_value.depth_stencil.stencil};
      SOUL_ASSERT(0, !(range.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT), "");
      vkCmdClearDepthStencilImage(
        clear_command_buffer,
        texture.vk_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &vk_clear_value,
        1,
        &range);
    } else {

      VkClearColorValue vk_clear_value;
      memcpy(&vk_clear_value, &clear_value, sizeof(vk_clear_value));
      vkCmdClearColorImage(
        clear_command_buffer,
        texture.vk_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &vk_clear_value,
        1,
        &range);
    }
    texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    texture.cache_state.commit_wait_event_or_barrier(
      QueueType::GRAPHIC,
      {PipelineStage::TOP_OF_PIPE},
      {},
      {PipelineStage::TRANSFER},
      {AccessType::TRANSFER_WRITE},
      true);
    texture.cache_state.commit_access(
      QueueType::GRAPHIC, {PipelineStage::TRANSFER}, {AccessType::TRANSFER_WRITE});
  }

  auto GPUResourceInitializer::generate_mipmap(Texture& texture) -> void
  {
    VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = texture.vk_handle,
      .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
    };

    auto mip_width = cast<i32>(texture.desc.extent.x);
    auto mip_height = cast<i32>(texture.desc.extent.y);

    const auto command_buffer = get_mipmap_gen_command_buffer().get_vk_handle();

    for (u32 i = 1; i < texture.desc.mip_levels; i++) {
      barrier.subresourceRange.baseMipLevel = i - 1;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

      VkImageBlit blit = {
        .srcSubresource =
          {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = i - 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
          },
        .srcOffsets =
          {
            {0, 0, 0},
            {mip_width, mip_height, 1},
          },
        .dstSubresource =
          {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = i,
            .baseArrayLayer = 0,
            .layerCount = 1,
          },
        .dstOffsets =
          {
            {0, 0, 0},
            {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1},
          },
      };

      vkCmdBlitImage(
        command_buffer,
        texture.vk_handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        texture.vk_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        VK_FILTER_LINEAR);

      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

      if (mip_width > 1) {
        mip_width /= 2;
      }
      if (mip_height > 1) {
        mip_height /= 2;
      }
    }

    barrier.subresourceRange.baseMipLevel = texture.desc.mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
      command_buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

    texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texture.cache_state = {
      .queue_owner = QueueType::GRAPHIC,
      .unavailable_pipeline_stages = {PipelineStage::FRAGMENT_SHADER},
      .unavailable_accesses = {},
      .sync_stages = {PipelineStage::FRAGMENT_SHADER},
      .visible_access_matrix = VisibleAccessMatrix::init_fill(AccessFlags{AccessType::SHADER_READ}),
    };
  }

  auto GPUResourceInitializer::flush(CommandQueues& command_queues, System& /* gpu_system */)
    -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    for (auto& thread_context : thread_contexts_) {
      if (!thread_context.clear_command_buffer.is_null()) {
        command_queues[QueueType::GRAPHIC].submit(thread_context.clear_command_buffer);
      }
      if (
        !thread_context.transfer_command_buffer.is_null() &&
        thread_context.mipmap_gen_command_buffer.is_null()) {
        command_queues[QueueType::TRANSFER].submit(thread_context.transfer_command_buffer);
      } else if (
        thread_context.transfer_command_buffer.is_null() &&
        !thread_context.mipmap_gen_command_buffer.is_null()) {
        command_queues[QueueType::GRAPHIC].submit(thread_context.mipmap_gen_command_buffer);
      } else if (
        !thread_context.transfer_command_buffer.is_null() &&
        !thread_context.mipmap_gen_command_buffer.is_null()) {
        command_queues[QueueType::TRANSFER].submit(thread_context.transfer_command_buffer);
        command_queues[QueueType::GRAPHIC].wait(
          command_queues[QueueType::TRANSFER].get_timeline_semaphore(),
          VK_PIPELINE_STAGE_TRANSFER_BIT);
        command_queues[QueueType::GRAPHIC].submit(thread_context.mipmap_gen_command_buffer);
      }

      thread_context.clear_command_buffer = PrimaryCommandBuffer();
      thread_context.mipmap_gen_command_buffer = PrimaryCommandBuffer();
      thread_context.transfer_command_buffer = PrimaryCommandBuffer();
    }
  }

  auto GPUResourceInitializer::reset() -> void
  {
    SOUL_ASSERT_MAIN_THREAD();

    for (auto& thread_context : thread_contexts_) {
      for (const StagingBuffer& buffer : thread_context.staging_buffers) {
        vmaDestroyBuffer(gpu_allocator_, buffer.vk_handle, buffer.allocation);
      }
      thread_context.staging_buffers.resize(0);
    }
  }

  auto GPUResourceFinalizer::init() -> void
  {
    thread_contexts_.resize(runtime::get_thread_count());
  }

  auto GPUResourceFinalizer::finalize(Buffer& buffer) -> void
  {
    if (buffer.cache_state.queue_owner == QueueType::NONE) {
      return;
    }
    thread_contexts_[runtime::get_thread_id()].sync_dst_queues_[buffer.cache_state.queue_owner] |=
      buffer.desc.queue_flags;
    buffer.cache_state.queue_owner = QueueType::NONE;
    buffer.cache_state = ResourceCacheState();
  }

  auto GPUResourceFinalizer::finalize(Texture& texture, TextureUsageFlags usage_flags) -> void
  {
    if (texture.cache_state.queue_owner == QueueType::NONE) {
      return;
    }

    static auto get_finalize_layout = [](TextureUsageFlags usage) -> VkImageLayout {
      VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
      static constexpr auto USAGE_LAYOUT_MAP = FlagMap<TextureUsage, VkImageLayout>::from_val_list({
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      });
      static_assert(std::size(USAGE_LAYOUT_MAP) == to_underlying(TextureUsage::COUNT));

      usage.for_each([&result](TextureUsage texture_usage) {
        if (result != VK_IMAGE_LAYOUT_UNDEFINED && result != USAGE_LAYOUT_MAP[texture_usage]) {
          result = VK_IMAGE_LAYOUT_GENERAL;
        } else {
          result = USAGE_LAYOUT_MAP[texture_usage];
        }
      });
      SOUL_ASSERT(0, result != VK_IMAGE_LAYOUT_UNDEFINED, "");
      return result;
    };

    VkImageLayout finalize_layout = get_finalize_layout(usage_flags);
    if (texture.layout != finalize_layout) {
      // TODO(kevinyu): Check if use specific read access (for example use
      // VK_ACCESS_TRANSFER_SRC_BIT only for VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) is faster
      const VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = texture.layout,
        .newLayout = finalize_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture.vk_handle,
        .subresourceRange =
          {
            .aspectMask = vk_cast_format_to_aspect_flags(texture.desc.format),
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
          },
      };
      thread_contexts_[runtime::get_thread_id()]
        .image_barriers_[texture.cache_state.queue_owner]
        .push_back(barrier);

      texture.layout = finalize_layout;
    }

    thread_contexts_[runtime::get_thread_id()].sync_dst_queues_[texture.cache_state.queue_owner] |=
      texture.desc.queue_flags;
    texture.cache_state = ResourceCacheState();
  }

  auto GPUResourceFinalizer::flush(
    CommandPools& command_pools, CommandQueues& command_queues, System& /* gpu_system */) -> void
  {
    SOUL_ASSERT_MAIN_THREAD();
    runtime::ScopeAllocator scope_allocator("Resource Finalizer Flush");

    auto command_buffers =
      FlagMap<QueueType, PrimaryCommandBuffer>::init_fill(PrimaryCommandBuffer(VK_NULL_HANDLE));

    // create command buffer and create semaphore
    for (auto queue_type : FlagIter<QueueType>()) {
      QueueFlags sync_dst_queue = std::accumulate(
        thread_contexts_.begin(),
        thread_contexts_.end(),
        QueueFlags({}),
        [queue_type](const QueueFlags flag, const ThreadContext& thread_context) -> QueueFlags {
          return flag | thread_context.sync_dst_queues_[queue_type];
        });

      const auto image_barrier_capacity = std::accumulate(
        thread_contexts_.begin(),
        thread_contexts_.end(),
        usize(0),
        [queue_type](const usize capacity, const ThreadContext& thread_context) -> usize {
          return capacity + thread_context.image_barriers_[queue_type].size();
        });
      Vector<VkImageMemoryBarrier> image_barriers(&scope_allocator);
      image_barriers.reserve(image_barrier_capacity);
      for (const auto& thread_context : thread_contexts_) {
        image_barriers.append(thread_context.image_barriers_[queue_type]);
      }

      sync_dst_queue.for_each([&](const QueueType dst_queue_type) {
        if (dst_queue_type == queue_type) {
          command_buffers[queue_type] = command_pools.request_command_buffer(queue_type);
          const VkMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
          };

          vkCmdPipelineBarrier(
            command_buffers[queue_type].get_vk_handle(),
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            1,
            &barrier,
            0,
            nullptr,
            soul::cast<u32>(image_barriers.size()),
            image_barriers.data());
        } else {
          command_queues[dst_queue_type].wait(
            command_queues[queue_type].get_timeline_semaphore(),
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }
      });
    }

    // submit command buffer
    for (const auto queue_type : FlagIter<QueueType>()) {
      if (const auto command_buffer = command_buffers[queue_type]; !command_buffer.is_null()) {
        command_queues[queue_type].submit(command_buffer);
      }
    }

    for (auto& context : thread_contexts_) {
      for (auto& image_barrier_list : context.image_barriers_) {
        image_barrier_list.resize(0);
      }
      context.sync_dst_queues_ = FlagMap<QueueType, QueueFlags>::init_fill(QueueFlags());
    }
  }

  RTInstanceDesc::RTInstanceDesc(
    const mat4f in_transform,
    const u32 instance_id,
    const u32 instance_mask,
    const u32 sbt_offset,
    const RTGeometryInstanceFlags flags,
    const GPUAddress blas_gpu_address)
      : instance_id(instance_id),
        instance_mask(instance_mask),
        sbt_offset(sbt_offset),
        flags(flags.to_uint32()),
        blas_gpu_address(blas_gpu_address)
  {
    const auto vk_transform = math::transpose(in_transform);
    std::memcpy(transform, &vk_transform, sizeof(transform));
  }
} // namespace soul::gpu
// NOLINTEND(readability-implicit-bool-conversion)
// NOLINTEND(cppcoreguidelines-init-variables)
// NOLINTEND(hicpp-signed-bitwise)
