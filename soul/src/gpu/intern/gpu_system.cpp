#include "gpu/system.h"
#include "gpu/render_graph.h"
#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_graph_execution.h"
#include "gpu/intern/bindless_descriptor_allocator.h"

#include "runtime/system.h"

#include "runtime/scope_allocator.h"

#include "core/array.h"
#include "core/dev_util.h"
#include "core/util.h"

#include <volk.h>

#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

#include <cstddef>
#include <limits>
#include <string>
#include <sstream>
#include <random>
#include <variant>
#include <span>
#include <slang.h>
#include <slang-com-ptr.h>

#include "memory/allocator.h"

static const char* RESOURCE_SLANG = R"SLANG(
[[vk::binding(0, 0)]] ByteAddressBuffer global_buffers[];
[[vk::binding(0, 1)]] SamplerState global_samplers[];
[[vk::binding(0, 2)]] Texture2D global_2d_textures[];
[[vk::binding(0, 2)]] TextureCube global_cube_textures[];

__glsl_extension(GL_EXT_nonuniform_qualifier)
T get_buffer<T>(uint descriptor_id) {
	return global_buffers[descriptor_id].Load<T>(0);
}

SamplerState get_sampler(uint descriptor_id) {
	return global_samplers[descriptor_id];
}

Texture2D get_texture_2d(uint descriptor_id) {
	return global_2d_textures[descriptor_id];
}
)SLANG";

namespace soul::gpu
{
	using namespace impl;

	slang::IGlobalSession* create_slang_global_session()
	{
		slang::IGlobalSession* result = nullptr;
		slang::createGlobalSession(&result);
		return result;
	}

	slang::IGlobalSession* get_slang_global_session()
	{
		static slang::IGlobalSession* slang_global_session = create_slang_global_session();
		return slang_global_session;
	}

	static void* vma_alloc_callback(void* user_data, const size_t size, const size_t alignment, VkSystemAllocationScope allocation_scope)
	{
		const auto allocator = static_cast<memory::Allocator*>(user_data);
		return allocator->allocate(size, alignment);
	}

	static auto vma_reallocation_callback(void* user_data, void* addr, const size_t size, const size_t alignment,
	                                    VkSystemAllocationScope scope) -> void*
	{
		const auto allocator = static_cast<memory::Allocator*>(user_data);

		if (addr != nullptr)
		{
			allocator->deallocate(addr, 0);
		}
		return allocator->allocate(size, alignment);
	}

	static void vma_free_callback(void* user_data, void* addr)
	{
		if (addr == nullptr) return;
		const auto allocator = static_cast<memory::Allocator*>(user_data);
		allocator->deallocate(addr, 0);
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data) {
		switch (message_severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			{
				SOUL_LOG_INFO("VkDebugUtils: %s", callback_data->pMessage);
				break;
			}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
			SOUL_LOG_WARN("VkDebugUtils: %s", callback_data->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			SOUL_LOG_ERROR("VkDebugUtils: %s", callback_data->pMessage);
			SOUL_PANIC(0, "Vulkan Error!");
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
		default: {
			SOUL_NOT_IMPLEMENTED();
			break;
		}
		}
		return VK_FALSE;
	}

	void System::init(const Config &config) {
		SOUL_ASSERT_MAIN_THREAD();
		runtime::ScopeAllocator<> scope_allocator("GPU::System::Init");

		_db.currentFrame = 0;
		_db.frameCounter = 0;
		
		_db.programs.add({});
		_db.semaphores.reserve(1000);
		_db.semaphores.add({});
		_db.samplerMap.reserve(128);

		SOUL_ASSERT(0, config.windowHandle != nullptr, "Invalid configuration value | windowHandle = nullptr");
		SOUL_ASSERT(0, config.threadCount > 0, "Invalid configuration value | threadCount = %d",
		            config.threadCount);
		SOUL_ASSERT(0,
		            config.maxFrameInFlight > 0,
		            "Invalid configuration value | maxFrameInFlight = %d",
		            config.maxFrameInFlight);

		SOUL_VK_CHECK(volkInitialize(), "Volk initialization fail!");
		SOUL_LOG_INFO("Volk initialization sucessful");

		auto create_instance = []()->VkInstance
		{
			const VkApplicationInfo app_info = {
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Soul",
				.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
				.pEngineName = "Soul",
				.engineVersion = VK_MAKE_VERSION(0, 0, 1),
				.apiVersion = VK_API_VERSION_1_3
			};

			static constexpr const char* REQUIRED_EXTENSIONS[] = {
				VK_KHR_SURFACE_EXTENSION_NAME,
	#if defined(SOUL_OS_WINDOWS)
				"VK_KHR_win32_surface",
	#endif // SOUL_PLATFORM_OS_WIN32
	#ifdef SOUL_OS_APPLE
					"VK_MVK_macos_surface",
	#endif // SOUL_PLATFORM_OS_APPLE
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME
			};

	#ifdef SOUL_VULKAN_VALIDATION_ENABLE
			static constexpr const char* REQUIRED_LAYERS[] = {
				"VK_LAYER_KHRONOS_validation",
			};
			auto is_required_layers_supported = []() -> bool {
				SOUL_LOG_INFO("Check vulkan layer support.");
				uint32 layer_count;

				vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

				Array<VkLayerProperties> available_layers;
				available_layers.resize(layer_count);
				vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
				for (const char* required_layer : REQUIRED_LAYERS) {
					bool layer_found = false;
					for (const auto& layer : available_layers)
					{
						if (strcmp(required_layer, layer.layerName) == 0) {
							layer_found = true;
							break;
						}
					}
					if (!layer_found) {
						SOUL_LOG_INFO("Validation layer %s not found!", required_layer);
						return false;
					}
				}
				available_layers.cleanup();

				return true;
			};

			SOUL_ASSERT(0, is_required_layers_supported(), "");

			constexpr uint32 required_layers_count = std::size(REQUIRED_LAYERS);
	#else
			constexpr uint32 required_layers_count = 0;
			static constexpr const char* const* REQUIRED_LAYERS = nullptr;
	#endif

			const VkInstanceCreateInfo instance_create_info = {
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &app_info,
				.enabledLayerCount = required_layers_count,
				.ppEnabledLayerNames = REQUIRED_LAYERS,
				.enabledExtensionCount = std::size(REQUIRED_EXTENSIONS),
				.ppEnabledExtensionNames = REQUIRED_EXTENSIONS
			};
			VkInstance instance;
			SOUL_VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &instance),
				"Vulkan instance creation fail!");
			SOUL_LOG_INFO("Vulkan instance creation succesful");
			volkLoadInstance(instance);
			SOUL_LOG_INFO("Instance version = %d, %d", VK_VERSION_MAJOR(volkGetInstanceVersion()), VK_VERSION_MINOR(volkGetInstanceVersion()));
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
				.pUserData = nullptr
			};
			SOUL_VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &debug_messenger_create_info, nullptr, &debug_messenger), "Vulkan debug messenger creation fail!");
			SOUL_LOG_INFO("Vulkan debug messenger creation sucesfull");
			return debug_messenger;
		};
		_db.debugMessenger = create_debug_utils_messenger(_db.instance);

		create_surface(config.windowHandle, &_db.surface);
		auto pick_physical_device = [](Database *db, int required_extension_count,
			                        const char *required_extensions[])-> EnumArray<QueueType, uint32> {
				SOUL_LOG_INFO("Picking vulkan physical device.");
				db->physicalDevice = VK_NULL_HANDLE;
				uint32 device_count = 0;
				vkEnumeratePhysicalDevices(db->instance, &device_count, nullptr);
				SOUL_ASSERT(0, device_count > 0, "There is no device with vulkan support!");

				soul::Array<VkPhysicalDevice> devices;
				devices.resize(device_count);
				vkEnumeratePhysicalDevices(db->instance, &device_count, devices.data());

				Array<VkSurfaceFormatKHR> formats;
				Array<VkPresentModeKHR> present_modes;

				db->physicalDevice = VK_NULL_HANDLE;
				int best_score = -1;

				EnumArray<QueueType, uint32> queue_family_indices;

				for (VkPhysicalDevice device : devices) {

					VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = {
						.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
						.pNext = nullptr
					};
					VkPhysicalDeviceFeatures2 device_features{
						.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
						.pNext = &indexing_features
					};
					vkGetPhysicalDeviceFeatures2(device, &device_features);
					const bool bindless_supported = indexing_features.descriptorBindingPartiallyBound && indexing_features.runtimeDescriptorArray;
					if (!bindless_supported)
						continue;

					VkPhysicalDeviceProperties physical_device_properties;
					VkPhysicalDeviceFeatures physical_device_features;
					vkGetPhysicalDeviceProperties(device, &physical_device_properties);
					vkGetPhysicalDeviceFeatures(device, &physical_device_features);
					
					const uint32_t api_version = physical_device_properties.apiVersion;
					const uint32_t driver_version = physical_device_properties.driverVersion;
					const uint32_t vendor_id = physical_device_properties.vendorID;
					const uint32_t device_id = physical_device_properties.deviceID;

					SOUL_LOG_INFO("Devices\n"
					              " -- Name = %s\n"
					              " -- Vendor = 0x%.8X\n"
					              " -- Device ID = 0x%.8X\n"
					              " -- Api Version = %d.%d.%d\n"
					              " -- Driver Version = %d.%d.%d\n",
					              physical_device_properties.deviceName, vendor_id, 
					              device_id, 
					              VK_VERSION_MAJOR(api_version), VK_VERSION_MINOR(api_version), VK_VERSION_PATCH(api_version),
					              VK_VERSION_MAJOR(driver_version), VK_VERSION_MINOR(driver_version), VK_VERSION_PATCH(driver_version));

					uint32_t extension_count;
					vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

					soul::Array<VkExtensionProperties> available_extensions;
					available_extensions.resize(extension_count);
					vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
					
					auto is_extension_available = [&available_extensions](const char* required_extension) -> bool
					{
						return std::ranges::any_of(available_extensions, [required_extension](const auto& properties)
						{
							return strcmp(properties.extensionName, required_extension);
						});
					};
					const bool all_extension_found = std::any_of(required_extensions, required_extensions + required_extension_count, is_extension_available);
					if (!all_extension_found) {
						continue;
					}

					vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, db->surface, &db->surfaceCaps);

					uint32 format_count;
					vkGetPhysicalDeviceSurfaceFormatsKHR(device, db->surface, &format_count, nullptr);
					SOUL_LOG_INFO(" -- Format count = %d", format_count);
					if (format_count == 0) {
						continue;
					}
					formats.resize(format_count);
					vkGetPhysicalDeviceSurfaceFormatsKHR(device, db->surface, &format_count, formats.data());

					uint32 present_mode_count;
					vkGetPhysicalDeviceSurfacePresentModesKHR(device, db->surface, &present_mode_count, nullptr);
					SOUL_LOG_INFO(" -- Present mode count = %d", present_mode_count);
					if (present_mode_count == 0) {
						continue;
					}
					present_modes.resize(present_mode_count);
					vkGetPhysicalDeviceSurfacePresentModesKHR(device, db->surface, &present_mode_count,
					                                          present_modes.data());

					uint32 queue_family_count;
					vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

					soul::Array<VkQueueFamilyProperties> queue_families;
					queue_families.resize(queue_family_count);
					vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

					const auto not_found_family_index = soul::cast<uint32>(queue_families.size());

					uint32 graphics_queue_family_index = not_found_family_index;
					// Try to find queue that support present, graphics and compute
					for (uint32 j = 0; j < queue_family_count; j++) {
						VkBool32 present_support = false;
						vkGetPhysicalDeviceSurfaceSupportKHR(device, j, db->surface, &present_support);

						const VkQueueFamilyProperties &queueProps = queue_families[j];
						if (VkQueueFlags required_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT; present_support && queueProps.queueCount > 0 &&
							(queueProps.queueFlags & required_flags) == required_flags) {
							graphics_queue_family_index = j;
						}
					}

					uint32 compute_queue_family_index = not_found_family_index;
					for (uint32 j = 0; j < queue_family_count; j++) {
						const VkQueueFamilyProperties &queue_props = queue_families[j];
						VkQueueFlags required_flags = VK_QUEUE_COMPUTE_BIT;
						if (j != graphics_queue_family_index && queue_props.queueCount > 0
							&& (queue_props.queueFlags & required_flags) == required_flags) {
							compute_queue_family_index = j;
							break;
						}
					}

					uint32 transfer_queue_family_index = not_found_family_index;
					for (uint32 j = 0; j < queue_family_count; j++) {
						const VkQueueFamilyProperties &queue_props = queue_families[j];
						VkQueueFlags required_flags = VK_QUEUE_TRANSFER_BIT;
						if (j != graphics_queue_family_index && j != compute_queue_family_index
							&& (queue_props.queueFlags & required_flags) == required_flags) {
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

					int score = 0;
					if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
						score += 100;
					}

					SOUL_LOG_INFO("-- Score = %d", score);
					if (score > best_score) {
						db->physicalDevice = device;
						queue_family_indices[QueueType::GRAPHIC] = graphics_queue_family_index;
						queue_family_indices[QueueType::TRANSFER] = transfer_queue_family_index;
						queue_family_indices[QueueType::COMPUTE] = compute_queue_family_index;
						best_score = score;
					}
				}

				SOUL_ASSERT(0, db->physicalDevice != VK_NULL_HANDLE,
				            "Cannot find physical device that satisfy the requirements.");

				vkGetPhysicalDeviceProperties(db->physicalDevice, &db->physicalDeviceProperties);
				vkGetPhysicalDeviceFeatures(db->physicalDevice, &db->physicalDeviceFeatures);
			
				const uint32_t api_version = db->physicalDeviceProperties.apiVersion;
				const uint32_t driver_version = db->physicalDeviceProperties.driverVersion;
				const uint32_t vendor_id = db->physicalDeviceProperties.vendorID;
				const uint32_t device_id = db->physicalDeviceProperties.deviceID;

				SOUL_LOG_INFO("Selected device\n"
				              " -- Name = %s\n"
				              " -- Vendor = 0x%.8X\n"
				              " -- Device ID = 0x%.8X\n"
				              " -- Api Version = 0x%.8X\n"
				              " -- Driver Version = 0x%.8X\n"
				              " -- Graphics queue family index = %d\n"
				              " -- Transfer queue family index = %d\n"
				              " -- Compute queue family index = %d\n",
				              db->physicalDeviceProperties.deviceName,
				              vendor_id,
				              device_id,
				              api_version,
				              driver_version,
				              queue_family_indices[QueueType::GRAPHIC],
				              queue_family_indices[QueueType::TRANSFER],
				              queue_family_indices[QueueType::COMPUTE]);

				SOUL_ASSERT(0, db->physicalDevice != VK_NULL_HANDLE, "Fail to find a suitable GPU!");
				return queue_family_indices;
			};
		static const char* device_required_extensions[] = {
			VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME,
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		static constexpr uint32_t DEVICE_REQUIRED_EXTENSION_COUNT = std::size(device_required_extensions);
		EnumArray<QueueType, uint32> queue_family_indices = pick_physical_device(&_db, DEVICE_REQUIRED_EXTENSION_COUNT, device_required_extensions);

		auto create_device_and_queue = 
			[](VkPhysicalDevice physicalDevice, EnumArray<QueueType, uint32>& queue_family_indices)
				-> std::tuple<VkDevice, EnumArray<QueueType, uint32>> {
			SOUL_LOG_INFO("Creating vulkan logical device");

			int graphicsQueueCount = 1;
			EnumArray<QueueType, uint32> queueIndex(0);

			if (queue_family_indices[QueueType::COMPUTE] == queue_family_indices[QueueType::GRAPHIC]) {
				graphicsQueueCount++;
				queueIndex[QueueType::COMPUTE] = queueIndex[QueueType::GRAPHIC] + 1;
			}
			if (queue_family_indices[QueueType::TRANSFER] == queue_family_indices[QueueType::GRAPHIC]) {
				graphicsQueueCount++;
				queueIndex[QueueType::TRANSFER] = queueIndex[QueueType::COMPUTE] + 1;
			}

			VkDeviceQueueCreateInfo queue_create_info[4] = {};
			float priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};
			uint32 queue_create_info_count = 1;

			queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info[0].queueFamilyIndex = queue_family_indices[QueueType::GRAPHIC];
			queue_create_info[0].queueCount = graphicsQueueCount;
			queue_create_info[0].pQueuePriorities = priorities;

			if (queue_family_indices[QueueType::GRAPHIC] != queue_family_indices[QueueType::COMPUTE]) {
				queue_create_info[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_info[queue_create_info_count].queueFamilyIndex = queue_family_indices[QueueType::COMPUTE];
				queue_create_info[queue_create_info_count].queueCount = 1;
				queue_create_info[queue_create_info_count].pQueuePriorities = priorities + queue_create_info_count;
				queue_create_info_count++;
			}

			if (queue_family_indices[QueueType::TRANSFER] != queue_family_indices[QueueType::GRAPHIC]) {
				queue_create_info[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_info[queue_create_info_count].queueFamilyIndex = queue_family_indices[QueueType::TRANSFER];
				queue_create_info[queue_create_info_count].queueCount = 1;
				queue_create_info[queue_create_info_count].pQueuePriorities = priorities + queue_create_info_count;
				queue_create_info_count++;
			}

			// TODO(kevinyu): This is temporary hack to avoid molten vk bug when queue family is not sorted in queueCreateInfo
			queue_create_info[3] = queue_create_info[0];
			queue_create_info[0] = queue_create_info[1];
			queue_create_info[1] = queue_create_info[2];
			queue_create_info[2] = queue_create_info[3];

			VkPhysicalDeviceFeatures device_features = {
				.geometryShader = VK_TRUE,
				.fillModeNonSolid = VK_TRUE,
				.fragmentStoresAndAtomics = VK_TRUE
			};
			VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
				.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
				.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
				.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
				.descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
				.descriptorBindingPartiallyBound = VK_TRUE,
				.descriptorBindingVariableDescriptorCount = VK_TRUE,
				.runtimeDescriptorArray = VK_TRUE
			};
			VkPhysicalDeviceVulkanMemoryModelFeatures memory_model_features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES,
				.pNext = &indexing_features,
				.vulkanMemoryModel = VK_TRUE
			};
			const VkPhysicalDeviceFeatures2 device_features2 = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
				.pNext = &memory_model_features,
				.features = device_features
			};

			const VkDeviceCreateInfo device_create_info = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = &device_features2,
				.queueCreateInfoCount = queue_create_info_count,
				.pQueueCreateInfos = queue_create_info,
				.enabledExtensionCount = DEVICE_REQUIRED_EXTENSION_COUNT,
				.ppEnabledExtensionNames = device_required_extensions,
				.pEnabledFeatures = nullptr
			};

			VkDevice device;
			SOUL_VK_CHECK(vkCreateDevice(physicalDevice, &device_create_info, nullptr, &device),
			              "Vulkan logical device creation fail!");
			SOUL_LOG_INFO("Vulkan logical device creation sucessful");
			return std::make_tuple(device, queueIndex);
		};

		auto [device, queueIndex] = create_device_and_queue(_db.physicalDevice, queue_family_indices);
		_db.device = device;
		for (const auto queue_type : EnumIter<QueueType>::Iterates())
		{
			_db.queues[queue_type].init(device, queue_family_indices[queue_type], queueIndex[queue_type]);
		}

		auto create_swapchain = [this, &queue_family_indices](Database *db, const uint32 swapchain_width, const uint32 swapchain_height) {

			auto pick_surface_format = [](VkPhysicalDevice physicalDevice,
				                       VkSurfaceKHR surface) -> VkSurfaceFormatKHR {
					runtime::ScopeAllocator<> scope_allocator("GPU::System::init::pickSurfaceFormat");
					SOUL_LOG_INFO("Picking surface format.");
					Array<VkSurfaceFormatKHR> formats(&scope_allocator);
					uint32 format_count;
					vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, nullptr);
					SOUL_ASSERT(0, format_count != 0, "Surface format count is zero!");
					formats.resize(format_count);
					vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, formats.data());
					for (const VkSurfaceFormatKHR surface_format : formats) {
						if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM
							&& surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
							return surface_format;
						}
					}
					VkSurfaceFormatKHR format = formats[0];
					return format;
				};
			db->swapchain.format = pick_surface_format(db->physicalDevice, db->surface);

			auto pick_surface_extent = [](const VkSurfaceCapabilitiesKHR capabilities, const uint32 swapchain_width,
				   const uint32 swapchain_height) -> VkExtent2D {
				SOUL_LOG_INFO("Picking vulkan swap extent");
				if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
					SOUL_LOG_INFO("vulkanPickSwapExtent | Extent = %d %d",
					              capabilities.currentExtent.width,
					              capabilities.currentExtent.height);
					return capabilities.currentExtent;
				}
				else {
					VkExtent2D actual_extent = {static_cast<uint32>(swapchain_width),
						static_cast<uint32>(swapchain_height)};
					actual_extent.width =
						std::max(capabilities.minImageExtent.width,
						         std::min(capabilities.maxImageExtent.width, actual_extent.width));
					actual_extent.height =
						std::max(capabilities.minImageExtent.height,
						         std::min(capabilities.maxImageExtent.height, actual_extent.height));
					return actual_extent;
				}
			};
			db->swapchain.extent = pick_surface_extent(db->surfaceCaps, swapchain_width, swapchain_height);

			uint32 image_count = db->surfaceCaps.minImageCount + 1;
			if (db->surfaceCaps.maxImageCount > 0 && image_count > db->surfaceCaps.maxImageCount) {
				image_count = db->surfaceCaps.maxImageCount;
			}
			SOUL_LOG_INFO("Swapchain image count = %d", image_count);

			const VkSwapchainCreateInfoKHR swapchain_info = {
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = db->surface,
				.minImageCount = image_count,
				.imageFormat = db->swapchain.format.format,
				.imageColorSpace = db->swapchain.format.colorSpace,
				.imageExtent = db->swapchain.extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = &queue_family_indices[QueueType::GRAPHIC],
				.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
				.clipped = VK_TRUE,
			};
			SOUL_VK_CHECK(vkCreateSwapchainKHR(db->device, &swapchain_info, nullptr, &db->swapchain.vkHandle),
			              "Fail to create vulkan swapchain!");
			vkGetSwapchainImagesKHR(db->device, db->swapchain.vkHandle, &image_count, nullptr);
			db->swapchain.images.resize(image_count);
			vkGetSwapchainImagesKHR(db->device, db->swapchain.vkHandle, &image_count, db->swapchain.images.data());

			db->swapchain.imageViews.reserve(image_count);
			std::ranges::transform(db->swapchain.images, std::back_inserter(db->swapchain.imageViews), [format = db->swapchain.format.format, device = db->device](VkImage image)
			{
				VkImageView image_view;
				const VkImageViewCreateInfo image_view_info = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = image,
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = format,
					.components = {
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY
					},
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				};
				SOUL_VK_CHECK(vkCreateImageView(device, &image_view_info, nullptr, &image_view),
					"Fail to create swapchain imageview");
				return image_view;
			});

			db->swapchain.textures.reserve(image_count);
			std::ranges::transform(db->swapchain.images, db->swapchain.imageViews, std::back_inserter(db->swapchain.textures), 
				[db, this, image_sharing_mode = swapchain_info.imageSharingMode](VkImage image, VkImageView image_view)
			{
				const auto texture_id = TextureID(db->texturePool.create());
				Texture& texture = *get_texture_ptr(texture_id);
				texture.desc = TextureDesc::D2("Swapchain Texture", TextureFormat::BGRA8, 1, {}, {}, Vec2ui32(db->swapchain.extent.width, db->swapchain.extent.height));
				texture.vkHandle = image;
				texture.view.vkHandle = image_view;
				texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
				texture.sharingMode = image_sharing_mode;
				texture.owner = ResourceOwner::NONE;
				return texture_id;
			});

			db->swapchain.fences.resize(image_count);
			std::ranges::fill(db->swapchain.fences,VK_NULL_HANDLE);
			SOUL_LOG_INFO("Vulkan swapchain creation sucessful");
		};
		create_swapchain(&_db, config.swapchainWidth, config.swapchainHeight);

		auto init_allocator = [](Database *db) {
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
				.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR
			};

			const VkAllocationCallbacks allocation_callbacks = {
				&(db->vulkanCPUAllocator),
				vma_alloc_callback,
				vma_reallocation_callback,
				vma_free_callback,
				nullptr,
				nullptr
			};

			const VmaAllocatorCreateInfo allocator_info = {
				.physicalDevice = db->physicalDevice,
				.device = db->device,
				.preferredLargeHeapBlockSize = 0,
				.pAllocationCallbacks = &allocation_callbacks,
				.pDeviceMemoryCallbacks = nullptr,
				.pVulkanFunctions = &vulkan_functions,
				.instance = db->instance
			};

			vmaCreateAllocator(&allocator_info, &db->gpuAllocator);

			SOUL_LOG_INFO("Vulkan init allocator sucessful");
		};
		init_allocator(&_db);
		_db.descriptorAllocator.init(_db.device);
		init_frame_context(config);
		_db.slang_global_session = get_slang_global_session();
		_frameBegin();
	}

	void System::init_frame_context(const System::Config &config) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_LOG_INFO("Frame Context Init");
		_db.frameContexts.reserve(config.maxFrameInFlight);
		std::fill_n(std::back_inserter(_db.frameContexts), _db.frameContexts.capacity(), _FrameContext(&_db.cpuAllocator));
		std::ranges::for_each(_db.frameContexts, [this, config](_FrameContext& frame_context)
		{
			frame_context.commandPools.init(_db.device, _db.queues, config.threadCount);
			frame_context.gpuResourceInitializer.init(_db.gpuAllocator, &frame_context.commandPools);
			frame_context.gpuResourceFinalizer.init();

			const VkFenceCreateInfo fence_info = {
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT
			};
			SOUL_VK_CHECK(vkCreateFence(_db.device, &fence_info, nullptr, &frame_context.fence), "Fail to create Fence");

			frame_context.imageAvailableSemaphore = create_semaphore();
			frame_context.renderFinishedSemaphore = create_semaphore();
		});
	}

	QueueData System::get_queue_data_from_queue_flags(QueueFlags flags) const {
		QueueData queue_data;
		const auto& queues = _db.queues;
		flags.forEach([&queue_data, queues](QueueType type)
		{
			queue_data.indices[queue_data.count++] = queues[type].getFamilyIndex();
		});
		return queue_data;
	}

	TextureID System::create_texture(const TextureDesc &desc) {
		SOUL_PROFILE_ZONE();

		SOUL_ASSERT(0, desc.layerCount >= 1, "");
		const auto texture_id = TextureID(_db.texturePool.create());
		Texture& texture = *_db.texturePool.get(texture_id.id);
		const VkFormat format = vkCast(desc.format);
		const QueueData queue_data = get_queue_data_from_queue_flags(desc.queueFlags);
		const VkImageCreateFlags image_create_flags = desc.type == TextureType::CUBE ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		const VkImageCreateInfo image_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = image_create_flags,
			.imageType = vkCast(desc.type),
			.format = format,
			.extent = get_vk_extent_3d(desc.extent),
			.mipLevels = desc.mipLevels,
			.arrayLayers = desc.layerCount,
			.samples = vkCast(desc.sampleCount),
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = vkCast(desc.usageFlags),
			.sharingMode = queue_data.count == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = queue_data.count,
			.pQueueFamilyIndices = queue_data.indices,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		const VmaAllocationCreateInfo alloc_info = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};

		SOUL_VK_CHECK(vmaCreateImage(_db.gpuAllocator,
			              &image_info, &alloc_info, &texture.vkHandle,
			              &texture.allocation, nullptr), "Fail to create image");

		VkImageAspectFlags image_aspect = vkCastFormatToAspectFlags(desc.format);
		if (image_aspect & VK_IMAGE_ASPECT_STENCIL_BIT)
		{
			SOUL_LOG_WARN("Texture creation with stencil format detected. Current version will remove the aspect stencil bit so the texture cannot be used for depth stencil. The reason is because Vulkan spec stated that descriptor cannot have more than one aspect.");
			image_aspect &= ~(VK_IMAGE_ASPECT_STENCIL_BIT);
		}
		
		const VkImageViewCreateInfo image_view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = texture.vkHandle,
			.viewType = vkCastToImageViewType(desc.type),
			.format = format,
			.components = {},
			.subresourceRange = {
				image_aspect,
				0,
				VK_REMAINING_MIP_LEVELS,
				0,
				VK_REMAINING_ARRAY_LAYERS
			}
		};

		SOUL_VK_CHECK(vkCreateImageView(_db.device,
			              &image_view_info, nullptr, &texture.view.vkHandle), "Fail to create image view");
		if (desc.usageFlags.test(TextureUsage::SAMPLED))
		{
			texture.view.sampledImageGPUHandle = _db.descriptorAllocator.create_sampled_image_descriptor(texture.view.vkHandle);
		}
		if (desc.usageFlags.test(TextureUsage::STORAGE))
		{
			texture.view.storageImageGPUHandle = _db.descriptorAllocator.create_storage_image_descriptor(texture.view.vkHandle);
		}

		texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		texture.sharingMode = image_info.sharingMode;
		texture.owner = ResourceOwner::NONE;
		texture.views = nullptr;
		texture.desc = desc;

		if (desc.name != nullptr) {
			char tex_name[1024];
			sprintf(tex_name, "%s(%d)", desc.name, _db.frameCounter);
			VkDebugUtilsObjectNameInfoEXT image_name_info = {
				VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, // sType
				nullptr,                                               // pNext
				VK_OBJECT_TYPE_IMAGE,                               // objectType
				reinterpret_cast<uint64>(texture.vkHandle),                         // object
				tex_name,                            // pObjectName
			};

			vkSetDebugUtilsObjectNameEXT(_db.device, &image_name_info);
		}
		return texture_id;
	}

	TextureID System::create_texture(const TextureDesc &desc, const TextureLoadDesc& load_desc) {
		SOUL_ASSERT(0, load_desc.data != nullptr, "");
		SOUL_ASSERT(0, load_desc.dataSize != 0, "");
		SOUL_ASSERT(0, load_desc.regionLoads != nullptr, "");
		SOUL_ASSERT(0, load_desc.regionLoadCount != 0, "");

		TextureDesc new_desc = desc;
		new_desc.usageFlags |= { TextureUsage::TRANSFER_DST };
		new_desc.queueFlags |= { QueueType::TRANSFER };
		if (load_desc.generateMipmap) {
			new_desc.usageFlags |= { TextureUsage::TRANSFER_SRC };
		}
	
		TextureID texture_id = create_texture(new_desc);
		Texture &texture = *get_texture_ptr(texture_id);

		get_frame_context().gpuResourceInitializer.load(texture, load_desc);

		if (load_desc.generateMipmap && desc.mipLevels > 1) {
			get_frame_context().gpuResourceInitializer.generate_mipmap(texture);
		}

		return texture_id;
	}

	TextureID System::create_texture(const TextureDesc& desc, const ClearValue clear_value) {
		TextureDesc new_desc = desc;
		new_desc.usageFlags |= { TextureUsage::TRANSFER_DST };
		new_desc.queueFlags |= { QueueType::GRAPHIC };

		TextureID texture_id = create_texture(new_desc);
		Texture &texture = *get_texture_ptr(texture_id);

		get_frame_context().gpuResourceInitializer.clear(texture, clear_value);

		return texture_id;
	}

	void System::finalize_texture(const TextureID texture_id, const TextureUsageFlags usage_flags)
	{
		get_frame_context().gpuResourceFinalizer.finalize(*get_texture_ptr(texture_id), usage_flags);
	}

	uint32 System::get_texture_mip_levels(const TextureID texture_id) const
	{
		return get_texture(texture_id).desc.mipLevels;
	}

	const TextureDesc& System::get_texture_desc(const TextureID texture_id) const
	{
		return get_texture(texture_id).desc;
	}

	void System::destroy_texture_descriptor(TextureID texture_id)
	{
		auto destroy_texture_view_descriptor = [this](TextureView& texture_view)
		{
			_db.descriptorAllocator.destroy_sampled_image_descriptor(texture_view.sampledImageGPUHandle);
			_db.descriptorAllocator.destroy_storage_image_descriptor(texture_view.storageImageGPUHandle);
		};
		Texture& texture = *get_texture_ptr(texture_id);
		destroy_texture_view_descriptor(texture.view);
		uint32 view_count = texture.desc.mipLevels * texture.desc.layerCount;
		if (texture.views != nullptr) std::for_each_n(texture.views, view_count, destroy_texture_view_descriptor);
	}

	void System::destroy_texture(TextureID id) {
		SOUL_ASSERT_MAIN_THREAD();
		get_frame_context().garbages.textures.add(id);
	}

	Texture *System:: get_texture_ptr(const TextureID texture_id) {
		return _db.texturePool.get(texture_id.id);
	}

	const Texture& System::get_texture(const TextureID texture_id) const {
		const Texture* texture = _db.texturePool.get(texture_id.id);
		return *texture;
	}

	TextureView System::get_texture_view(const TextureID texture_id, const uint32 level,
	                                                        const uint32 layer) {
		Texture& texture = *get_texture_ptr(texture_id);
		SOUL_ASSERT(0, level < texture.desc.mipLevels, "");

		const uint32 layer_count = texture.desc.type == TextureType::D2_ARRAY ? texture.desc.extent.z : 1;

		if (texture.desc.mipLevels == 1) return texture.view;
		if (texture.views == nullptr) {
			texture.views = _db.cpuAllocator.create_raw_array<TextureView>(soul::cast<uint64>(layer_count) * texture.desc.mipLevels);
			std::fill_n(texture.views, texture.desc.mipLevels, TextureView());
		}
		const soul_size view_idx = layer * texture.desc.mipLevels + level;
		if (texture.views[view_idx].vkHandle == VK_NULL_HANDLE) {
			TextureView& texture_view = texture.views[level];
			const VkImageViewCreateInfo image_view_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = texture.vkHandle,
				.viewType = vkCastToImageViewType(texture.desc.type),
				.format = vkCast(texture.desc.format),
				.components = {},
				.subresourceRange = {
					vkCastFormatToAspectFlags(texture.desc.format),
					level,
					1,
					layer,
					1
				}
			};
			SOUL_VK_CHECK(vkCreateImageView(_db.device,
				              &image_view_info, nullptr, &texture_view.vkHandle), "Fail to create image view");
			if (texture.desc.usageFlags.test(TextureUsage::SAMPLED))
			{
				texture_view.sampledImageGPUHandle = _db.descriptorAllocator.create_sampled_image_descriptor(texture_view.vkHandle);
			}
			if (texture.desc.usageFlags.test(TextureUsage::STORAGE))
			{
				texture_view.storageImageGPUHandle = _db.descriptorAllocator.create_storage_image_descriptor(texture_view.vkHandle);
			}
		}
		return texture.views[level];
	}

	TextureView System::get_texture_view(const TextureID texture_id, const SubresourceIndex subresource_index)
	{
		return get_texture_view(texture_id, subresource_index.get_level(), subresource_index.get_layer());
	}

	TextureView System::get_texture_view(const TextureID texture_id, const std::optional<SubresourceIndex> subresource_index)
	{
		if (subresource_index)
		{
			return get_texture_view(texture_id, *subresource_index);
		}
		return get_texture(texture_id).view;
	}

	BufferID System::create_buffer(const BufferDesc &desc) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, desc.count > 0, "");
		SOUL_ASSERT(0, desc.typeSize > 0, "");
		SOUL_ASSERT(0, desc.typeAlignment > 0, "");
		SOUL_ASSERT(0, desc.usageFlags.any(), "");

		const auto buffer_id = BufferID(_db.bufferPool.create());
		Buffer &buffer = *get_buffer_ptr(buffer_id);

		const QueueData queue_data = get_queue_data_from_queue_flags(desc.queueFlags);
		SOUL_ASSERT(0, queue_data.count > 0, "");

		uint64 alignment = desc.typeSize;
		if (desc.usageFlags.test(BufferUsage::UNIFORM)) {
			const size_t min_ubo_alignment = _db.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
			SOUL_ASSERT(0, isPowerOfTwo(min_ubo_alignment), "");
			const size_t dynamic_alignment = (desc.typeSize + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
			alignment = std::max(alignment, dynamic_alignment);
		}
		if (desc.usageFlags.test(BufferUsage::STORAGE)) {
			const size_t min_ssbo_alignment = _db.physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
			SOUL_ASSERT(0, isPowerOfTwo(min_ssbo_alignment), "");
			const size_t dynamic_alignment = (desc.typeSize + min_ssbo_alignment - 1) & ~(min_ssbo_alignment - 1);
			alignment = std::max(alignment, dynamic_alignment);

		}

		const VkBufferCreateInfo buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = alignment * desc.count,
			.usage = vkCastBufferUsageFlags(desc.usageFlags),
			.sharingMode = queue_data.count > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = queue_data.count,
			.pQueueFamilyIndices = queue_data.indices
		};

		const VmaAllocationCreateInfo alloc_info = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};

		SOUL_VK_CHECK(vmaCreateBuffer(_db.gpuAllocator, &buffer_info, &alloc_info, &buffer.vkHandle, &buffer.allocation, nullptr), "Fail to create buffer");
		
		buffer.unitSize = alignment;
		buffer.desc = desc;
		buffer.owner = ResourceOwner::NONE;
		if (buffer.desc.usageFlags.test(BufferUsage::STORAGE))
			buffer.storageBufferGPUHandle = _db.descriptorAllocator.create_storage_buffer_descriptor(buffer.vkHandle);

		return buffer_id;
	}

	BufferID System::create_buffer(const BufferDesc& desc, const void* data) {
		SOUL_PROFILE_ZONE();
		BufferDesc new_desc = desc;
		new_desc.usageFlags |= { BufferUsage::TRANSFER_DST };
		new_desc.queueFlags |= { QueueType::TRANSFER };

		const BufferID buffer_id = create_buffer(new_desc);
		impl::Buffer& buffer = *get_buffer_ptr(buffer_id);

		get_frame_context().gpuResourceInitializer.load(buffer, data);

		return buffer_id;
	}

	void System::finalize_buffer(const BufferID buffer_id)
	{
		get_frame_context().gpuResourceFinalizer.finalize(*get_buffer_ptr(buffer_id));
	}

	void System::destroy_buffer_descriptor(BufferID buffer_id)
	{
		_db.descriptorAllocator.destroy_storage_buffer_descriptor(get_buffer_ptr(buffer_id)->storageBufferGPUHandle);
	}

	void System::destroy_buffer(BufferID id) {
		SOUL_ASSERT_MAIN_THREAD();
		get_frame_context().garbages.buffers.add(id);
	}

	Buffer *System::get_buffer_ptr(BufferID buffer_id) {
		return _db.bufferPool.get(buffer_id.id);
	}

	const Buffer& System::get_buffer(BufferID buffer_id) const {
		SOUL_ASSERT(!buffer_id.is_null(), "");
		return *_db.bufferPool.get(buffer_id.id);
	}

	_FrameContext &System::get_frame_context() {
		return _db.frameContexts[_db.currentFrame % _db.frameContexts.size()];
	}

	static std::string format_glsl_error_message(const char* source, const std::string& error_message) {
		std::string formatted_error_message;

		soul::Array<std::string> source_lines;
		source_lines.add(std::string(""));
		std::stringstream source_stream(source);
		for (std::string line; std::getline(source_stream, line, '\n');)
			source_lines.add(line);

		 auto get_surrounding_line = [](const std::string& filename, const uint64 line, const std::string& source_line) -> std::string {
			std::string result;
			result += "        ";
			result += filename;
			result += ":";
			result += std::to_string(line);
			result += ": ";
			result += source_line;
			result += '\n';
			return result;
		};

		std::stringstream error_stream(error_message);
		for (std::string line; std::getline(error_stream, line);) {
			const uint64 filename_end_pos = line.find(':');
			const std::string filename = line.substr(0, filename_end_pos);
			const uint64 source_line_end_pos = line.find(':', filename_end_pos);
			const auto source_line = soul::cast<int64>(std::stoull(line.substr(filename_end_pos + 1, source_line_end_pos)));
			
			for (int64 before_line = std::max<int64>(0, soul::cast<int64>(source_line - 5)); before_line < source_line; before_line++) {
				formatted_error_message += get_surrounding_line(filename, before_line, source_lines[before_line]);
			}

			formatted_error_message += " >>>>>> ";
			formatted_error_message += filename;
			formatted_error_message += ":";
			formatted_error_message += std::to_string(source_line);
			formatted_error_message += ": ";
			formatted_error_message += source_lines[source_line];
			formatted_error_message += '\n';

			for (int64 after_line = source_line + 1; after_line < source_line + 5 && after_line < soul::cast<int64>(source_lines.size()); after_line++) {
				formatted_error_message += get_surrounding_line(filename, after_line, source_lines[after_line]);
			}

			formatted_error_message += '\n';
			formatted_error_message += line;
			formatted_error_message += '\n';
			formatted_error_message += "====================================================";
			formatted_error_message += '\n';

		}
		
		return formatted_error_message;
	}


	PipelineStateID System::request_pipeline_state(const GraphicPipelineStateDesc& desc, VkRenderPass renderPass, const TextureSampleCount sample_count) {
		//TODO(kevinyu): Do we need to hash renderPass and sample_count as well?
		PipelineStateDesc key(desc);
		if (auto id = _db.pipeline_state_cache_.find(key); id != PipelineStateCache::NULLVAL)
		{
			return PipelineStateID(id);
		}

		auto create_graphic_pipeline_state = [this](const GraphicPipelineStateDesc& desc, VkRenderPass render_pass, const TextureSampleCount sample_count) -> PipelineState
		{
			runtime::ScopeAllocator<> scope_allocator("create_graphic_pipeline_state");

			Program* program = get_program_ptr(desc.programID);

			Array<VkPipelineShaderStageCreateInfo> shader_stage_infos(&scope_allocator);

			for (ShaderStage stage : EnumIter<ShaderStage>())
			{
				const Shader& shader = program->shaders[stage];
				if (shader.vkHandle == VK_NULL_HANDLE) continue;
				const VkPipelineShaderStageCreateInfo shader_stage_info = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = vkCast(stage),
					.module = shader.vkHandle,
					.pName = "main"
				};
				shader_stage_infos.add(shader_stage_info);
			}

			static auto PRIMITIVE_TOPOLOGY_MAP = EnumArray<Topology, VkPrimitiveTopology>::build_from_list({
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
			});

			const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = PRIMITIVE_TOPOLOGY_MAP[desc.inputLayout.topology],
				.primitiveRestartEnable = VK_FALSE
			};

			const VkViewport viewport = {
				.x = static_cast<float>(desc.viewport.offsetX),
				.y = static_cast<float>(desc.viewport.offsetY),
				.width = static_cast<float>(desc.viewport.width),
				.height = static_cast<float>(desc.viewport.height),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			};

			const VkRect2D scissor = {
				.offset = { desc.scissor.offsetX, desc.scissor.offsetY },
				.extent = { desc.scissor.width, desc.scissor.height }
			};

			const VkPipelineViewportStateCreateInfo viewport_state = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				.viewportCount = 1,
				.pViewports = &viewport,
				.scissorCount = 1,
				.pScissors = &scissor
			};

			static auto POLYGON_MODE_MAP = EnumArray<PolygonMode, VkPolygonMode>::build_from_list({
				VK_POLYGON_MODE_FILL,
				VK_POLYGON_MODE_LINE,
				VK_POLYGON_MODE_POINT
				});

			const VkPipelineRasterizationStateCreateInfo rasterizer_state = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = POLYGON_MODE_MAP[desc.raster.polygonMode],
				.depthBiasEnable = (desc.depthBias.slope != 0.0f || desc.depthBias.constant != 0.0f) ? VK_TRUE : VK_FALSE,
				.depthBiasConstantFactor = desc.depthBias.constant,
				.depthBiasSlopeFactor = desc.depthBias.slope,
				.lineWidth = desc.raster.lineWidth
			};

			const VkPipelineMultisampleStateCreateInfo multisample_state = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = vkCast(sample_count),
				.sampleShadingEnable = VK_FALSE
			};

			VkVertexInputAttributeDescription attr_descs[MAX_INPUT_PER_SHADER];
			uint32 attr_desc_count = 0;
			for (uint32 i = 0; i < MAX_INPUT_PER_SHADER; i++) {
				auto& input_attribute = desc.inputAttributes[i];
				if (input_attribute.type == VertexElementType::DEFAULT) continue;
				attr_descs[attr_desc_count].format = vkCast(input_attribute.type, input_attribute.flags);
				attr_descs[attr_desc_count].binding = desc.inputAttributes[i].binding;
				attr_descs[attr_desc_count].location = i;
				attr_descs[attr_desc_count].offset = desc.inputAttributes[i].offset;
				attr_desc_count++;
			}

			VkVertexInputBindingDescription input_binding_descs[MAX_INPUT_BINDING_PER_SHADER];
			uint32 input_binding_desc_count = 0;
			for (uint32 i = 0; i < MAX_INPUT_BINDING_PER_SHADER; i++) {
				if (desc.inputBindings[i].stride == 0) continue;
				input_binding_descs[input_binding_desc_count] = {
					i,
					desc.inputBindings[i].stride,
					VK_VERTEX_INPUT_RATE_VERTEX
				};
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
			std::transform(desc.colorAttachments, desc.colorAttachments + MAX_COLOR_ATTACHMENT_PER_SHADER, color_blend_attachments,
				[](const auto& attachment) -> VkPipelineColorBlendAttachmentState
				{
					return {
						.blendEnable = attachment.blendEnable ? VK_TRUE : VK_FALSE,
						.srcColorBlendFactor = vkCast(attachment.srcColorBlendFactor),
						.dstColorBlendFactor = vkCast(attachment.dstColorBlendFactor),
						.colorBlendOp = vkCast(attachment.colorBlendOp),
						.srcAlphaBlendFactor = vkCast(attachment.srcAlphaBlendFactor),
						.dstAlphaBlendFactor = vkCast(attachment.dstAlphaBlendFactor),
						.alphaBlendOp = vkCast(attachment.alphaBlendOp),
						.colorWriteMask = soul::cast<uint32>(attachment.colorWrite ? 0xf : 0x0)
					};
				});

			const VkPipelineColorBlendStateCreateInfo color_blending = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.logicOpEnable = VK_FALSE,
				.logicOp = VK_LOGIC_OP_COPY,
				.attachmentCount = desc.colorAttachmentCount,
				.pAttachments = color_blend_attachments,
				.blendConstants = {}
			};

			const VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = desc.depthStencilAttachment.depthTestEnable,
				.depthWriteEnable = desc.depthStencilAttachment.depthWriteEnable,
				.depthCompareOp = vkCast(desc.depthStencilAttachment.depthCompareOp),
				.depthBoundsTestEnable = VK_FALSE,
				.stencilTestEnable = VK_FALSE,
				.front = {},
				.back = {},
				.minDepthBounds = 0.0f,
				.maxDepthBounds = 1.0f
			};

			static constexpr VkDynamicState DYNAMIC_STATES[] = { VK_DYNAMIC_STATE_SCISSOR };
			const VkPipelineDynamicStateCreateInfo dynamic_state_info = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = 1,
				.pDynamicStates = DYNAMIC_STATES
			};

			const VkGraphicsPipelineCreateInfo pipeline_info = {
				.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				.stageCount = soul::cast<uint32>(shader_stage_infos.size()),
				.pStages = shader_stage_infos.data(),
				.pVertexInputState = &inputStateInfo,
				.pInputAssemblyState = &input_assembly_state,
				.pViewportState = &viewport_state,
				.pRasterizationState = &rasterizer_state,
				.pMultisampleState = &multisample_state,
				.pDepthStencilState = &depth_stencil_info,
				.pColorBlendState = &color_blending,
				.pDynamicState = desc.scissor.dynamic ? &dynamic_state_info : nullptr,
				.layout = get_bindless_pipeline_layout(),
				.renderPass = render_pass,
				.subpass = 0,
				.basePipelineHandle = VK_NULL_HANDLE
			};

			VkPipeline pipeline;
			{
				SOUL_PROFILE_ZONE_WITH_NAME("vkCreateGraphicsPipelines");
				SOUL_VK_CHECK(vkCreateGraphicsPipelines(_db.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), "Fail to create graphics pipelines");
			}

			shader_stage_infos.cleanup();
			return { pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, desc.programID };
		};
		return PipelineStateID(_db.pipeline_state_cache_.create(key, create_graphic_pipeline_state, desc, renderPass, sample_count));

	}

	PipelineStateID System::request_pipeline_state(const ComputePipelineStateDesc& desc)
	{
		PipelineStateDesc key(desc);
		if (auto id = _db.pipeline_state_cache_.find(key); id != PipelineStateCache::NULLVAL)
		{
			return PipelineStateID(id);
		}

		auto create_compute_pipeline_state = [this](const ComputePipelineStateDesc& desc) -> PipelineState
		{
			const Program* program = get_program_ptr(desc.programID);

			const Shader& compute_shader = program->shaders[ShaderStage::COMPUTE];
			const VkPipelineShaderStageCreateInfo compute_shader_stage_create_info = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = compute_shader.vkHandle,
				.pName = "main"
			};

			const VkComputePipelineCreateInfo create_info = {
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = compute_shader_stage_create_info,
				.layout = get_bindless_pipeline_layout()
			};

			VkPipeline pipeline;
			{
				SOUL_PROFILE_ZONE_WITH_NAME("vkCreateGraphicsPipelines");
				SOUL_VK_CHECK(vkCreateComputePipelines(_db.device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline), "Fail to create compute pipelines");
			}
			return { pipeline, VK_PIPELINE_BIND_POINT_COMPUTE, desc.programID };
		};

		return PipelineStateID(_db.pipeline_state_cache_.create(key, create_compute_pipeline_state, desc));
	}


	PipelineState* System::get_pipeline_state_ptr(const PipelineStateID pipeline_state_id) {
		return _db.pipeline_state_cache_.get(pipeline_state_id.id);
	}

	const PipelineState& System::get_pipeline_state(PipelineStateID pipeline_state_id) {
		return *_db.pipeline_state_cache_.get(pipeline_state_id.id);
	}

	VkPipelineLayout System::get_bindless_pipeline_layout() const
	{
		return _db.descriptorAllocator.get_pipeline_layout();
	}

	BindlessDescriptorSets System::get_bindless_descriptor_sets() const
	{
		return _db.descriptorAllocator.get_bindless_descriptor_sets();
	}

	ProgramID System::create_program(const ProgramDesc& program_desc) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::request_program");
		runtime::ScopeAllocator<> scope_allocator("GPU::System::programCreate");

		const auto program_id = ProgramID(_db.programs.add({}));
		Program& program = _db.programs[program_id.id];

		const slang::TargetDesc target_desc = {
			.format = SLANG_SPIRV,
			.profile = _db.slang_global_session->findProfile("glsl_450")
		};
		soul::Array<std::string> search_paths;
		soul::Array<const char*> slang_search_paths(&scope_allocator);
		for (const auto& path : std::span(program_desc.searchPaths, program_desc.searchPathCount))
		{
			search_paths.push_back(path.string());
			slang_search_paths.push_back(search_paths.back().c_str());
		}
		
		const slang::SessionDesc session_desc = {
			.targets = &target_desc,
			.targetCount = 1,
			.searchPaths = slang_search_paths.data(),
			.searchPathCount = soul::cast<SlangInt>(slang_search_paths.size())
		};
		Slang::ComPtr<slang::ISession> session;
		SOUL_ASSERT(0,
			SLANG_SUCCEEDED(_db.slang_global_session->createSession(session_desc, session.writeRef())),
			"Fail to create session");

		SlangCompileRequest* request = nullptr;
		session->createCompileRequest(&request);
		SCOPE_EXIT(spDestroyCompileRequest(request));

		int translation_unit_index = spAddTranslationUnit(request, SLANG_SOURCE_LANGUAGE_SLANG, "");
		spAddTranslationUnitSourceString(request, translation_unit_index, "resource_.slang", RESOURCE_SLANG);

		for (auto& source : std::span(program_desc.sources, program_desc.sourceCount)) {
			if (std::holds_alternative<ShaderFile>(source))
			{
				const ShaderFile& shader_file = std::get<ShaderFile>(source);
				const std::filesystem::path& path = shader_file.path;
				const std::filesystem::path full_path = [&path, &program_desc]()->std::filesystem::path
				{
					if (path.is_absolute())
					{
						SOUL_ASSERT(0, std::filesystem::exists(path), "%s does not exist", path.string().c_str());
						return std::filesystem::canonical(path);
					}
					else {
						for (const auto& search_path : std::span(program_desc.searchPaths, program_desc.searchPathCount))
						{
							auto full_path_candidate = search_path / path;
							if (std::filesystem::exists(full_path_candidate)) return std::filesystem::canonical(full_path_candidate);
						}
						SOUL_ASSERT(0, false, "Cannot find file %s in any search path", path.string().c_str());
						return "";
					}
				}();
				spAddTranslationUnitSourceFile(request, translation_unit_index, full_path.string().c_str());
			}
			else
			{
				const ShaderString& shader_string = std::get<ShaderString>(source);
				spAddTranslationUnitSourceString(request, translation_unit_index, "", shader_string.c_str());
			}
		}

		EnumArray<ShaderStage, int> entry_indexes;

		static constexpr auto SLANG_STAGE_MAP = EnumArray<ShaderStage, SlangStage>::build_from_list({
			SLANG_STAGE_VERTEX,
			SLANG_STAGE_GEOMETRY,
			SLANG_STAGE_FRAGMENT,
			SLANG_STAGE_COMPUTE
		});

		for (ShaderStage stage : EnumIter<ShaderStage>())
		{
			const char* entry_point_name = program_desc.entryPointNames[stage];
			if (entry_point_name == nullptr) continue;
			entry_indexes[stage] = spAddEntryPoint(request, translation_unit_index, entry_point_name, SLANG_STAGE_MAP[stage]);
		}
		SlangResult result = spCompile(request);
		if (SLANG_FAILED(result))
		{
			char const* diagnostics = spGetDiagnosticOutput(request);
			SOUL_ASSERT(0, false, "Slang Diagnostics = %s", diagnostics);
		}

		for (ShaderStage stage : EnumIter<ShaderStage>())
		{
			const char* entry_point_name = program_desc.entryPointNames[stage];
			if (entry_point_name == nullptr) continue;

			Shader& shader = program.shaders[stage];
			size_t spirv_code_size = 0;
			void const* spirv_code = spGetEntryPointCode(request, entry_indexes[stage], &spirv_code_size);
			const VkShaderModuleCreateInfo shader_module_create_info = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = spirv_code_size,
				.pCode = static_cast<const uint32_t*>(spirv_code)
			};
			SOUL_LOG_INFO("Code size = %d", spirv_code_size);
			SOUL_VK_CHECK(vkCreateShaderModule(_db.device, &shader_module_create_info, nullptr, &shader.vkHandle));
			shader.entryPoint = entry_point_name;
		}

		return program_id;
	}

	Program *System::get_program_ptr(const ProgramID program_id) {
		SOUL_ASSERT(0, program_id != PROGRAM_ID_NULL, "");
		return &_db.programs[program_id.id];
	}

	const Program& System::get_program(const ProgramID program_id) {
		SOUL_ASSERT(0, program_id != PROGRAM_ID_NULL, "");
		return _db.programs[program_id.id];
	}

	SamplerID System::request_sampler(const SamplerDesc &desc) {
		const uint64 hash_key = hash_fnv1(&desc);
		if (_db.samplerMap.isExist(hash_key)) return _db.samplerMap[hash_key];

		const VkSamplerCreateInfo sampler_create_info = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = vkCast(desc.magFilter),
			.minFilter = vkCast(desc.minFilter),
			.mipmapMode = vkCastMipmapFilter(desc.mipmapFilter),
			.addressModeU = vkCast(desc.wrapU),
			.addressModeV = vkCast(desc.wrapV),
			.addressModeW = vkCast(desc.wrapW),
			.anisotropyEnable = desc.anisotropyEnable,
			.maxAnisotropy = desc.maxAnisotropy,
			.compareEnable = desc.compareEnable ? VK_TRUE : VK_FALSE,
			.compareOp = vkCast(desc.comapreOp),
			.minLod = 0,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE
		};

		VkSampler sampler;
		SOUL_VK_CHECK(vkCreateSampler(_db.device, &sampler_create_info, nullptr, &sampler), "Fail to create sampler");
		DescriptorID descriptor_id = _db.descriptorAllocator.create_sampler_descriptor(sampler);
		const SamplerID sampler_id = { sampler, descriptor_id };
		_db.samplerMap.add(hash_key, sampler_id);

		return sampler_id;
	}

	VkRenderPass System::request_render_pass(const RenderPassKey& key)
	{
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();
		if (_db.renderPassMaps.contains(key)) 
		{
			return _db.renderPassMaps[key];
		}

		auto attachment_flag_to_load_op = [](AttachmentFlags flags) -> VkAttachmentLoadOp
		{
			if (flags & ATTACHMENT_CLEAR_BIT)
			{
				return VK_ATTACHMENT_LOAD_OP_CLEAR;
			}
			if ((flags & ATTACHMENT_FIRST_PASS_BIT) && !(flags & ATTACHMENT_EXTERNAL_BIT))
			{
				return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}
			
			return VK_ATTACHMENT_LOAD_OP_LOAD;
			
		};

		auto attachment_flag_to_store_op = [](AttachmentFlags flags) -> VkAttachmentStoreOp
		{
			if ((flags & ATTACHMENT_LAST_PASS_BIT) && !(flags & ATTACHMENT_EXTERNAL_BIT))
			{
				return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}
			return VK_ATTACHMENT_STORE_OP_STORE;
		};

		VkAttachmentDescription attachments[MAX_COLOR_ATTACHMENT_PER_SHADER  * 2 + MAX_INPUT_ATTACHMENT_PER_SHADER + 1] = {};
		VkAttachmentReference attachment_refs[MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
		uint8 attachment_count = 0;

		for (auto color_attachment : key.colorAttachments)
		{
			if (color_attachment.flags & ATTACHMENT_ACTIVE_BIT)
			{
				VkAttachmentDescription& attachment = attachments[attachment_count];
				attachment.format = vkCast(color_attachment.format);
				attachment.samples = vkCast(color_attachment.sampleCount);
				AttachmentFlags flags = color_attachment.flags;
				
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
		const uint8 color_attachment_count = attachment_count;

		for (auto input_attachment : key.inputAttachments)
		{
			const Attachment& attachmentKey = input_attachment;
			if (attachmentKey.flags & ATTACHMENT_ACTIVE_BIT) {
				VkAttachmentDescription& attachment = attachments[attachment_count];
				attachment.format = vkCast(attachmentKey.format);
				attachment.samples = vkCast(input_attachment.sampleCount);

				AttachmentFlags flags = attachmentKey.flags;

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
		const uint8 input_attachment_count = attachment_count - color_attachment_count;

		for (auto resolve_attachment : key.resolveAttachments)
		{
			if (resolve_attachment.flags & ATTACHMENT_ACTIVE_BIT)
			{
				VkAttachmentDescription& attachment = attachments[attachment_count];
				attachment.format = vkCast(resolve_attachment.format);
				attachment.samples = vkCast(resolve_attachment.sampleCount);
				AttachmentFlags flags = resolve_attachment.flags;

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
		const uint8 resolve_attachment_count = attachment_count - (input_attachment_count + color_attachment_count);

		SOUL_ASSERT(0, resolve_attachment_count == color_attachment_count || resolve_attachment_count == 0, "");
		VkSubpassDescription subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = input_attachment_count,
			.pInputAttachments = attachment_refs + color_attachment_count,
			.colorAttachmentCount = color_attachment_count,
			.pColorAttachments = attachment_refs,
			.pResolveAttachments =  resolve_attachment_count > 0 ? attachment_refs + color_attachment_count + input_attachment_count  : nullptr
		};

		if (key.depthAttachment.flags & ATTACHMENT_ACTIVE_BIT)
		{
			AttachmentFlags flags = key.depthAttachment.flags;
			VkAttachmentDescription& attachment = attachments[attachment_count];
			attachment.format = vkCast(key.depthAttachment.format);
			attachment.samples = vkCast(key.depthAttachment.sampleCount);

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
			.pDependencies = nullptr
		};

		VkRenderPass render_pass;
		SOUL_VK_CHECK(vkCreateRenderPass(_db.device, &render_pass_info, nullptr, &render_pass), "Fail to create render pass");

		_db.renderPassMaps.insert(key, render_pass);
		
		return render_pass;
	}
	
	VkFramebuffer System::create_framebuffer(const VkFramebufferCreateInfo &info) {
		SOUL_PROFILE_ZONE();
		VkFramebuffer framebuffer;
		SOUL_VK_CHECK(vkCreateFramebuffer(_db.device, &info, nullptr, &framebuffer), "Fail to create framebuffer");
		return framebuffer;
	}

	void System::destroy_framebuffer(VkFramebuffer framebuffer) {
		get_frame_context().garbages.frameBuffers.add(framebuffer);
	}

	VkEvent System::create_event() {
		SOUL_ASSERT_MAIN_THREAD();
		VkEvent event;
		const VkEventCreateInfo event_info = {.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};
		SOUL_VK_CHECK(vkCreateEvent(_db.device, &event_info, nullptr, &event), "Fail to create event");
		return event;
	}

	void System::destroy_event(VkEvent event) {
		SOUL_ASSERT_MAIN_THREAD();
		get_frame_context().garbages.events.add(event);
	}

	SemaphoreID System::create_semaphore() {
		SOUL_ASSERT_MAIN_THREAD();
		SemaphoreID semaphoreID = SemaphoreID(_db.semaphores.add(Semaphore()));
		Semaphore &semaphore = _db.semaphores[semaphoreID.id];
		semaphore = {};
		const VkSemaphoreCreateInfo semaphore_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		SOUL_VK_CHECK(vkCreateSemaphore(_db.device, &semaphore_info, nullptr, &semaphore.vkHandle), "Fail to create semaphore");
		return semaphoreID;
	}

	void System::reset_semaphore(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		get_semaphore_ptr(ID)->stageFlags = 0;
		get_semaphore_ptr(ID)->state = semaphore_state::INITIAL;
	}

	Semaphore *System::get_semaphore_ptr(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		return &_db.semaphores[ID.id];
	}

	void System::destroy_semaphore(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		get_frame_context().garbages.semaphores.add(ID);
	}

	void CommandQueue::init(VkDevice inDevice, uint32 inFamilyIndex, uint32 queueIndex)
	{
		device = inDevice;
		familyIndex = inFamilyIndex;
		vkGetDeviceQueue(device, familyIndex, queueIndex, &vkHandle);
	}

	void CommandQueue::wait(Semaphore* semaphore, VkPipelineStageFlags waitStage)
	{
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, waitStage != 0, "");

		SOUL_ASSERT(0, semaphore->state == semaphore_state::SUBMITTED, "");
		semaphore->state = semaphore_state::PENDING;

		if (!commands.empty()) {
			flush(0, nullptr, VK_NULL_HANDLE);
		}
		waitSemaphores.add(semaphore->vkHandle);
		waitStages.add(waitStage);
	}

	void CommandQueue::submit(VkCommandBuffer commandBuffer, const Array<Semaphore*>& semaphores, VkFence fence)
	{
		submit(commandBuffer, soul::cast<uint32>(semaphores.size()), semaphores.data(), fence);
	}

	void CommandQueue::submit(VkCommandBuffer commandBuffer, Semaphore* semaphore, VkFence fence)
	{
		submit(commandBuffer, 1, &semaphore, fence);
	}

	void CommandQueue::submit(VkCommandBuffer commandBuffer, uint32 semaphoreCount, Semaphore* const* semaphores, VkFence fence)
	{
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, semaphoreCount <= MAX_SIGNAL_SEMAPHORE, "");
		SOUL_PROFILE_ZONE();

		SOUL_VK_CHECK(vkEndCommandBuffer(commandBuffer), "Fail to end command buffer");

		commands.add(commandBuffer);

		for (soul_size semaphoreIdx = 0; semaphoreIdx < semaphoreCount; semaphoreIdx++) {
			Semaphore* semaphore = semaphores[semaphoreIdx];
			SOUL_ASSERT(0, semaphore->state == semaphore_state::INITIAL, "");
			semaphore->state = semaphore_state::SUBMITTED;
		}

		// TODO : Fix this
		if (semaphoreCount != 0 || fence != VK_NULL_HANDLE) {
			flush(semaphoreCount, semaphores, fence);
		}
	}

	void CommandQueue::flush(uint32 semaphoreCount, Semaphore* const* semaphores, VkFence fence)
	{
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = soul::cast<uint32>(commands.size());
		submitInfo.pCommandBuffers = commands.data();


		SOUL_ASSERT(0, waitSemaphores.size() == waitStages.size(), "");
		submitInfo.waitSemaphoreCount = soul::cast<uint32>(waitSemaphores.size());
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();

		VkSemaphore signalSemaphores[MAX_SIGNAL_SEMAPHORE];
		for (soul_size i = 0; i < semaphoreCount; i++) {
			signalSemaphores[i] = semaphores[i]->vkHandle;
		}

		submitInfo.signalSemaphoreCount = semaphoreCount;
		submitInfo.pSignalSemaphores = signalSemaphores;

		SOUL_VK_CHECK(vkQueueSubmit(vkHandle, 1, &submitInfo, fence), "Fail to submit queue");
		commands.resize(0);
		waitSemaphores.resize(0);
		waitStages.resize(0);
	}

	void CommandQueue::present(const VkPresentInfoKHR& presentInfo)
	{
		SOUL_VK_CHECK(vkQueuePresentKHR(vkHandle, &presentInfo), "Fail to present queue");
	}
	
	void System::execute(const RenderGraph &renderGraph) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();

		RenderGraphExecution execution(&renderGraph, this, runtime::get_context_allocator(), _db.queues, get_frame_context().commandPools);
		execution.init();
		get_frame_context().gpuResourceInitializer.flush(_db.queues, *this);
		get_frame_context().gpuResourceFinalizer.flush(get_frame_context().commandPools, _db.queues, *this);
		execution.run();
		execution.cleanup();
	}

	void System::flush_frame() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		_frameEnd();
		_frameBegin();
	}

	void System::_frameBegin() {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		_FrameContext &frameContext = get_frame_context();
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Wait fence");
			SOUL_VK_CHECK(vkWaitForFences(_db.device, 1, &frameContext.fence, VK_TRUE, UINT64_MAX), "Fail to wait fences");
		}
		
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Reset command pools");
			frameContext.commandPools.reset();
		}
		
		frameContext.gpuResourceInitializer.reset();

		reset_semaphore(frameContext.imageAvailableSemaphore);
		reset_semaphore(frameContext.renderFinishedSemaphore);

		uint32 swapchainIndex;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Acquire Next Image KHR");
			vkAcquireNextImageKHR(_db.device, _db.swapchain.vkHandle, UINT64_MAX,
			                      get_semaphore_ptr(frameContext.imageAvailableSemaphore)->vkHandle, VK_NULL_HANDLE,
			                      &swapchainIndex);
		}
		
		get_semaphore_ptr(frameContext.imageAvailableSemaphore)->state = semaphore_state::SUBMITTED;
		frameContext.swapchainIndex = swapchainIndex;
		TextureID swapchainTextureID = _db.swapchain.textures[swapchainIndex];
		Texture *swapchainTexture = get_texture_ptr(swapchainTextureID);
		swapchainTexture->owner = ResourceOwner::PRESENTATION_ENGINE;
		swapchainTexture->layout = VK_IMAGE_LAYOUT_UNDEFINED;

		{
			SOUL_PROFILE_ZONE_WITH_NAME("Destroy images");
			auto destroy_texture_view = [this](TextureView& texture_view)
			{
				vkDestroyImageView(_db.device, texture_view.vkHandle, nullptr);
			};
			for (TextureID texture_id : frameContext.garbages.textures) {
				destroy_texture_descriptor(texture_id);
				Texture& texture = *get_texture_ptr(texture_id);
				vmaDestroyImage(_db.gpuAllocator, texture.vkHandle, texture.allocation);
				destroy_texture_view(texture.view);
				soul_size view_count = soul::cast<uint64>(texture.desc.mipLevels) * texture.desc.layerCount;
				if (texture.views != nullptr) {
					std::for_each_n(texture.views, view_count, destroy_texture_view);
					_db.cpuAllocator.destroy_array(texture.views, view_count);
					texture.views = nullptr;
				}
				_db.texturePool.destroy(texture_id.id);
			}
			frameContext.garbages.textures.resize(0);
		}
		

		{
			SOUL_PROFILE_ZONE_WITH_NAME("Destroy buffers");
			for (BufferID bufferID : frameContext.garbages.buffers) {
				destroy_buffer_descriptor(bufferID);
				Buffer& buffer = *get_buffer_ptr(bufferID);
				vmaDestroyBuffer(_db.gpuAllocator, buffer.vkHandle, buffer.allocation);
				_db.descriptorAllocator.destroy_storage_buffer_descriptor(buffer.storageBufferGPUHandle);
				_db.bufferPool.destroy(bufferID.id);
			}
			frameContext.garbages.buffers.resize(0);
		}

		{
			SOUL_PROFILE_ZONE_WITH_NAME("Destroy render passes");
			for (VkRenderPass renderPass : frameContext.garbages.renderPasses) {
				vkDestroyRenderPass(_db.device, renderPass, nullptr);
			}
			frameContext.garbages.renderPasses.resize(0);
		}
		

		for (VkFramebuffer framebuffer : frameContext.garbages.frameBuffers) {
			vkDestroyFramebuffer(_db.device, framebuffer, nullptr);
		}
		frameContext.garbages.frameBuffers.resize(0);

		for (VkPipeline pipeline: frameContext.garbages.pipelines) {
			vkDestroyPipeline(_db.device, pipeline, nullptr);
		}
		frameContext.garbages.pipelines.resize(0);

		for (VkEvent event : frameContext.garbages.events) {
			vkDestroyEvent(_db.device, event, nullptr);
		}
		frameContext.garbages.events.resize(0);

		for (SemaphoreID semaphoreID: frameContext.garbages.semaphores) {
			vkDestroySemaphore(_db.device, get_semaphore_ptr(semaphoreID)->vkHandle, nullptr);
			_db.semaphores.remove(semaphoreID.id);
		}
		frameContext.garbages.semaphores.resize(0);

		_db.pipeline_state_cache_.on_new_frame();
	}

	void System::_frameEnd() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		_FrameContext &frameContext = get_frame_context();
		uint32 swapchainIndex = frameContext.swapchainIndex;
		if (_db.swapchain.fences[swapchainIndex] != VK_NULL_HANDLE) {
			SOUL_VK_CHECK(vkWaitForFences(_db.device, 1, &_db.swapchain.fences[swapchainIndex], VK_TRUE, UINT64_MAX), "Fail to wait fences");
		}
		_db.swapchain.fences[swapchainIndex] = frameContext.fence;

		Texture &swapchainTexture = *get_texture_ptr(_db.swapchain.textures[swapchainIndex]);
		SOUL_VK_CHECK(vkResetFences(_db.device, 1, &frameContext.fence), "Fail to reset fences");
		{
			SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::LastSubmission");
			frameContext.gpuResourceInitializer.flush(_db.queues, *this);
			frameContext.gpuResourceFinalizer.flush(get_frame_context().commandPools, _db.queues, *this);

			SOUL_ASSERT(0,
			            swapchainTexture.owner == ResourceOwner::GRAPHIC_QUEUE
			            || swapchainTexture.owner == ResourceOwner::PRESENTATION_ENGINE,
			            "");
			// TODO: Handle when swapchain texture is untouch (ResourceOwner is PRESENTATION_ENGINE)
			VkCommandBuffer cmdBuffer = frameContext.commandPools.requestCommandBuffer(QueueType::GRAPHIC);

			VkImageMemoryBarrier imageBarrier = {};
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			imageBarrier.dstAccessMask = 0;
			imageBarrier.oldLayout = swapchainTexture.layout;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			imageBarrier.image = swapchainTexture.vkHandle;
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(cmdBuffer,
			                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			                     0,
			                     0,
			                     nullptr,
			                     0,
			                     nullptr,
			                     1,
			                     &imageBarrier);

			static auto  RESOURCE_OWNER_TO_QUEUE_TYPE = EnumArray<ResourceOwner, QueueType>::build_from_list({
				QueueType::COUNT,
				QueueType::GRAPHIC,
				QueueType::COMPUTE,
				QueueType::TRANSFER,
				QueueType::GRAPHIC
			});

			auto _syncQueueToGraphic = [this](QueueType queueType) {
				SemaphoreID semaphoreID = create_semaphore();
				VkCommandBuffer cmdBuffer = get_frame_context().commandPools.requestCommandBuffer(queueType);
				Semaphore* semaphorePtr = get_semaphore_ptr(semaphoreID);
				_db.queues[queueType].submit(cmdBuffer, 1, &semaphorePtr);
				_db.queues[QueueType::GRAPHIC].wait(semaphorePtr, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
				destroy_semaphore(semaphoreID);
			};

			{
				SOUL_PROFILE_ZONE_WITH_NAME("Sync Transfer");
				_syncQueueToGraphic(QueueType::TRANSFER);
			}

			{
				SOUL_PROFILE_ZONE_WITH_NAME("Sync Compute");
				_syncQueueToGraphic(QueueType::COMPUTE);
			}

			if (swapchainTexture.owner == ResourceOwner::PRESENTATION_ENGINE) {
				_db.queues[QueueType::GRAPHIC].wait(get_semaphore_ptr(get_frame_context().imageAvailableSemaphore), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			}

			_db.queues[RESOURCE_OWNER_TO_QUEUE_TYPE[swapchainTexture.owner]].submit(cmdBuffer, get_semaphore_ptr(get_frame_context().renderFinishedSemaphore), frameContext.fence);
			swapchainTexture.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &get_semaphore_ptr(frameContext.renderFinishedSemaphore)->vkHandle;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &_db.swapchain.vkHandle;
		presentInfo.pImageIndices = &frameContext.swapchainIndex;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::QueuePresent");
			_db.queues[QueueType::GRAPHIC].present(presentInfo);
		}
		
		_db.frameCounter++;
		_db.currentFrame += 1;
		_db.currentFrame %= _db.frameContexts.size();
	}

	Vec2ui32 System::get_swapchain_extent() {
		return {_db.swapchain.extent.width, _db.swapchain.extent.height};
	}

	TextureID System::get_swapchain_texture() {
		return _db.swapchain.textures[get_frame_context().swapchainIndex];
	}

	DescriptorID System::get_ssbo_descriptor_id(BufferID buffer_id) const
	{
		return get_buffer(buffer_id).storageBufferGPUHandle;
	}

	DescriptorID System::get_srv_descriptor_id(TextureID texture_id, std::optional<SubresourceIndex> subresource_index)
	{
		return get_texture_view(texture_id, subresource_index).sampledImageGPUHandle;
	}

	DescriptorID System::get_uav_descriptor_id(TextureID texture_id, std::optional<SubresourceIndex> subresource_index)
	{
		return get_texture_view(texture_id, subresource_index).storageImageGPUHandle;
	}

	DescriptorID System::get_sampler_descriptor_id(SamplerID sampler_id) const
	{
		return sampler_id.descriptorID;
	}

	void PrimaryCommandBuffer::begin_render_pass(const VkRenderPassBeginInfo& render_pass_begin_info, VkSubpassContents subpass_contents)
	{
		vkCmdBeginRenderPass(vk_handle_, &render_pass_begin_info, subpass_contents);
	}

	void PrimaryCommandBuffer::end_render_pass()
	{
		vkCmdEndRenderPass(vk_handle_);
	}

	void PrimaryCommandBuffer::execute_secondary_command_buffers(uint32_t count, const SecondaryCommandBuffer* secondary_command_buffers)
	{
		static_assert(sizeof(SecondaryCommandBuffer) == sizeof(VkCommandBuffer));
		auto command_buffers = reinterpret_cast<const VkCommandBuffer*>(secondary_command_buffers);
		vkCmdExecuteCommands(vk_handle_, count, command_buffers);
	}

	void SecondaryCommandBuffer::end()
	{
		vkEndCommandBuffer(vk_handle_);
	}

	void CommandPool::init(VkDevice inDevice, VkCommandBufferLevel inLevel, uint32 queueFamilyIndex)
	{
		SOUL_ASSERT(0, inDevice != VK_NULL_HANDLE, "Device is invalid!");
		device = inDevice;
		level = inLevel;
		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &vkHandle);
		count = 0;
	}

	VkCommandBuffer CommandPool::request()
	{
		if (allocatedBuffers.size() == count) {
			VkCommandBuffer cmdBuffer;
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = vkHandle;
			allocInfo.level = level;
			allocInfo.commandBufferCount = 1;

			SOUL_VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer), "Fail to allocate command buffers");
			allocatedBuffers.add(cmdBuffer);
		}

		VkCommandBuffer cmdBuffer = allocatedBuffers[count];
		count++;
		return cmdBuffer;
	}

	void CommandPool::reset()
	{
		vkResetCommandPool(device, vkHandle, 0);
		count = 0;
	}
	
	void CommandPools::init(VkDevice device, const CommandQueues& queues, soul_size thread_count)
	{
		SOUL_ASSERT_MAIN_THREAD();
		runtime::push_allocator(allocator_);

		secondary_pools_.resize(thread_count);
		for (auto& pool: secondary_pools_)
		{
			pool.init(device, VK_COMMAND_BUFFER_LEVEL_SECONDARY, queues[QueueType::GRAPHIC].getFamilyIndex());
		}

		primary_pools_.resize(thread_count);
		for (auto& pools : primary_pools_)
		{
			for (const auto queue_type : EnumIter<QueueType>())
			{
				pools[queue_type].init(device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, queues[queue_type].getFamilyIndex());
			}
		}

		runtime::pop_allocator();
	}

	void CommandPools::reset()
	{
		for (auto& pools : primary_pools_)
		{
			for (auto& pool : pools)
			{
				pool.reset();
			}
		}

		for (auto& pool : secondary_pools_)
		{
			pool.reset();
		}
	}

	VkCommandBuffer CommandPools::requestCommandBuffer(QueueType queueType)
	{
		VkCommandBuffer cmdBuffer = primary_pools_[runtime::get_thread_id()][queueType].request();
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};
		SOUL_VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo), "");
		return cmdBuffer;
	}

	PrimaryCommandBuffer CommandPools::request_command_buffer(const QueueType queue_type)
	{
		VkCommandBuffer cmd_buffer = primary_pools_[runtime::get_thread_id()][queue_type].request();
		const VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};
		SOUL_VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &beginInfo), "");
		return PrimaryCommandBuffer(cmd_buffer);
	}

	SecondaryCommandBuffer CommandPools::request_secondary_command_buffer(VkRenderPass render_pass, const uint32_t subpass, VkFramebuffer framebuffer)
	{
		VkCommandBuffer cmd_buffer = secondary_pools_[runtime::get_thread_id()].request();
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
		};

		VkCommandBufferInheritanceInfo inheritance_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			.renderPass = render_pass,
			.subpass = subpass,
			.framebuffer = framebuffer
		};

		beginInfo.pInheritanceInfo = &inheritance_info;

		vkBeginCommandBuffer(cmd_buffer, &beginInfo);

		return SecondaryCommandBuffer(cmd_buffer);
	}

	GPUResourceInitializer::ThreadContext& GPUResourceInitializer::get_thread_context()
	{
		return thread_contexts_[runtime::get_thread_id()];
	}

	GPUResourceInitializer::StagingBuffer GPUResourceInitializer::get_staging_buffer(soul_size size)
	{
		const VkBufferCreateInfo staging_buffer_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		};
		const VmaAllocationCreateInfo staging_alloc_info = {
			.usage = VMA_MEMORY_USAGE_CPU_ONLY
		};
		
		StagingBuffer staging_buffer = {};
		SOUL_VK_CHECK(vmaCreateBuffer(gpu_allocator_, &staging_buffer_info, &staging_alloc_info, &staging_buffer.vkHandle,
			&staging_buffer.allocation, nullptr), "");
		get_thread_context().staging_buffers_.add(staging_buffer);
		return staging_buffer;
	}

	PrimaryCommandBuffer GPUResourceInitializer::get_transfer_command_buffer()
	{
		if (get_thread_context().transfer_command_buffer_.is_null())
		{
			get_thread_context().transfer_command_buffer_ = command_pools_->request_command_buffer(QueueType::TRANSFER);
		}
		return get_thread_context().transfer_command_buffer_;
	}

	PrimaryCommandBuffer GPUResourceInitializer::get_mipmap_gen_command_buffer()
	{
		if (get_thread_context().mipmap_gen_command_buffer_.is_null())
		{
			get_thread_context().mipmap_gen_command_buffer_ = command_pools_->request_command_buffer(QueueType::GRAPHIC);
		}
		return get_thread_context().mipmap_gen_command_buffer_;
	}

	PrimaryCommandBuffer GPUResourceInitializer::get_clear_command_buffer()
	{
		if (get_thread_context().clear_command_buffer_.is_null())
		{
			get_thread_context().clear_command_buffer_ = command_pools_->request_command_buffer(QueueType::GRAPHIC);
		}
		return get_thread_context().clear_command_buffer_;
	}

	void GPUResourceInitializer::load_staging_buffer(const StagingBuffer& buffer, const void* data, soul_size size)
	{
		void* mappedData;
		vmaMapMemory(gpu_allocator_, buffer.allocation, &mappedData);
		memcpy(mappedData, data, size);
		vmaUnmapMemory(gpu_allocator_, buffer.allocation);
	}

	void GPUResourceInitializer::load_staging_buffer(const StagingBuffer& buffer, const void* data, soul_size count, soul_size type_size, soul_size stride)
	{
		void* mapped_data;
		vmaMapMemory(gpu_allocator_, buffer.allocation, &mapped_data);
		const byte* src_base = soul::cast<byte*>(mapped_data);
		const byte* dst_base = soul::cast<byte*>(data);
		for (soul_size i = 0; i < count; i++)
		{
			memcpy(soul::cast<void*>(src_base + (i * stride)), soul::cast<void*>(dst_base + (i * type_size)), type_size);
		}
		vmaUnmapMemory(gpu_allocator_, buffer.allocation);
	}

	void GPUResourceInitializer::init(VmaAllocator gpuAllocator, CommandPools* commandPools)
	{
		gpu_allocator_ = gpuAllocator;
		command_pools_ = commandPools;
		thread_contexts_.resize(runtime::get_thread_count());
		for (auto& context : thread_contexts_)
		{
			memory::Allocator* default_allocator = GetDefaultAllocator();
			context.staging_buffers_ = Array<StagingBuffer>(default_allocator);
		}
	}

	void GPUResourceInitializer::load(Buffer& buffer, const void* data)
	{
		SOUL_ASSERT(0, buffer.owner == ResourceOwner::NONE, "Buffer must be uninitialized!");
		soul_size buffer_size = buffer.unitSize * buffer.desc.count;
		StagingBuffer staging_buffer = get_staging_buffer(buffer_size);
		load_staging_buffer(staging_buffer, data, buffer.desc.count, buffer.desc.typeSize, buffer.unitSize);

		const VkBufferCopy copy_region = {
			.size = buffer_size
		};

		const char* name = "Unknown texture";
		const VkDebugUtilsLabelEXT pass_label = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
			nullptr,                                  // pNext
			name,                           // pLabelName
			{},             // color
		};

		VkCommandBuffer transfer_command_buffer = get_transfer_command_buffer().get_vk_handle();
		vkCmdBeginDebugUtilsLabelEXT(transfer_command_buffer, &pass_label);
		vkCmdCopyBuffer(transfer_command_buffer, staging_buffer.vkHandle, buffer.vkHandle, 1, &copy_region);
		vkCmdEndDebugUtilsLabelEXT(transfer_command_buffer);

		buffer.owner = ResourceOwner::TRANSFER_QUEUE;
	}

	void GPUResourceInitializer::load(Texture& texture, const TextureLoadDesc& loadDesc)
	{
		SOUL_ASSERT(0, texture.layout == VK_IMAGE_LAYOUT_UNDEFINED, "Texture must be uninitialized!");
		SOUL_ASSERT(0, texture.owner == ResourceOwner::NONE, "Texture must be uninitialized!");
		
		StagingBuffer staging_buffer = get_staging_buffer(loadDesc.dataSize);
		load_staging_buffer(staging_buffer, loadDesc.data, loadDesc.dataSize);

		const VkImageMemoryBarrier before_transfer_barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = texture.vkHandle,
			.subresourceRange = {
				.aspectMask = vkCastFormatToAspectFlags(texture.desc.format),
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = VK_REMAINING_ARRAY_LAYERS
			}
		};

		VkCommandBuffer transfer_command_buffer = get_transfer_command_buffer().get_vk_handle();

		vkCmdPipelineBarrier(
			transfer_command_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &before_transfer_barrier);

		for (soul_size load_idx = 0; load_idx < loadDesc.regionLoadCount; load_idx++)
		{
			const TextureRegionLoad& region_load = loadDesc.regionLoads[load_idx];

			SOUL_ASSERT(0, region_load.textureRegion.extent.z == 1, "3D texture is not supported yet");
			const VkBufferImageCopy copy_region = {
				.bufferOffset = region_load.bufferOffset,
				.bufferRowLength = region_load.bufferRowLength,
				.bufferImageHeight = region_load.bufferImageHeight,
				.imageSubresource = {
					.aspectMask = vkCastFormatToAspectFlags(texture.desc.format),
					.mipLevel = region_load.textureRegion.mipLevel,
					.baseArrayLayer = region_load.textureRegion.baseArrayLayer,
					.layerCount = region_load.textureRegion.layerCount
				},
				.imageOffset = {
					.x = region_load.textureRegion.offset.x,
					.y = region_load.textureRegion.offset.y,
					.z = region_load.textureRegion.offset.z
				},
				.imageExtent = {
					.width = region_load.textureRegion.extent.x,
					.height = region_load.textureRegion.extent.y,
					.depth = region_load.textureRegion.extent.z
				}
			};
			SOUL_ASSERT(0, region_load.textureRegion.extent.z == 1, "3D texture is not supported yet");
			
			vkCmdCopyBufferToImage(transfer_command_buffer,
				staging_buffer.vkHandle,
				texture.vkHandle,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copy_region);
		}

		texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		texture.owner = ResourceOwner::TRANSFER_QUEUE;
	}

	void GPUResourceInitializer::clear(Texture& texture, ClearValue clearValue)
	{
		const VkImageMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = texture.vkHandle,
			.subresourceRange = {
				.aspectMask = vkCastFormatToAspectFlags(texture.desc.format),
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = VK_REMAINING_ARRAY_LAYERS

			}
		};

		VkCommandBuffer clear_command_buffer = get_clear_command_buffer().get_vk_handle();
		vkCmdPipelineBarrier(
			clear_command_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		const VkImageSubresourceRange range = {
			.aspectMask = vkCastFormatToAspectFlags(texture.desc.format),
			.levelCount = texture.desc.mipLevels,
			.layerCount = 1
		};
		if (range.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
			const VkClearDepthStencilValue vk_clear_value = { clearValue.depthStencil.depth, clearValue.depthStencil.stencil };
			SOUL_ASSERT(0, !(range.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT), "");
			vkCmdClearDepthStencilImage(
				clear_command_buffer,
				texture.vkHandle,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&vk_clear_value,
				1,
				&range);
		}
		else {

			VkClearColorValue vk_clear_value;
			memcpy(&vk_clear_value, &clearValue, sizeof(vk_clear_value));
			vkCmdClearColorImage(
				clear_command_buffer,
				texture.vkHandle,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&vk_clear_value,
				1,
				&range);
		}
		texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		texture.owner = ResourceOwner::GRAPHIC_QUEUE;
	}

	void GPUResourceInitializer::generate_mipmap(Texture& texture)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = texture.vkHandle;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		auto mip_width = cast<int32>(texture.desc.extent.x);
		auto mip_height = cast<int32>(texture.desc.extent.y);

		VkCommandBuffer command_buffer = get_mipmap_gen_command_buffer().get_vk_handle();

		for (uint32 i = 1; i < texture.desc.mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mip_width, mip_height, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(command_buffer,
				texture.vkHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				texture.vkHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mip_width > 1) mip_width /= 2;
			if (mip_height > 1) mip_height /= 2;
		}

		barrier.subresourceRange.baseMipLevel = texture.desc.mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texture.owner = ResourceOwner::GRAPHIC_QUEUE;
	}

	void GPUResourceInitializer::flush(CommandQueues& command_queues, System& gpu_system)
	{
		SOUL_ASSERT_MAIN_THREAD();
		for (auto& thread_context : thread_contexts_)
		{
			if (!thread_context.clear_command_buffer_.is_null())
			{
				command_queues[QueueType::GRAPHIC].submit(thread_context.clear_command_buffer_.get_vk_handle());
			}
			if (!thread_context.transfer_command_buffer_.is_null() && thread_context.mipmap_gen_command_buffer_.is_null())
			{
				command_queues[QueueType::TRANSFER].submit(thread_context.transfer_command_buffer_.get_vk_handle());
			}
			else if (thread_context.transfer_command_buffer_.is_null() && !thread_context.mipmap_gen_command_buffer_.is_null())
			{
				command_queues[QueueType::GRAPHIC].submit(thread_context.mipmap_gen_command_buffer_.get_vk_handle());
			}
			else if (!thread_context.transfer_command_buffer_.is_null() && !thread_context.mipmap_gen_command_buffer_.is_null())
			{
				SemaphoreID mipmap_semaphore = gpu_system.create_semaphore();
				Semaphore* mipmap_semaphore_ptr = gpu_system.get_semaphore_ptr(mipmap_semaphore);
				command_queues[QueueType::TRANSFER].submit(thread_context.transfer_command_buffer_.get_vk_handle(), 1, &mipmap_semaphore_ptr);
				command_queues[QueueType::GRAPHIC].wait(mipmap_semaphore_ptr, VK_PIPELINE_STAGE_TRANSFER_BIT);
				command_queues[QueueType::GRAPHIC].submit(thread_context.mipmap_gen_command_buffer_.get_vk_handle());
			}

			thread_context.clear_command_buffer_ = PrimaryCommandBuffer();
			thread_context.mipmap_gen_command_buffer_ = PrimaryCommandBuffer();
			thread_context.transfer_command_buffer_ = PrimaryCommandBuffer();
		}
		
	}

	void GPUResourceInitializer::reset()
	{
		SOUL_ASSERT_MAIN_THREAD();

		for (auto& thread_context : thread_contexts_)
		{
			for (const StagingBuffer& buffer : thread_context.staging_buffers_) {
				vmaDestroyBuffer(gpu_allocator_, buffer.vkHandle, buffer.allocation);
			}
			thread_context.staging_buffers_.resize(0);
		}
	}

	void GPUResourceFinalizer::init()
	{
		thread_contexts_.resize(runtime::get_thread_count());

	}

	void GPUResourceFinalizer::finalize(Buffer& buffer)
	{
		thread_contexts_[runtime::get_thread_id()].sync_dst_queues_[RESOURCE_OWNER_TO_QUEUE_TYPE[buffer.owner]] |= buffer.desc.queueFlags;
		buffer.owner = ResourceOwner::NONE;
	}

	void GPUResourceFinalizer::finalize(Texture& texture, TextureUsageFlags usage_flags)
	{
		if (texture.owner == ResourceOwner::NONE) {
			return;
		}

		static auto get_finalize_layout = [](TextureUsageFlags usage) -> VkImageLayout
		{
			VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
			static constexpr VkImageLayout USAGE_LAYOUT_MAP[] = {
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};
			static_assert(std::size(USAGE_LAYOUT_MAP) == to_underlying(TextureUsage::COUNT));
			Util::ForEachBit(usage.val(), [&result](uint32 bit)
			{
				if (result != VK_IMAGE_LAYOUT_UNDEFINED && result != USAGE_LAYOUT_MAP[bit])
				{
					result = VK_IMAGE_LAYOUT_GENERAL;
				} else
				{
					result = USAGE_LAYOUT_MAP[bit];
				}
			});
			SOUL_ASSERT(0, result != VK_IMAGE_LAYOUT_UNDEFINED, "");
			return result;
		};

		VkImageLayout finalize_layout = get_finalize_layout(usage_flags);
		if (texture.layout != finalize_layout)
		{
			// TODO(kevinyu): Check if use specific read access (for example use VK_ACCESS_TRANSFER_SRC_BIT only for VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) is faster
			const VkImageMemoryBarrier barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.oldLayout = texture.layout,
				.newLayout = finalize_layout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = texture.vkHandle,
				.subresourceRange = {
					.aspectMask = vkCastFormatToAspectFlags(texture.desc.format),
					.baseMipLevel = 0,
					.levelCount = VK_REMAINING_MIP_LEVELS,
					.baseArrayLayer = 0,
					.layerCount = VK_REMAINING_ARRAY_LAYERS

				}
			};
			thread_contexts_[runtime::get_thread_id()].image_barriers_[RESOURCE_OWNER_TO_QUEUE_TYPE[texture.owner]].add(barrier);

			texture.layout = finalize_layout;
		}

		thread_contexts_[runtime::get_thread_id()].sync_dst_queues_[RESOURCE_OWNER_TO_QUEUE_TYPE[texture.owner]] |= texture.desc.queueFlags;
		texture.owner = ResourceOwner::NONE;
	}

	void GPUResourceFinalizer::flush(CommandPools& command_pools, CommandQueues& command_queues, System& gpu_system)
	{
		SOUL_ASSERT_MAIN_THREAD();
		runtime::ScopeAllocator scope_allocator("Resource Finalizer Flush");

		EnumArray<QueueType, VkCommandBuffer> command_buffers(VK_NULL_HANDLE);
		EnumArray<QueueType, Array<Semaphore*>> signal_semaphores((Array<Semaphore*>(&scope_allocator)));
		EnumArray<QueueType, Array<SemaphoreID>> wait_semaphores((Array<SemaphoreID>(&scope_allocator)));

		// create command buffer and create semaphore
		for (auto queue_type : EnumIter<QueueType>())
		{
			QueueFlags sync_dst_queue = std::accumulate(thread_contexts_.begin(), thread_contexts_.end(), QueueFlags({}),
				[queue_type](const QueueFlags flag, const ThreadContext& thread_context) -> QueueFlags
				{
					return flag | thread_context.sync_dst_queues_[queue_type];
				});

			soul_size image_barrier_capacity = std::accumulate(thread_contexts_.begin(), thread_contexts_.end(), 0,
				[queue_type](const soul_size capacity, const ThreadContext& thread_context) -> soul_size
				{
					return capacity + thread_context.image_barriers_[queue_type].size();
				});
			Array<VkImageMemoryBarrier> image_barriers(&scope_allocator);
			image_barriers.reserve(image_barrier_capacity);
			for (const auto& thread_context : thread_contexts_)
			{
				image_barriers.append(thread_context.image_barriers_[queue_type]);
			}

			sync_dst_queue.forEach([&](const QueueType dst_queue_type)
			{
				if (dst_queue_type == queue_type)
				{
					command_buffers[queue_type] = command_pools.request_command_buffer(queue_type).get_vk_handle();
					const VkMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT
					};

					vkCmdPipelineBarrier(command_buffers[queue_type], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
						1, &barrier,
						0, nullptr,
						soul::cast<uint32>(image_barriers.size()), image_barriers.data());
				} else
				{
					const SemaphoreID semaphore_id = gpu_system.create_semaphore();
					Semaphore* semaphore = gpu_system.get_semaphore_ptr(semaphore_id);
					signal_semaphores[queue_type].add(semaphore);
					wait_semaphores[dst_queue_type].add(semaphore_id);
				}
			});
		}

		// submit command buffer
		for (auto queue_type : EnumIter<QueueType>())
		{
			if (VkCommandBuffer command_buffer = command_buffers[queue_type]; command_buffer != VK_NULL_HANDLE)
			{
				command_queues[queue_type].submit(command_buffer, signal_semaphores[queue_type]);
			}
		}

		// wait and destroy semaphore
		for (auto queue_type : EnumIter<QueueType>())
		{
			for (SemaphoreID semaphore_id : wait_semaphores[queue_type])
			{
				command_queues[queue_type].wait(gpu_system.get_semaphore_ptr(semaphore_id), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				gpu_system.destroy_semaphore(semaphore_id);
			}
		}

		for (auto& context : thread_contexts_)
		{
			for (auto& image_barrier_list : context.image_barriers_)
			{
				image_barrier_list.resize(0);
			}
			context.sync_dst_queues_ = EnumArray<QueueType, QueueFlags>(QueueFlags());
		}
	}
	
}
