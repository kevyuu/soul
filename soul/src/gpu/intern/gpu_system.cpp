#include "gpu/system.h"
#include "gpu/render_graph.h"
#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_graph_execution.h"

#include "runtime/system.h"

#include "runtime/scope_allocator.h"

#include "core/array.h"
#include "core/dev_util.h"

#include <volk/volk.h>

#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>
#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>

#include <cstddef>
#include <limits>
#include <string>
#include <sstream>

namespace Soul::GPU
{
	using namespace impl;

	static void* VmaAllocCallback(void* userData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
		const auto allocator = (Memory::Allocator*)userData;
		return allocator->allocate(size, alignment);
	}

	static void* VmaReallocationCallback(void* userData, void* addr, size_t size, size_t alignment, VkSystemAllocationScope scope)
	{
		const auto allocator = (Memory::Allocator*)userData;

		if (addr != nullptr)
		{
			allocator->deallocate(addr, 0);
		}
		return allocator->allocate(size, alignment);
	}

	static void VmaFreeCallback(void* userData, void* addr)
	{
		if (addr == nullptr) return;
		const auto allocator = (Memory::Allocator*)userData;
		allocator->deallocate(addr, 0);
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL _debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData) {
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			{
				SOUL_LOG_INFO("VkDebugUtils: %s", pCallbackData->pMessage);
				break;
			}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
			SOUL_LOG_WARN("VkDebugUtils: %s", pCallbackData->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			SOUL_LOG_ERROR("VkDebugUtils: %s", pCallbackData->pMessage);
			SOUL_PANIC(0, "Vulkan Error!");
			break;
		}
		default: {
			SOUL_LOG_INFO("VkDebugUtils: %s", pCallbackData->pMessage);
			break;
		}
		}
		return VK_FALSE;
	}

	void System::init(const Config &config) {
		SOUL_ASSERT_MAIN_THREAD();
		Runtime::ScopeAllocator<> scopeAllocator("GPU::System::Init");
		SOUL_MEMORY_ALLOCATOR_ZONE(&scopeAllocator);

		_db.currentFrame = 0;
		_db.frameCounter = 0;

		_db.buffers.add({});
		_db.shaderArgSetIDs.add({});
		_db.shaders.add({});
		_db.programs.add({});
		_db.pipelineStates.add({});
		_db.semaphores.reserve(1000);
		_db.semaphores.add({});
		_db.textures.add({});
		_db.descriptorSets.reserve(1024);
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

		VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
		appInfo.pApplicationName = "Soul Engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.pEngineName = "Soul Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		SOUL_LOG_INFO("Creating vulkan instance");
		VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		instanceCreateInfo.pApplicationInfo = &appInfo;

		static constexpr const char *requiredExtensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(SOUL_OS_WINDOWS)
			"VK_KHR_win32_surface",
#endif // SOUL_PLATFORM_OS_WIN32
#ifdef SOUL_OS_APPLE
				"VK_MVK_macos_surface",
#endif // SOUL_PLATFORM_OS_APPLE
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		static constexpr uint32_t requiredExtensionCount = sizeof(requiredExtensions) / sizeof(requiredExtensions[0]);
		instanceCreateInfo.enabledExtensionCount = requiredExtensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions;

#ifdef SOUL_VULKAN_VALIDATION_ENABLE
		static constexpr const char *REQUIRED_LAYERS[] = {
			"VK_LAYER_KHRONOS_validation",
		};

		static constexpr uint32_t requiredLayerCount = sizeof(REQUIRED_LAYERS) / sizeof(REQUIRED_LAYERS[0]);

		static constexpr auto checkLayerSupport = []() -> bool {
			SOUL_LOG_INFO("Check vulkan layer support.");
			uint32 layerCount;

			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			Array<VkLayerProperties> availableLayers;
			availableLayers.resize(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
			for (const char* requiredLayer : REQUIRED_LAYERS) {
				bool layerFound = false;
				for (int j = 0; j < layerCount; j++) {
					if (strcmp(requiredLayer, availableLayers[j].layerName) == 0) {
						layerFound = true;
						break;
					}
				}
				if (!layerFound) {
					SOUL_LOG_INFO("Validation layer %s not found!", requiredLayer);
					return false;
				}
			}
			availableLayers.cleanup();

			return true;
		};

		SOUL_ASSERT(0, checkLayerSupport(), "");
		instanceCreateInfo.enabledLayerCount = requiredLayerCount;
		instanceCreateInfo.ppEnabledLayerNames = REQUIRED_LAYERS;
#endif
		
		SOUL_VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &_db.instance),
		              "Vulkan instance creation fail!");
		SOUL_LOG_INFO("Vulkan instance creation succesful");

		volkLoadInstance(_db.instance);
		SOUL_LOG_INFO("Instance version = %d, %d", VK_VERSION_MAJOR(volkGetInstanceVersion()), VK_VERSION_MINOR(volkGetInstanceVersion()));

		static constexpr auto createDebugUtilsMessenger = [](Database *db) {
			SOUL_LOG_INFO("Creating vulkan debug utils messenger");
			VkDebugUtilsMessengerCreateInfoEXT
				debugMessengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
			debugMessengerCreateInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugMessengerCreateInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugMessengerCreateInfo.pfnUserCallback = _debugCallback;
			debugMessengerCreateInfo.pUserData = nullptr;
			SOUL_VK_CHECK(vkCreateDebugUtilsMessengerEXT(db->instance, &debugMessengerCreateInfo, nullptr,
				              &db->debugMessenger),
			              "Vulkan debug messenger creation fail!");
			SOUL_LOG_INFO("Vulkan debug messenger creation sucesfull");
		};
		createDebugUtilsMessenger(&_db);

		_surfaceCreate(config.windowHandle, &_db.surface);

		static constexpr auto
			pickPhysicalDevice = [](Database *db, int requiredExtensionCount,
			                        const char *requiredExtensions[])-> EnumArray<QueueType, uint32> {
				SOUL_LOG_INFO("Picking vulkan physical device.");
				db->physicalDevice = VK_NULL_HANDLE;
				uint32 deviceCount = 0;
				vkEnumeratePhysicalDevices(db->instance, &deviceCount, nullptr);
				SOUL_ASSERT(0, deviceCount > 0, "There is no device with vulkan support!");

				Soul::Array<VkPhysicalDevice> devices;
				devices.resize(deviceCount);
				vkEnumeratePhysicalDevices(db->instance, &deviceCount, devices.data());

				Array<VkSurfaceFormatKHR> formats;
				Array<VkPresentModeKHR> presentModes;

				db->physicalDevice = VK_NULL_HANDLE;
				int bestScore = -1;

				EnumArray<QueueType, uint32> queueFamilyIndices;

				for (int i = 0; i < devices.size(); i++) {

					VkPhysicalDeviceProperties physicalDeviceProperties;
					VkPhysicalDeviceFeatures physicalDeviceFeatures;

					vkGetPhysicalDeviceProperties(devices[i], &physicalDeviceProperties);
					vkGetPhysicalDeviceFeatures(devices[i], &physicalDeviceFeatures);


					const uint32_t apiVersion = physicalDeviceProperties.apiVersion;
					const uint32_t driverVersion = physicalDeviceProperties.driverVersion;
					const uint32_t vendorID = physicalDeviceProperties.vendorID;
					const uint32_t deviceID = physicalDeviceProperties.deviceID;

					SOUL_LOG_INFO("Devices %d\n"
					              " -- Name = %s\n"
					              " -- Vendor = 0x%.8X\n"
					              " -- Device ID = 0x%.8X\n"
					              " -- Api Version = %d.%d.%d\n"
					              " -- Driver Version = %d.%d.%d\n",
					              i, physicalDeviceProperties.deviceName, vendorID, 
					              deviceID, 
					              VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion),
					              VK_VERSION_MAJOR(driverVersion), VK_VERSION_MINOR(driverVersion), VK_VERSION_PATCH(driverVersion));

					uint32_t extensionCount;
					vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensionCount, nullptr);

					Soul::Array<VkExtensionProperties> availableExtensions;
					availableExtensions.resize(extensionCount);
					vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensionCount, availableExtensions.data());
					bool allExtensionFound = true;

					for (int j = 0; j < requiredExtensionCount; j++) {
						bool extensionFound = false;
						for (int k = 0; k < extensionCount; k++) {
							if (strcmp(availableExtensions[k].extensionName, requiredExtensions[j]) == 0) {
								extensionFound = true;
								break;
							}
						}
						if (!extensionFound) {
							SOUL_LOG_INFO(" -- Extension %s not found", requiredExtensions[j]);
							allExtensionFound = false;
							break;
						}
						SOUL_LOG_INFO(" -- Extension %s found", requiredExtensions[j]);
					}
					availableExtensions.cleanup();
					if (!allExtensionFound) {
						continue;
					}

					vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[i], db->surface, &db->surfaceCaps);

					uint32 formatCount;
					vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], db->surface, &formatCount, nullptr);
					SOUL_LOG_INFO(" -- Format count = %d", formatCount);
					if (formatCount == 0) {
						continue;
					}
					formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], db->surface, &formatCount, formats.data());

					uint32 presentModeCount;
					vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], db->surface, &presentModeCount, nullptr);
					SOUL_LOG_INFO(" -- Present mode count = %d", presentModeCount);
					if (presentModeCount == 0) {
						continue;
					}
					presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], db->surface, &presentModeCount,
					                                          presentModes.data());

					uint32 queueFamilyCount = 0;
					vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);

					Soul::Array<VkQueueFamilyProperties> queueFamilies;
					queueFamilies.resize(queueFamilyCount);
					vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies.data());

					int graphicsQueueFamilyIndex = -1;
					int presentQueueFamilyIndex = -1;
					int computeQueueFamilyIndex = -1;
					int transferQueueFamilyIndex = -1;

					// Try to find queue that support present, graphics and compute
					for (int j = 0; j < queueFamilyCount; j++) {
						VkBool32 presentSupport = false;
						vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, db->surface, &presentSupport);

						const VkQueueFamilyProperties &queueProps = queueFamilies[j];
						VkQueueFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
						if (presentSupport && queueProps.queueCount > 0 &&
							(queueProps.queueFlags & requiredFlags) == requiredFlags) {
							graphicsQueueFamilyIndex = j;
							presentQueueFamilyIndex = j;
						}
					}

					if (graphicsQueueFamilyIndex == -1) {
						for (int j = 0; j < queueFamilyCount; j++) {
							const VkQueueFamilyProperties &queueProps = queueFamilies[j];
							VkQueueFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
							if (queueProps.queueCount > 0 && (queueProps.queueFlags & requiredFlags) == requiredFlags) {
								graphicsQueueFamilyIndex = j;
								break;
							}
						}
					}

					for (int j = 0; j < queueFamilyCount; j++) {
						for (int j = 0; j < queueFamilyCount; j++) {
							const VkQueueFamilyProperties &queueProps = queueFamilies[j];
							VkQueueFlags requiredFlags = VK_QUEUE_COMPUTE_BIT;
							if (j != graphicsQueueFamilyIndex && queueProps.queueCount > 0
								&& (queueProps.queueFlags & requiredFlags) == requiredFlags) {
								computeQueueFamilyIndex = j;
								break;
							}
						}
					}
					if (computeQueueFamilyIndex == -1) computeQueueFamilyIndex = graphicsQueueFamilyIndex;

					for (int j = 0; j < queueFamilyCount; j++) {
						for (int j = 0; j < queueFamilyCount; j++) {
							const VkQueueFamilyProperties &queueProps = queueFamilies[j];
							VkQueueFlags requiredFlags = VK_QUEUE_TRANSFER_BIT;
							if (j != graphicsQueueFamilyIndex && j != computeQueueFamilyIndex
								&& (queueProps.queueFlags & requiredFlags) == requiredFlags) {
								transferQueueFamilyIndex = j;
								break;
							}
						}
					}
					if (transferQueueFamilyIndex == -1) transferQueueFamilyIndex = graphicsQueueFamilyIndex;

					if (presentQueueFamilyIndex == -1) {
						for (int j = 0; j < queueFamilyCount; j++) {
							VkBool32 presentSupport = false;
							vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, db->surface, &presentSupport);
							if (presentSupport) {
								presentQueueFamilyIndex = j;
								break;
							}
						}
					}

					queueFamilies.cleanup();

					if (graphicsQueueFamilyIndex == -1) {
						SOUL_LOG_INFO(" -- Graphics queue family not found");
						continue;
					}
					if (presentQueueFamilyIndex == -1) {
						SOUL_LOG_INFO(" -- Present queue family not found");
						continue;
					}
					if (computeQueueFamilyIndex == -1) {
						SOUL_LOG_INFO(" -- Compute queue family not found");
						continue;
					}
					if (transferQueueFamilyIndex == -1) {
						SOUL_LOG_INFO("-- Transfer queue family not found");
						continue;
					}

					int score = 0;
					if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
						score += 100;
					}

					SOUL_LOG_INFO("-- Score = %d", score);
					if (score > bestScore && presentQueueFamilyIndex == graphicsQueueFamilyIndex) {
						db->physicalDevice = devices[i];
						queueFamilyIndices[QueueType::GRAPHIC] = graphicsQueueFamilyIndex;
						queueFamilyIndices[QueueType::TRANSFER] = transferQueueFamilyIndex;
						queueFamilyIndices[QueueType::COMPUTE] = computeQueueFamilyIndex;
						bestScore = score;
					}
				}

				SOUL_ASSERT(0, db->physicalDevice != VK_NULL_HANDLE,
				            "Cannot find physical device that satisfy the requirements.");

				vkGetPhysicalDeviceProperties(db->physicalDevice, &db->physicalDeviceProperties);
				vkGetPhysicalDeviceFeatures(db->physicalDevice, &db->physicalDeviceFeatures);
			
				const uint32_t apiVersion = db->physicalDeviceProperties.apiVersion;
				const uint32_t driverVersion = db->physicalDeviceProperties.driverVersion;
				const uint32_t vendorID = db->physicalDeviceProperties.vendorID;
				const uint32_t deviceID = db->physicalDeviceProperties.deviceID;

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
				              vendorID,
				              deviceID,
				              apiVersion,
				              driverVersion,
				              queueFamilyIndices[QueueType::GRAPHIC],
				              queueFamilyIndices[QueueType::TRANSFER],
				              queueFamilyIndices[QueueType::COMPUTE]);

				formats.cleanup();
				presentModes.cleanup();
				devices.cleanup();

				SOUL_ASSERT(0, db->physicalDevice != VK_NULL_HANDLE, "Fail to find a suitable GPU!");
				return queueFamilyIndices;
			};
		static const char* deviceRequiredExtensions[] = {
			VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME,
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		static const uint32_t deviceRequiredExtensionCount = sizeof(deviceRequiredExtensions) / sizeof(deviceRequiredExtensions[0]);
		EnumArray<QueueType, uint32> queueFamilyIndices = pickPhysicalDevice(&_db, deviceRequiredExtensionCount, deviceRequiredExtensions);

		static constexpr auto createDeviceAndQueue = 
			[](VkPhysicalDevice physicalDevice, EnumArray<QueueType, uint32>& queueFamilyIndices)
				-> std::tuple<VkDevice, EnumArray<QueueType, uint32>> {
			SOUL_LOG_INFO("Creating vulkan logical device");

			int graphicsQueueCount = 1;
			EnumArray<QueueType, uint32> queueIndex(0);

			if (queueFamilyIndices[QueueType::COMPUTE] == queueFamilyIndices[QueueType::GRAPHIC]) {
				graphicsQueueCount++;
				queueIndex[QueueType::COMPUTE] = queueIndex[QueueType::GRAPHIC] + 1;
			}
			if (queueFamilyIndices[QueueType::TRANSFER] == queueFamilyIndices[QueueType::GRAPHIC]) {
				graphicsQueueCount++;
				queueIndex[QueueType::TRANSFER] = queueIndex[QueueType::COMPUTE] + 1;
			}

			VkDeviceQueueCreateInfo queueCreateInfo[4] = {};
			float priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};
			int queueCreateInfoCount = 1;

			queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo[0].queueFamilyIndex = queueFamilyIndices[QueueType::GRAPHIC];
			queueCreateInfo[0].queueCount = graphicsQueueCount;
			queueCreateInfo[0].pQueuePriorities = priorities;

			if (queueFamilyIndices[QueueType::GRAPHIC] != queueFamilyIndices[QueueType::COMPUTE]) {
				queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = queueFamilyIndices[QueueType::COMPUTE];
				queueCreateInfo[queueCreateInfoCount].queueCount = 1;
				queueCreateInfo[queueCreateInfoCount].pQueuePriorities = priorities + queueCreateInfoCount;
				queueCreateInfoCount++;
			}

			if (queueFamilyIndices[QueueType::TRANSFER] != queueFamilyIndices[QueueType::GRAPHIC]) {
				queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = queueFamilyIndices[QueueType::TRANSFER];
				queueCreateInfo[queueCreateInfoCount].queueCount = 1;
				queueCreateInfo[queueCreateInfoCount].pQueuePriorities = priorities + queueCreateInfoCount;
				queueCreateInfoCount++;
			}

			// TODO(kevinyu): This is temporary hack to avoid molten vk bug when queue family is not sorted in queueCreateInfo
			queueCreateInfo[3] = queueCreateInfo[0];
			queueCreateInfo[0] = queueCreateInfo[1];
			queueCreateInfo[1] = queueCreateInfo[2];
			queueCreateInfo[2] = queueCreateInfo[3];

			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceFeatures.geometryShader = VK_TRUE;
			deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
			deviceFeatures.fillModeNonSolid = VK_TRUE;

			VkPhysicalDeviceVulkanMemoryModelFeatures deviceVulkan12Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES };
			deviceVulkan12Features.vulkanMemoryModel = VK_TRUE;

			VkPhysicalDeviceFeatures2 deviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
			deviceFeatures2.pNext = &deviceVulkan12Features;
			deviceFeatures2.features = deviceFeatures;

			VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
			deviceCreateInfo.pNext = &deviceFeatures2;
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
			deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
			deviceCreateInfo.pEnabledFeatures = nullptr;

			constexpr uint32 deviceExtensionCount = deviceRequiredExtensionCount;

			deviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
			deviceCreateInfo.ppEnabledExtensionNames = deviceRequiredExtensions;

			VkDevice device;
			SOUL_VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device),
			              "Vulkan logical device creation fail!");
			SOUL_LOG_INFO("Vulkan logical device creation sucessful");
			return std::make_tuple(device, queueIndex);

		};
		auto [device, queueIndex] = createDeviceAndQueue(_db.physicalDevice, queueFamilyIndices);
		_db.device = device;
		for (const auto queueType : EnumIter<QueueType>::Iterates())
		{
			_db.queues[queueType].init(device, queueFamilyIndices[queueType], queueIndex[queueType]);
		}

		static const auto createSwapchain = [this, &queueFamilyIndices](Database *db, uint32 swapchainWidth, uint32 swapchainHeight) {

			static const auto
				pickSurfaceFormat = [](VkPhysicalDevice physicalDevice,
				                       VkSurfaceKHR surface) -> VkSurfaceFormatKHR {
					Runtime::ScopeAllocator<> scopeAllocator("GPU::System::init::pickSurfaceFormat");
					SOUL_LOG_INFO("Picking surface format.");
					Array<VkSurfaceFormatKHR> formats(&scopeAllocator);
					uint32 formatCount;
					vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
					SOUL_ASSERT(0, formatCount != 0, "Surface format count is zero!");
					formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

					for (int i = 0; i < formats.size(); i++) {
						const VkSurfaceFormatKHR surfaceFormat = formats[0];
						if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM
							&& surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
							formats.cleanup();
							return surfaceFormat;
						}
					}
					VkSurfaceFormatKHR format = formats[0];
					formats.cleanup();
					return format;
				};
			db->swapchain.format = pickSurfaceFormat(db->physicalDevice, db->surface);

			static constexpr auto pickSurfaceExtent =
				[](VkSurfaceCapabilitiesKHR capabilities, uint32 swapchainWidth,
				   uint32 swapchainHeight) -> VkExtent2D {
				SOUL_LOG_INFO("Picking vulkan swap extent");
				if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
					SOUL_LOG_INFO("vulkanPickSwapExtent | Extent = %d %d",
					              capabilities.currentExtent.width,
					              capabilities.currentExtent.height);
					return capabilities.currentExtent;
				}
				else {
					VkExtent2D actualExtent = {static_cast<uint32>(swapchainWidth),
						static_cast<uint32>(swapchainHeight)};

					actualExtent.width =
						std::max(capabilities.minImageExtent.width,
						         std::min(capabilities.maxImageExtent.width, actualExtent.width));
					actualExtent.height =
						std::max(capabilities.minImageExtent.height,
						         std::min(capabilities.maxImageExtent.height, actualExtent.height));

					return actualExtent;
				}
			};
			db->swapchain.extent = pickSurfaceExtent(db->surfaceCaps, swapchainWidth, swapchainHeight);

			uint32 imageCount = db->surfaceCaps.minImageCount + 1;
			if (db->surfaceCaps.maxImageCount > 0 && imageCount > db->surfaceCaps.maxImageCount) {
				imageCount = db->surfaceCaps.maxImageCount;
			}
			SOUL_LOG_INFO("Swapchain image count = %d", imageCount);

			VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
			swapchainInfo.surface = db->surface;
			swapchainInfo.minImageCount = imageCount;
			swapchainInfo.imageFormat = db->swapchain.format.format;
			swapchainInfo.imageColorSpace = db->swapchain.format.colorSpace;
			swapchainInfo.imageExtent = db->swapchain.extent;
			swapchainInfo.imageArrayLayers = 1;
			swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainInfo.queueFamilyIndexCount = 1;
			swapchainInfo.pQueueFamilyIndices = &queueFamilyIndices[QueueType::GRAPHIC];
			swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			swapchainInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainInfo.clipped = VK_TRUE;

			SOUL_VK_CHECK(vkCreateSwapchainKHR(db->device, &swapchainInfo, nullptr, &db->swapchain.vkHandle),
			              "Fail to create vulkan swapchain!");

			vkGetSwapchainImagesKHR(db->device, db->swapchain.vkHandle, &imageCount, nullptr);
			db->swapchain.images.resize(imageCount);
			vkGetSwapchainImagesKHR(db->device, db->swapchain.vkHandle, &imageCount, db->swapchain.images.data());

			db->swapchain.imageViews.resize(imageCount);
			for (int i = 0; i < imageCount; i++) {
				VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
				imageViewInfo.image = db->swapchain.images[i];
				imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewInfo.format = db->swapchain.format.format;
				imageViewInfo.components =
				{VK_COMPONENT_SWIZZLE_IDENTITY, 
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY};
				imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewInfo.subresourceRange.baseMipLevel = 0;
				imageViewInfo.subresourceRange.levelCount = 1;
				imageViewInfo.subresourceRange.baseArrayLayer = 0;
				imageViewInfo.subresourceRange.layerCount = 1;

				SOUL_VK_CHECK(vkCreateImageView(db->device, &imageViewInfo, nullptr, &db->swapchain.imageViews[i]),
				              "Fail to create swapchain imageview %d",
				              i);
			}

			db->swapchain.textures.resize(imageCount);
			for (int i = 0; i < imageCount; i++) {
				db->swapchain.textures[i] = TextureID(db->textures.add(Texture()));
				Texture &texture = *_texturePtr(db->swapchain.textures[i]);
				texture.vkHandle = db->swapchain.images[i];
				texture.view = db->swapchain.imageViews[i];
				texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
				texture.extent.width = db->swapchain.extent.width;
				texture.extent.height = db->swapchain.extent.height;
				// TODO: Is depth always 1?
				texture.extent.depth = 1;
				texture.sharingMode = swapchainInfo.imageSharingMode;
				// TODO: Fix the format
				texture.format = TextureFormat::BGRA8;
				// TODO: Fix the type
				texture.type = TextureType::D2;
				texture.owner = ResourceOwner::NONE;
			}

			db->swapchain.fences.resize(imageCount);
			for (int i = 0; i < imageCount; i++) {
				db->swapchain.fences[i] = VK_NULL_HANDLE;
			}
			SOUL_LOG_INFO("Vulkan swapchain creation sucessful");
		};
		createSwapchain(&_db, config.swapchainWidth, config.swapchainHeight);

		_frameContextInit(config);

		static constexpr auto initAllocator = [](Database *db) {
			VmaVulkanFunctions vulkanFunctions = {};
			vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
			vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
			vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
			vulkanFunctions.vkFreeMemory = vkFreeMemory;
			vulkanFunctions.vkMapMemory = vkMapMemory;
			vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
			vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
			vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
			vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
			vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
			vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
			vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
			vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
			vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
			vulkanFunctions.vkCreateImage = vkCreateImage;
			vulkanFunctions.vkDestroyImage = vkDestroyImage;
			vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
			vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
			vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
			vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
			vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
			vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;

			VkAllocationCallbacks allocationCallbacks = {
				&(db->vulkanCPUAllocator),
				VmaAllocCallback,
				VmaReallocationCallback,
				VmaFreeCallback,
				nullptr,
				nullptr
			};

			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = db->physicalDevice;
			allocatorInfo.device = db->device;
			allocatorInfo.instance = db->instance;
			allocatorInfo.preferredLargeHeapBlockSize = 0;
			allocatorInfo.pAllocationCallbacks = &allocationCallbacks;
			allocatorInfo.pDeviceMemoryCallbacks = nullptr;
			allocatorInfo.frameInUseCount = 0;
			allocatorInfo.pHeapSizeLimit = 0;
			allocatorInfo.pVulkanFunctions = &vulkanFunctions;

			vmaCreateAllocator(&allocatorInfo, &db->gpuAllocator);

			SOUL_LOG_INFO("Vulkan init allocator sucessful");
		};
		initAllocator(&_db);

		_frameBegin();
	}

	void System::_frameContextInit(const System::Config &config) {
		SOUL_ASSERT_MAIN_THREAD();

		SOUL_LOG_INFO("Frame Context Init");
		_db.frameContexts.reserve(config.maxFrameInFlight);
		for (int i = 0; i < _db.frameContexts.capacity(); i++) {
			_db.frameContexts.add(_FrameContext(&_db.cpuAllocator));
			_FrameContext &frameContext = _db.frameContexts[i];

			frameContext.commandPools.init(_db.device, _db.queues, config.threadCount);

			VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			SOUL_VK_CHECK(vkCreateFence(_db.device, &fenceInfo, nullptr, &frameContext.fence), "");

			VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
			frameContext.imageAvailableSemaphore = _semaphoreCreate();
			frameContext.renderFinishedSemaphore = _semaphoreCreate();

			VkDescriptorPoolSize poolSizes[2] = {};

			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			poolSizes[0].descriptorCount = 100;

			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = 200;

			VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
			poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
			poolInfo.pPoolSizes = poolSizes;
			poolInfo.maxSets = 100;

			SOUL_VK_CHECK(vkCreateDescriptorPool(_db.device, &poolInfo, nullptr, &frameContext.descriptorPool), "");
		}
	}

	QueueData System::_getQueueDataFromQueueFlags(QueueFlags flags) {
		QueueData queueData;
		
		static_assert(QUEUE_GRAPHIC_BIT == 1 << uint64(QueueType::GRAPHIC));
		static_assert(QUEUE_COMPUTE_BIT == 1 << uint64(QueueType::COMPUTE));
		static_assert(QUEUE_TRANSFER_BIT == 1 << uint64(QueueType::TRANSFER));

		const auto& queues = _db.queues;

		Util::ForEachBit(flags, [&queueData, queues](uint32 bit) {
			queueData.indices[queueData.count++] = queues[QueueType(bit)].getFamilyIndex();
		});
		return queueData;
	}

	TextureID System::textureCreate(const TextureDesc &desc) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		TextureID textureID = TextureID(_db.textures.add(Texture()));
		Texture &texture = *_texturePtr(textureID);

		VkFormat format = vkCast(desc.format);

		VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		imageInfo.imageType = vkCast(desc.type);
		imageInfo.format = format;
		imageInfo.extent = {desc.width, desc.height, desc.depth};
		imageInfo.mipLevels = desc.mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = vkCastImageUsageFlags(desc.usageFlags);
		QueueData queueData = _getQueueDataFromQueueFlags(desc.queueFlags);
		imageInfo.sharingMode = queueData.count == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		imageInfo.queueFamilyIndexCount = queueData.count;
		imageInfo.pQueueFamilyIndices = queueData.indices;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		SOUL_VK_CHECK(vmaCreateImage(_db.gpuAllocator,
			              &imageInfo, &allocInfo, &texture.vkHandle,
			              &texture.allocation, nullptr), "");

		VkImageAspectFlags imageAspect = vkCastFormatToAspectFlags(desc.format);
		if (imageAspect & VK_IMAGE_ASPECT_STENCIL_BIT)
		{
			SOUL_LOG_WARN("Texture creation with stencil format detected. Current version will remove the aspect stencil bit so the texture cannot be used for depth stencil. The reason is because Vulkan spec stated that descriptor cannot have more than one aspect.");
			imageAspect &= ~(VK_IMAGE_ASPECT_STENCIL_BIT);
		}
		
		VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewInfo.image = texture.vkHandle;
		imageViewInfo.viewType = vkCastToImageViewType(desc.type);
		imageViewInfo.format = format;
		imageViewInfo.components = {};
		imageViewInfo.subresourceRange = {
			imageAspect,
			0,
			VK_REMAINING_MIP_LEVELS,
			0,
			1
		};
		SOUL_VK_CHECK(vkCreateImageView(_db.device,
			              &imageViewInfo, nullptr, &texture.view), "Create Image View fail");
		
		texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		texture.extent = imageInfo.extent;
		texture.sharingMode = imageInfo.sharingMode;
		texture.format = desc.format;
		texture.type = desc.type;
		texture.owner = ResourceOwner::NONE;
		texture.mipCount = desc.mipLevels;
		texture.mipViews = nullptr;


		if (desc.name != nullptr) {
			char texName[1024];
			sprintf(texName, "%s(%d)", desc.name, _db.frameCounter);
			VkDebugUtilsObjectNameInfoEXT imageNameInfo = {
				VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, // sType
				nullptr,                                               // pNext
				VK_OBJECT_TYPE_IMAGE,                               // objectType
				(uint64_t) texture.vkHandle,                         // object
				texName,                            // pObjectName
			};

			vkSetDebugUtilsObjectNameEXT(_db.device, &imageNameInfo);
		}
		return textureID;
	}

	TextureID System::textureCreate(const TextureDesc &desc, const byte *data, uint32 dataSize) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, data != nullptr, "");
		SOUL_ASSERT(0, dataSize != 0, "");
		TextureDesc newDesc = desc;
		if (data != nullptr) {
			SOUL_ASSERT(0, dataSize != 0, "");
			newDesc.usageFlags |= TEXTURE_USAGE_TRANSFER_DST_BIT;
			newDesc.queueFlags |= QUEUE_TRANSFER_BIT;
			if (desc.mipLevels > 1) {
				newDesc.usageFlags |= TEXTURE_USAGE_TRANSFER_SRC_BIT;
			}
		}

		TextureID textureID = textureCreate(newDesc);
		Texture &texture = *_texturePtr(textureID);

		VkImageMemoryBarrier beforeTransferBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		beforeTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		beforeTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		beforeTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beforeTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beforeTransferBarrier.image = texture.vkHandle;
		beforeTransferBarrier.subresourceRange.aspectMask = vkCastFormatToAspectFlags(desc.format);
		beforeTransferBarrier.subresourceRange.baseMipLevel = 0;
		beforeTransferBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		beforeTransferBarrier.subresourceRange.baseArrayLayer = 0;
		beforeTransferBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		beforeTransferBarrier.srcAccessMask = 0;
		beforeTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		if (!_frameContext().stagingAvailable) _stagingSetup();

		const char* texName = "Unknown Texture";
		if (desc.name != nullptr) {
			texName = desc.name;
		}

		Vec4f color = { (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, 1.0f };
		const VkDebugUtilsLabelEXT passLabel = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
			nullptr,                                  // pNext
			texName,                           // pLabelName
			{color.x, color.y, color.z, color.w},             // color
		};
		vkCmdBeginDebugUtilsLabelEXT(_frameContext().stagingCommandBuffer, &passLabel);


		vkCmdPipelineBarrier(
			_frameContext().stagingCommandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &beforeTransferBarrier);

		_stagingTransferToTexture(dataSize, data, desc, textureID);

		vkCmdEndDebugUtilsLabelEXT(_frameContext().stagingCommandBuffer);

		texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		texture.owner = ResourceOwner::TRANSFER_QUEUE;

		if (data != nullptr && desc.mipLevels > 1) {
			_generateMipmaps(textureID);
		}

		return textureID;
	}

	TextureID System::textureCreate(const TextureDesc& desc, ClearValue clearValue) {
		TextureDesc newDesc = desc;
		newDesc.usageFlags |= TEXTURE_USAGE_TRANSFER_DST_BIT;
		newDesc.queueFlags |= QUEUE_GRAPHIC_BIT;

		TextureID textureID = textureCreate(newDesc);
		Texture &texture = *_texturePtr(textureID);

		VkImageMemoryBarrier beforeTransferBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		beforeTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		beforeTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		beforeTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beforeTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beforeTransferBarrier.image = texture.vkHandle;
		beforeTransferBarrier.subresourceRange.aspectMask = vkCastFormatToAspectFlags(desc.format);
		beforeTransferBarrier.subresourceRange.baseMipLevel = 0;
		beforeTransferBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		beforeTransferBarrier.subresourceRange.baseArrayLayer = 0;
		beforeTransferBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		beforeTransferBarrier.srcAccessMask = 0;
		beforeTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		if (!_frameContext().stagingAvailable) _stagingSetup();
		vkCmdPipelineBarrier(
			_frameContext().clearCommandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &beforeTransferBarrier);

		VkImageSubresourceRange range;
		range.baseMipLevel = 0;
		range.levelCount = desc.mipLevels;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		range.aspectMask = vkCastFormatToAspectFlags(desc.format);
		if (range.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
			VkClearDepthStencilValue  vkClearValue;
			vkClearValue = { clearValue.depthStencil.depth, clearValue.depthStencil.stencil };
			SOUL_ASSERT(0, !(range.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT), "");
			vkCmdClearDepthStencilImage(
				_frameContext().clearCommandBuffer,
				texture.vkHandle,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&vkClearValue,
				1,
				&range);
		} else {

			VkClearColorValue vkClearValue;
			memcpy(&vkClearValue, &clearValue, sizeof(vkClearValue));
			vkCmdClearColorImage(
				_frameContext().clearCommandBuffer,
				texture.vkHandle,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&vkClearValue,
				1,
				&range);
		}
		texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		texture.owner = ResourceOwner::GRAPHIC_QUEUE;

		return textureID;
	}

	void System::_generateMipmaps(TextureID textureID) {
		Texture& texture = *_texturePtr(textureID);

		SOUL_ASSERT(0, texture.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, "");
		SOUL_ASSERT(0, texture.owner == ResourceOwner::TRANSFER_QUEUE, "");
		SOUL_ASSERT(0, texture.mipCount > 1, "");

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = texture.vkHandle;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int mipWidth = texture.extent.width;
		int mipHeight = texture.extent.height;

		VkCommandBuffer commandBuffer = _frameContext().genMipmapCommandBuffer;

		for (int i = 1; i < texture.mipCount; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
			                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			                     0, nullptr,
			                     0, nullptr,
			                     1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
			               texture.vkHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			               texture.vkHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			               1, &blit,
			               VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
			                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			                     0, nullptr,
			                     0, nullptr,
			                     1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = texture.mipCount - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                     0, nullptr,
		                     0, nullptr,
		                     1, &barrier);

		texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texture.owner = ResourceOwner::GRAPHIC_QUEUE;

	}

	TextureID System::_textureExportCreate(TextureID srcTextureID) {

		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		TextureID textureID = TextureID(_db.textures.add(Texture()));
		Texture& texture = *_texturePtr(textureID);

		Texture* srcTexture = _texturePtr(srcTextureID);

		VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = vkCast(srcTexture->type);
		imageInfo.format = vkCast(srcTexture->format);
		imageInfo.extent = srcTexture->extent;
		imageInfo.mipLevels = srcTexture->mipCount;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;;
		QueueData queueData = _getQueueDataFromQueueFlags(QUEUE_GRAPHIC_BIT);
		imageInfo.sharingMode = queueData.count == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		imageInfo.queueFamilyIndexCount = queueData.count;
		imageInfo.pQueueFamilyIndices = queueData.indices;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		SOUL_VK_CHECK(vmaCreateImage(_db.gpuAllocator,
			              &imageInfo, &allocInfo, &texture.vkHandle,
			              &texture.allocation, nullptr), "");

		VkImageAspectFlags imageAspect = vkCastFormatToAspectFlags(srcTexture->format);
		if (imageAspect & VK_IMAGE_ASPECT_STENCIL_BIT)
		{
			SOUL_LOG_WARN("Texture creation with stencil format detected. Current version will remove the aspect stencil bit so the texture cannot be used for depth stencil. The reason is because Vulkan spec stated that descriptor cannot have more than one aspect.");
			imageAspect &= ~(VK_IMAGE_ASPECT_STENCIL_BIT);
		}

		texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		texture.extent = srcTexture->extent;
		texture.sharingMode = imageInfo.sharingMode;
		texture.format = srcTexture->format;
		texture.type = srcTexture->type;
		texture.owner = ResourceOwner::NONE;
		texture.mipCount = srcTexture->mipCount;
		texture.mipViews = nullptr;

		char texName[1024];
		sprintf(texName, "Export Tex(%d)", _db.frameCounter);

		VkDebugUtilsObjectNameInfoEXT imageNameInfo = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, // sType
			nullptr,                                               // pNext
			VK_OBJECT_TYPE_IMAGE,                               // objectType
			(uint64_t)texture.vkHandle,                         // object
			texName,                            // pObjectName
		};

		vkSetDebugUtilsObjectNameEXT(_db.device, &imageNameInfo);

		return textureID;
	}

	void System::textureDestroy(TextureID id) {
		SOUL_ASSERT_MAIN_THREAD();
		_frameContext().garbages.textures.add(id);
	}

	Texture *System::_texturePtr(TextureID textureID) {
		return &_db.textures[textureID.id];
	}

	const Texture& System::_textureRef(TextureID textureID) {
		return _db.textures[textureID.id];
	}

	VkImageView System::_textureGetMipView(TextureID textureID, uint32 level) {
		Texture* texture = _texturePtr(textureID);
		SOUL_ASSERT(0, level < texture->mipCount, "");
		if (texture->mipCount == 1) return texture->view;
		if (texture->mipViews == nullptr) {
			texture->mipViews = (VkImageView*) _db.cpuAllocator.allocate(sizeof(VkImageView) * texture->mipCount, alignof(VkImageView));
			for (int i = 0; i < texture->mipCount; i++) {
				texture->mipViews[i] = VK_NULL_HANDLE;
			}
		}
		if (texture->mipViews[level] == VK_NULL_HANDLE) {
			VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			imageViewInfo.image = texture->vkHandle;
			imageViewInfo.viewType = vkCastToImageViewType(texture->type);
			imageViewInfo.format = vkCast(texture->format);
			imageViewInfo.components = {};
			imageViewInfo.subresourceRange = {
				vkCastFormatToAspectFlags(texture->format),
				level,
				1,
				0,
				1
			};
			VkImageView imageView;
			SOUL_VK_CHECK(vkCreateImageView(_db.device,
				              &imageViewInfo, nullptr, &imageView), "Create Image View fail");
			texture->mipViews[level] = imageView;
		}
		return texture->mipViews[level];
	}

	void System::_stagingFrameBegin() {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();
		_FrameContext &frameContext = _frameContext();
		for (const Buffer &buffer : frameContext.stagingBuffers) {
			vmaDestroyBuffer(_db.gpuAllocator, buffer.vkHandle, buffer.allocation);
		}
		_frameContext().stagingBuffers.resize(0);
	}

	void System::_stagingFrameEnd() {
		if (!_frameContext().stagingSynced) _stagingFlush();
	}

	void System::_stagingSetup() {
		SOUL_ASSERT_MAIN_THREAD();

		_FrameContext &frameContext = _frameContext();
		frameContext.stagingCommandBuffer = frameContext.commandPools.requestCommandBuffer(QueueType::TRANSFER);
		frameContext.clearCommandBuffer = frameContext.commandPools.requestCommandBuffer(QueueType::GRAPHIC);
		frameContext.genMipmapCommandBuffer = frameContext.commandPools.requestCommandBuffer(QueueType::GRAPHIC);
		frameContext.stagingAvailable = true;
		frameContext.stagingSynced = false;
	}

	void System::_stagingFlush() {
		SOUL_ASSERT_MAIN_THREAD();
		_FrameContext &frameContext = _frameContext();

		SemaphoreID mipmapSemaphore = _semaphoreCreate();
		Semaphore* mipmapSemaphorePtr = _semaphorePtr(mipmapSemaphore);
		_db.queues[QueueType::TRANSFER].submit(frameContext.stagingCommandBuffer, 1, &mipmapSemaphorePtr);
		_db.queues[QueueType::GRAPHIC].submit(frameContext.clearCommandBuffer);
		_db.queues[QueueType::GRAPHIC].wait(mipmapSemaphorePtr, VK_PIPELINE_STAGE_TRANSFER_BIT);
		_db.queues[QueueType::GRAPHIC].submit(frameContext.genMipmapCommandBuffer);
		_semaphoreDestroy(mipmapSemaphore);

		frameContext.stagingCommandBuffer = VK_NULL_HANDLE;
		frameContext.clearCommandBuffer = VK_NULL_HANDLE;
		frameContext.stagingAvailable = false;
		frameContext.stagingSynced = true;
	}

	void System::_transferBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t size) {
		SOUL_ASSERT_MAIN_THREAD();

		if (!_frameContext().stagingAvailable) _stagingSetup();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;

		const char* name = "Unknown texture";
		Vec4f color = { (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, 1.0f };
		const VkDebugUtilsLabelEXT passLabel = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
			nullptr,                                  // pNext
			name,                           // pLabelName
			{color.x, color.y, color.z, color.w},             // color
		};
		vkCmdBeginDebugUtilsLabelEXT(_frameContext().stagingCommandBuffer, &passLabel);
		vkCmdCopyBuffer(_frameContext().stagingCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkCmdEndDebugUtilsLabelEXT(_frameContext().stagingCommandBuffer);
	}

	Buffer System::_stagingBufferRequest(const byte *data, uint32 size) {
		SOUL_ASSERT_MAIN_THREAD();

		SOUL_ASSERT(0, data != nullptr, "");
		Buffer stagingBuffer = {};
		VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		SOUL_VK_CHECK(vmaCreateBuffer(_db.gpuAllocator,
			              &bufferInfo,
			              &allocInfo,
			              &stagingBuffer.vkHandle,
			              &stagingBuffer.allocation,
			              nullptr), "");
		
		_frameContext().stagingBuffers.add(stagingBuffer);

		void *mappedData;
		vmaMapMemory(_db.gpuAllocator, stagingBuffer.allocation, &mappedData);
		memcpy(mappedData, data, size);
		vmaUnmapMemory(_db.gpuAllocator, stagingBuffer.allocation);

		return stagingBuffer;
	}

	void System::_stagingTransferToBuffer(const byte *data, uint32 size, BufferID bufferID) {
		SOUL_ASSERT_MAIN_THREAD();

		Buffer stagingBuffer = _stagingBufferRequest(data, size);
		_transferBufferToBuffer(stagingBuffer.vkHandle, _bufferPtr(bufferID)->vkHandle, size);
	}

	void System::_stagingTransferToTexture(uint32 size, const byte *data, const TextureDesc &desc, TextureID textureID) {
		SOUL_ASSERT_MAIN_THREAD();

		Buffer stagingBuffer = _stagingBufferRequest(data, size);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset.x = 0;
		copyRegion.imageOffset.y = 0;
		copyRegion.imageOffset.z = 0;
		copyRegion.imageExtent.width = desc.width;
		copyRegion.imageExtent.height = desc.height;
		copyRegion.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(_frameContext().stagingCommandBuffer,
		                       stagingBuffer.vkHandle,
		                       _texturePtr(textureID)->vkHandle,
		                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                       1,
		                       &copyRegion);
	}

	BufferID System::bufferCreate(const BufferDesc &desc) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, desc.count > 0, "");
		SOUL_ASSERT(0, desc.typeSize > 0, "");
		SOUL_ASSERT(0, desc.typeAlignment > 0, "");
		SOUL_ASSERT(0, desc.usageFlags != 0, "");

		BufferID bufferID = BufferID(_db.buffers.add({}));
		Buffer &buffer = *_bufferPtr(bufferID);
		

		QueueData queueData = _getQueueDataFromQueueFlags(desc.queueFlags);

		SOUL_ASSERT(0, queueData.count > 0, "");

		uint64 alignment = desc.typeSize;

		if (desc.usageFlags & BUFFER_USAGE_UNIFORM_BIT) {
			size_t minUboAlignment = _db.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
			SOUL_ASSERT(0, isPowerOfTwo(minUboAlignment), "");
			size_t dynamicAlignment = (desc.typeSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
			alignment = std::max(alignment, dynamicAlignment);
		}

		if (desc.usageFlags & BUFFER_USAGE_STORAGE_BIT) {
			size_t minSsboAlignment = _db.physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
			SOUL_ASSERT(0, isPowerOfTwo(minSsboAlignment), "");
			size_t dynamicAlignment = (desc.typeSize + minSsboAlignment - 1) & ~(minSsboAlignment - 1);
			alignment = std::max(alignment, dynamicAlignment);
		}

		VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		bufferInfo.size = alignment * desc.count;
		bufferInfo.usage = vkCastBufferUsageFlags(desc.usageFlags);
		bufferInfo.sharingMode = queueData.count > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.queueFamilyIndexCount = queueData.count;
		bufferInfo.pQueueFamilyIndices = queueData.indices;
		
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		SOUL_VK_CHECK(
			vmaCreateBuffer(_db.gpuAllocator, &bufferInfo, &allocInfo, &buffer.vkHandle, &buffer.allocation,
				nullptr),
			"");

		buffer.unitCount = desc.count;
		buffer.unitSize = alignment;
		buffer.usageFlags = desc.usageFlags;
		buffer.queueFlags = desc.queueFlags;
		buffer.owner = ResourceOwner::NONE;

		return bufferID;
	}

	void System::bufferDestroy(BufferID id) {
		SOUL_ASSERT_MAIN_THREAD();
		_frameContext().garbages.buffers.add(id);
	}

	Buffer *System::_bufferPtr(BufferID bufferID) {
		return &_db.buffers[bufferID.id];
	}

	const Buffer& System::_bufferRef(BufferID bufferID) {
		SOUL_ASSERT(!bufferID.isNull(), "");
		return _db.buffers[bufferID.id];
	}

	_FrameContext &System::_frameContext() {
		return _db.frameContexts[_db.currentFrame % _db.frameContexts.size()];
	}

	static std::string _FormatGLSLErrorMessage(const char* source, const std::string& errorMessage) {
		std::string formattedErrorMessage;

		Soul::Array<std::string> sourceLines;
		sourceLines.add(std::string(""));
		std::stringstream sourceStream(source);
		for (std::string line; std::getline(sourceStream, line, '\n');)
			sourceLines.add(line);

		static constexpr auto _getSurroundingLine = [](const std::string& filename, uint64 line, const std::string& sourceLine) -> std::string {
			std::string result("");
			result += "        ";
			result += filename;
			result += ":";
			result += std::to_string(line);
			result += ": ";
			result += sourceLine;
			result += '\n';
			return result;
		};

		std::stringstream errorStream(errorMessage);
		for (std::string line; std::getline(errorStream, line);) {
			uint64 filenameEndPos = line.find(':');
			std::string filename = line.substr(0, filenameEndPos);
			uint64 sourceLineEndPos = line.find(':', filenameEndPos);
			uint64 sourceLine = std::stoull(line.substr(filenameEndPos + 1, sourceLineEndPos));
			
			for (uint64 beforeLine = std::max<int64>(0, int64(sourceLine) - 5); beforeLine < sourceLine; beforeLine++) {
				formattedErrorMessage += _getSurroundingLine(filename, beforeLine, sourceLines[beforeLine]);
			}

			formattedErrorMessage += " >>>>>> ";
			formattedErrorMessage += filename;
			formattedErrorMessage += ":";
			formattedErrorMessage += std::to_string(sourceLine);
			formattedErrorMessage += ": ";
			formattedErrorMessage += sourceLines[sourceLine];
			formattedErrorMessage += '\n';

			for (uint64 afterLine = sourceLine + 1; afterLine < sourceLine + 5 && afterLine < sourceLines.size(); afterLine++) {
				formattedErrorMessage += _getSurroundingLine(filename, afterLine, sourceLines[afterLine]);
			}

			formattedErrorMessage += '\n';
			formattedErrorMessage += line;
			formattedErrorMessage += '\n';
			formattedErrorMessage += "====================================================";
			formattedErrorMessage += '\n';

		}
		
		return formattedErrorMessage;
	}

	ShaderID System::shaderCreate(const ShaderDesc &desc, ShaderStage stage) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT(0, desc.sourceSize > 0, "");
		SOUL_ASSERT(0, desc.source != nullptr, "");
		SOUL_ASSERT(0, desc.name != nullptr, "");
		SOUL_ASSERT(0, strlen(desc.name) != 0, "Shader name cannot be empty");

		Runtime::ScopeAllocator<> scopeAllocator("shaderCreate");
		SOUL_MEMORY_ALLOCATOR_ZONE(&scopeAllocator);

		Shader shader;

		shaderc::Compiler glslCompiler;

		uint32 shadercShaderStageMap[uint32(ShaderStage::COUNT)] = {
			0,
			shaderc_shader_kind::shaderc_glsl_vertex_shader,
			shaderc_shader_kind::shaderc_glsl_geometry_shader,
			shaderc_shader_kind::shaderc_glsl_fragment_shader,
			shaderc_shader_kind::shaderc_glsl_compute_shader
		};

		shaderc::CompileOptions options;
		options.SetTargetSpirv(shaderc_spirv_version_1_3);
		shaderc::SpvCompilationResult glslCompileResult = glslCompiler.CompileGlslToSpv(
			desc.source,
			desc.sourceSize,
			(shaderc_shader_kind) shadercShaderStageMap[(uint32) stage],
			desc.name,
			options);

		std::string errorMessage = glslCompileResult.GetErrorMessage();

		static constexpr const char* _STAGE_NAMES[] = {
			"NONE",
			"VERTEX",
			"GEOMETRY",
			"FRAGMENT",
			"COMPUTE"
		};
		static constexpr EnumArray<ShaderStage, const char*> STAGE_NAMES(_STAGE_NAMES);

		SOUL_ASSERT(0, glslCompileResult.GetCompilationStatus() == shaderc_compilation_status_success,
		            "Fail when compiling pass : %d, shader_type = %s.\n"
		            "Error : \n%s\n", desc.name, STAGE_NAMES[stage], _FormatGLSLErrorMessage(desc.source, errorMessage).c_str());

		VkShaderModuleCreateInfo moduleInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		const uint32 *spirvCode = glslCompileResult.begin();
		size_t spirvCodeSize = glslCompileResult.end() - spirvCode;
		moduleInfo.pCode = spirvCode;
		moduleInfo.codeSize = spirvCodeSize * sizeof(uint32);

		SOUL_VK_CHECK(vkCreateShaderModule(_db.device, &moduleInfo, nullptr, &shader.module),
		              "Fail creating vertex shader module");

		spirv_cross::Compiler spirvCompiler(spirvCode, spirvCodeSize);
		spirv_cross::ShaderResources resources = spirvCompiler.get_shader_resources();

		for (spirv_cross::Resource &resource : resources.sampled_images) {
			unsigned set = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			unsigned binding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);
			const spirv_cross::SPIRType &type = spirvCompiler.get_type(resource.type_id);
			shader.bindings[set][binding].type = DescriptorType::SAMPLED_IMAGE;
			if (type.array.empty()) {
				shader.bindings[set][binding].count = 1;
			}
			else {
				SOUL_ASSERT(0, type.array.size() == 1, "Array must be one dimensional");
				SOUL_ASSERT(0, type.array_size_literal.front(), "Array size must be literal");
				shader.bindings[set][binding].count = type.array[0];
			}
		}

		for (spirv_cross::Resource &resource : resources.subpass_inputs) {
			unsigned set = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			unsigned binding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);
			unsigned attachmentIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);

			const spirv_cross::SPIRType &type = spirvCompiler.get_type(resource.type_id);
			shader.bindings[set][binding].type = DescriptorType::INPUT_ATTACHMENT;
			shader.bindings[set][binding].attachmentIndex = attachmentIndex;
			if (type.array.empty()) {
				shader.bindings[set][binding].count = 1;
			}
			else {
				SOUL_ASSERT(0, type.array.size() == 1, "Array must be one dimensional");
				SOUL_ASSERT(0, type.array_size_literal.front(), "Array size must be literal");
				shader.bindings[set][binding].count = type.array[0];
			}
		}

		for (spirv_cross::Resource &resource : resources.storage_images) {
			unsigned set = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			unsigned binding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);

			const spirv_cross::SPIRType &type = spirvCompiler.get_type(resource.type_id);
			shader.bindings[set][binding].type = DescriptorType::STORAGE_IMAGE;
			if (type.array.empty()) {
				shader.bindings[set][binding].count = 1;
			}
			else {
				SOUL_ASSERT(0, type.array.size() == 1, "Array must be one dimensional");
				SOUL_ASSERT(0, type.array_size_literal.front(), "Array size must be literal");
				shader.bindings[set][binding].count = type.array[0];
			}
		}

		SOUL_ASSERT(0, resources.separate_images.empty(), "texture<Dimension> is not supported yet!");
		SOUL_ASSERT(0, resources.push_constant_buffers.empty(), "push_constant is not supported yet!");
		SOUL_ASSERT(0, resources.storage_buffers.empty(), "SSBO is not supported yet!");

		for (spirv_cross::Resource &resource : resources.uniform_buffers) {
			unsigned set = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			unsigned binding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);
			const spirv_cross::SPIRType &type = spirvCompiler.get_type(resource.type_id);
			shader.bindings[set][binding].type = DescriptorType::UNIFORM_BUFFER;
			if (type.array.empty()) {
				shader.bindings[set][binding].count = 1;
			}
			else {
				SOUL_ASSERT(0, type.array.size() == 1, "Array must be one dimensional");
				SOUL_ASSERT(0, type.array_size_literal.front(), "Array size must be literal");
				shader.bindings[set][binding].count = type.array[0];
			}
		}

		SOUL_ASSERT(0, resources.stage_inputs.size() <= MAX_INPUT_PER_SHADER, "");

		if (stage != ShaderStage::VERTEX) {
			return ShaderID(_db.shaders.add(shader));
		}

		struct AttrExtraInfo {
			uint8 location;
			uint8 alignment;
			uint8 size;
		};

		Array<AttrExtraInfo> attrExtraInfos;
		attrExtraInfos.resize(MAX_INPUT_PER_SHADER);

		for (int i = 0; i < resources.stage_inputs.size(); i++) {
			spirv_cross::Resource &resource = resources.stage_inputs[i];
			unsigned location = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
			const spirv_cross::SPIRType &type = spirvCompiler.get_type(resource.base_type_id);

			AttrExtraInfo &extraInfo = attrExtraInfos[location];
			extraInfo.location = location;

			SOUL_ASSERT(0, type.columns <= 1, "");

			VkFormat floatVecSizeToVkFormat[4] = {
				VK_FORMAT_R32_SFLOAT,
				VK_FORMAT_R32G32_SFLOAT,
				VK_FORMAT_R32G32B32_SFLOAT,
				VK_FORMAT_R32G32B32A32_SFLOAT
			};
			VkFormat doubleVecSizeToVkFormat[4] = {
				VK_FORMAT_R64_SFLOAT,
				VK_FORMAT_R64G64_SFLOAT,
				VK_FORMAT_R64G64B64_SFLOAT,
				VK_FORMAT_R64G64B64A64_SFLOAT
			};
			VkFormat intVecSizeToVkFormat[4] = {
				VK_FORMAT_R32_SINT,
				VK_FORMAT_R32G32_SINT,
				VK_FORMAT_R32G32B32_SINT,
				VK_FORMAT_R32G32B32A32_SINT
			};
			VkFormat uintVecSizeToVkFormat[4] = {
				VK_FORMAT_R32_UINT,
				VK_FORMAT_R32G32_UINT,
				VK_FORMAT_R32G32B32_UINT,
				VK_FORMAT_R32G32B32A32_UINT
			};

			VkFormat *vecSizeToVkFormat = nullptr;
			switch (type.basetype) {
			case spirv_cross::SPIRType::Float:
				vecSizeToVkFormat = floatVecSizeToVkFormat;
				extraInfo.alignment = alignof(float);
				extraInfo.size = sizeof(float) * type.vecsize;
				break;
			case spirv_cross::SPIRType::UInt:
				vecSizeToVkFormat = uintVecSizeToVkFormat;
				extraInfo.alignment = alignof(uint32);
				extraInfo.size = sizeof(uint32) * type.vecsize;
				break;
			case spirv_cross::SPIRType::Int:
				vecSizeToVkFormat = intVecSizeToVkFormat;
				extraInfo.alignment = alignof(int32);
				extraInfo.size = sizeof(int32) * type.vecsize;
				break;
			case spirv_cross::SPIRType::Double:
				vecSizeToVkFormat = doubleVecSizeToVkFormat;
				extraInfo.alignment = alignof(double);
				extraInfo.size = sizeof(double) * type.vecsize;
				break;
			default:
				SOUL_PANIC(0, "Basetype %d is not supported", type.basetype);
			}
			shader.inputs[location].format = vecSizeToVkFormat[type.vecsize - 1];
		}

		uint32 currentOffset = 0;
		uint32 vertexAlignment = 0;
		for (int i = 0; i < resources.stage_inputs.size(); i++) {
			int location = attrExtraInfos[i].location;
			uint32 alignmentMinusOne = attrExtraInfos[i].alignment - 1;
			shader.inputs[location].offset = (currentOffset + alignmentMinusOne) & ~alignmentMinusOne;
			currentOffset += attrExtraInfos[i].size;
			vertexAlignment = std::max<uint32>(vertexAlignment, attrExtraInfos[i].alignment);
		}

		uint32 vertexSize = (currentOffset + vertexAlignment - 1) & ~(vertexAlignment - 1);
		shader.inputStride = vertexSize;

		attrExtraInfos.cleanup();

		return ShaderID(_db.shaders.add(shader));
	}

	void System::shaderDestroy(ShaderID shaderID) {
		_frameContext().garbages.shaders.add(shaderID);
	}

	Shader *System::_shaderPtr(ShaderID shaderID) {
		return &_db.shaders[shaderID.id];
	}

	VkDescriptorSetLayout System::_descriptorSetLayoutRequest(const DescriptorSetLayoutKey& key) {
		SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::_descriptorSetLayoutRequest");
		Runtime::ScopeAllocator<> scopeAllocator("GPU::System::_descriptorSetLayoutRequest");

		if (_db.descriptorSetLayoutMaps.isExist(key)) {
			return _db.descriptorSetLayoutMaps[key];
		}

		VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;

		Array<VkDescriptorSetLayoutBinding> bindings(&scopeAllocator);
		for (int i = 0; i < MAX_BINDING_PER_SET; i++) {
			const DescriptorSetLayoutBinding& binding = key.bindings[i];
			if (binding.stageFlags == 0) continue;
			bindings.add({});

			bindings.back().stageFlags = binding.stageFlags;
			bindings.back().descriptorType = binding.descriptorType;
			bindings.back().descriptorCount = binding.descriptorCount;
			bindings.back().binding = i;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();
		SOUL_VK_CHECK(
			vkCreateDescriptorSetLayout(_db.device, &layoutInfo, nullptr, &setLayout),
			"");
		bindings.cleanup();

		_db.descriptorSetLayoutMaps.add(key, setLayout);

		return setLayout;
	}

	PipelineStateID System::_pipelineStateRequest(const PipelineStateDesc& desc, VkRenderPass renderPass) {
		SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::pipelineStateRequest");

		std::lock_guard<std::mutex> lock(_db.pipelineStateRequestMutex);

		if (_db.pipelineStateMaps.isExist(desc)) {
			return _db.pipelineStateMaps[desc];
		}

		Runtime::ScopeAllocator<> scopeAllocator("GPU::System::pipelineStateRequest");

		Program* program = _programPtr(desc.programID);

		Array<VkPipelineShaderStageCreateInfo> shaderStageInfos(&scopeAllocator);

		const Shader& vertShader = *_shaderPtr(program->shaderIDs[ShaderStage::VERTEX]);
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vertShaderStageInfo.module = vertShader.module;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.pName = "main";
		shaderStageInfos.add(vertShaderStageInfo);

		const Shader& fragShader = *_shaderPtr(program->shaderIDs[ShaderStage::FRAGMENT]);
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		fragShaderStageInfo.module = fragShader.module;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.pName = "main";
		shaderStageInfos.add(fragShaderStageInfo);

		ShaderID geomShaderID = program->shaderIDs[ShaderStage::GEOMETRY];
		if (geomShaderID != SHADER_ID_NULL) {
			const Shader& geomShader = *_shaderPtr(geomShaderID);
			VkPipelineShaderStageCreateInfo geomShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			geomShaderStageInfo.module = geomShader.module;
			geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			geomShaderStageInfo.pName = "main";
			shaderStageInfos.add(geomShaderStageInfo);
		}

		static VkPrimitiveTopology primitiveTopologyMap[(uint32)Topology::COUNT] = {
			VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
			VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
		};
		VkPipelineInputAssemblyStateCreateInfo
			inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssemblyState.topology = primitiveTopologyMap[(uint32)desc.inputLayout.topology];
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = desc.viewport.offsetX;
		viewport.y = desc.viewport.offsetY;
		viewport.width = desc.viewport.width;
		viewport.height = desc.viewport.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { int32(desc.scissor.offsetX), int32(desc.scissor.offsetY) };
		scissor.extent = { uint32(desc.scissor.width), uint32(desc.scissor.height) };

		VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		static VkPolygonMode polygonModeMap[(uint32)PolygonMode::COUNT] = {
			VK_POLYGON_MODE_FILL,
			VK_POLYGON_MODE_LINE,
			VK_POLYGON_MODE_POINT
		};

		VkPipelineRasterizationStateCreateInfo rasterizerState = {
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizerState.depthClampEnable = VK_FALSE;
		rasterizerState.rasterizerDiscardEnable = VK_FALSE;
		rasterizerState.polygonMode = polygonModeMap[(uint32)desc.raster.polygonMode];
		rasterizerState.lineWidth = desc.raster.lineWidth;

		// TODO(kevinyu) : Enable multisampling configuration
		VkPipelineMultisampleStateCreateInfo multisampleState = {
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkVertexInputAttributeDescription attrDescs[MAX_INPUT_PER_SHADER];
		uint32 attrDescCount = 0;
		for (int i = 0; i < MAX_INPUT_PER_SHADER; i++) {
			if (vertShader.inputs[i].format == VK_FORMAT_UNDEFINED) continue;
			auto& inputAttribute = desc.inputAttributes[i];
			attrDescs[attrDescCount].format = inputAttribute.type == VertexElementType::DEFAULT ? vertShader.inputs[i].format : vkCast(inputAttribute.type, inputAttribute.flags);
			attrDescs[attrDescCount].binding = desc.inputAttributes[i].binding;
			attrDescs[attrDescCount].location = i;
			attrDescs[attrDescCount].offset = desc.inputAttributes[i].offset;
			attrDescCount++;
		}

		VkVertexInputBindingDescription inputBindingDescs[MAX_INPUT_BINDING_PER_SHADER];
		uint32 inputBindingDescCount = 0;
		for (uint32 i = 0; i < MAX_INPUT_BINDING_PER_SHADER; i++) {
			if (desc.inputBindings[i].stride == 0) continue;
			inputBindingDescs[inputBindingDescCount] = {
				i,
				desc.inputBindings[i].stride,
				VK_VERTEX_INPUT_RATE_VERTEX
			};
			inputBindingDescCount++;
		}

		VkPipelineVertexInputStateCreateInfo inputStateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		inputStateInfo.vertexAttributeDescriptionCount = attrDescCount;
		inputStateInfo.pVertexAttributeDescriptions = attrDescs;
		inputStateInfo.vertexBindingDescriptionCount = attrDescCount > 0 ? inputBindingDescCount : 0;
		inputStateInfo.pVertexBindingDescriptions = attrDescCount > 0 ? inputBindingDescs : nullptr;

		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.stageCount = shaderStageInfos.size();
		pipelineInfo.pStages = shaderStageInfos.data();
		pipelineInfo.pVertexInputState = &inputStateInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerState;
		pipelineInfo.pMultisampleState = &multisampleState;

		VkPipelineColorBlendAttachmentState colorBlendAttachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
		for (int i = 0; i < desc.colorAttachmentCount; i++) {
			VkPipelineColorBlendAttachmentState& blendState = colorBlendAttachments[i];
			const PipelineStateDesc::ColorAttachmentDesc& attachment = desc.colorAttachments[i];
			blendState.blendEnable = attachment.blendEnable ? VK_TRUE : VK_FALSE;
			blendState.srcColorBlendFactor = vkCast(attachment.srcColorBlendFactor);
			blendState.dstColorBlendFactor = vkCast(attachment.dstColorBlendFactor);
			blendState.colorBlendOp = vkCast(attachment.colorBlendOp);
			blendState.srcAlphaBlendFactor = vkCast(attachment.srcAlphaBlendFactor);
			blendState.dstAlphaBlendFactor = vkCast(attachment.dstAlphaBlendFactor);
			blendState.alphaBlendOp = vkCast(attachment.alphaBlendOp);
			blendState.colorWriteMask =
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
				VK_COLOR_COMPONENT_A_BIT;
		}
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = desc.colorAttachmentCount;
		colorBlending.pAttachments = colorBlendAttachments;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;
		pipelineInfo.pColorBlendState = &colorBlending;

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depthStencilInfo.depthTestEnable = desc.depthStencilAttachment.depthTestEnable;
		depthStencilInfo.depthWriteEnable = desc.depthStencilAttachment.depthWriteEnable;
		depthStencilInfo.depthCompareOp = vkCast(desc.depthStencilAttachment.depthCompareOp);
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilInfo.stencilTestEnable = VK_FALSE;
		depthStencilInfo.front = {};
		depthStencilInfo.back = {};
		depthStencilInfo.minDepthBounds = 0.0f;
		depthStencilInfo.maxDepthBounds = 1.0f;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;

		VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_SCISSOR };
		if (desc.scissor.dynamic) {
			dynamicStateInfo.dynamicStateCount = 1;
			dynamicStateInfo.pDynamicStates = dynamicStates;
			pipelineInfo.pDynamicState = &dynamicStateInfo;
		}

		pipelineInfo.layout = program->pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VkPipeline pipeline;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("vkCreateGraphicsPipelines");
			SOUL_VK_CHECK(vkCreateGraphicsPipelines(_db.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "");
		}

		shaderStageInfos.cleanup();

		PipelineStateID pipelineStateID = PipelineStateID(_db.pipelineStates.add({ pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, desc.programID }));
		_db.pipelineStateMaps.add(desc, pipelineStateID);
		return pipelineStateID;

	}

	PipelineState* System::_pipelineStatePtr(PipelineStateID pipelineStateID) {
		return &_db.pipelineStates[pipelineStateID.id];
	}

	const PipelineState& System::_pipelineStateRef(PipelineStateID ID) {
		return _db.pipelineStates[ID.id];
	}

	ProgramID System::programRequest(const ProgramDesc& programDesc) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::programRequest");
		Runtime::ScopeAllocator<> scopeAllocator("GPU::System::programCreate");

		if (_db.programMaps.isExist(programDesc)) {
			return _db.programMaps[programDesc];
		}

		ProgramID programID = ProgramID(_db.programs.add({}));
		Program &program = _db.programs[programID.id];

		for (int i = 0; i < uint64(ShaderStage::COUNT); i++) {
			program.shaderIDs[ShaderStage(i)] = programDesc.shaderIDs[ShaderStage(i)];
		}

		const EnumArray<ShaderStage, ShaderID>& shaderIDs = programDesc.shaderIDs;
		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			for (int j = 0; j < uint64(ShaderStage::COUNT); j++) {
				ShaderID shaderID = shaderIDs[ShaderStage(j)];
				if (shaderID == SHADER_ID_NULL) continue;
				const Shader &shader = *_shaderPtr(shaderID);

				for (int k = 0; k < MAX_BINDING_PER_SET; k++) {

					static VkPipelineStageFlags vulkanPipelineStageBitsMap[] = {
						0,
						VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
						VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					};
					static_assert(SOUL_ARRAY_LEN(vulkanPipelineStageBitsMap) == uint64(ShaderStage::COUNT), "");

					ProgramDescriptorBinding &progBinding = program.bindings[i][k];
					const ShaderDescriptorBinding &shaderBinding = shader.bindings[i][k];

					if (shader.bindings[i][k].count == 0) continue;

					if (progBinding.shaderStageFlags == 0) {
						progBinding.type = shaderBinding.type;
						progBinding.count = shaderBinding.count;
						progBinding.attachmentIndex = shaderBinding.attachmentIndex;
					}
					else {
						SOUL_ASSERT(0, program.bindings[i][k].type == shader.bindings[i][k].type, "");
						SOUL_ASSERT(0, program.bindings[i][k].count == shader.bindings[i][k].count, "");
						SOUL_ASSERT(0, program.bindings[i][k].attachmentIndex == shader.bindings[i][k].attachmentIndex, "");
					}
					progBinding.shaderStageFlags |= vkCast(ShaderStage(j));
					progBinding.pipelineStageFlags |= vulkanPipelineStageBitsMap[j];
				}
			}
		}

		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			DescriptorSetLayoutKey descriptorSetLayoutKey = {};

			for (int j = 0; j < MAX_BINDING_PER_SET; j++) {
				const ProgramDescriptorBinding& progBinding = program.bindings[i][j];
				if (progBinding.shaderStageFlags == 0) continue;

				DescriptorSetLayoutBinding& binding = descriptorSetLayoutKey.bindings[j];

				binding.stageFlags = progBinding.shaderStageFlags;
				binding.descriptorType = vkCast(progBinding.type);
				binding.descriptorCount = progBinding.count;
			}
			program.descriptorLayouts[i] = _descriptorSetLayoutRequest(descriptorSetLayoutKey);
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount = MAX_SET_PER_SHADER_PROGRAM;
		pipelineLayoutInfo.pSetLayouts = program.descriptorLayouts;

		SOUL_VK_CHECK(vkCreatePipelineLayout(_db.device, &pipelineLayoutInfo, nullptr, &program.pipelineLayout),"");

		_db.programMaps.add(programDesc, programID);
		return programID;
	}

	Program *System::_programPtr(ProgramID programID) {
		SOUL_ASSERT(0, programID != PROGRAM_ID_NULL, "");
		return &_db.programs[programID.id];
	}

	const Program& System::_programRef(ProgramID programID) {
		SOUL_ASSERT(0, programID != PROGRAM_ID_NULL, "");
		return _db.programs[programID.id];
	}

	VkPipeline System::_pipelineCreate(const ComputeBaseNode &node, ProgramID programID) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::_pipelineCreate");
		Runtime::ScopeAllocator<> scopeAllocator("GPU::System::_pipelineCreate");

		const ComputePipelineConfig& pipelineConfig = node.pipelineConfig;
		SOUL_ASSERT(0, pipelineConfig.computeShaderID!=SHADER_ID_NULL, "");

		const Shader &computeShader = *_shaderPtr(pipelineConfig.computeShaderID);

		VkComputePipelineCreateInfo pipelineInfo = {
			VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			0, 0,
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				0, 0, VK_SHADER_STAGE_COMPUTE_BIT, computeShader.module, "main", 0
			},
			_programPtr(programID)->pipelineLayout, 0, 0
		};

		VkPipeline pipeline;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("vkCreateGraphicsPipelines");
			SOUL_VK_CHECK(vkCreateComputePipelines(_db.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),"");
		}
		return pipeline;
	}

	SamplerID System::samplerRequest(const SamplerDesc &desc) {
		uint64 hashKey = hashFNV1((const uint8 *) &desc, sizeof(SamplerDesc));
		if (_db.samplerMap.isExist(hashKey)) {
			return SamplerID(_db.samplerMap[hashKey]);
		}

		VkSamplerCreateInfo samplerCreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		samplerCreateInfo.minFilter = vkCast(desc.minFilter);
		samplerCreateInfo.magFilter = vkCast(desc.magFilter);
		samplerCreateInfo.mipmapMode = vkCastMipmapFilter(desc.mipmapFilter);
		samplerCreateInfo.addressModeU = vkCast(desc.wrapU);
		samplerCreateInfo.addressModeV = vkCast(desc.wrapV);
		samplerCreateInfo.addressModeW = vkCast(desc.wrapW);
		samplerCreateInfo.anisotropyEnable = desc.anisotropyEnable;
		samplerCreateInfo.maxAnisotropy = desc.maxAnisotropy;
		samplerCreateInfo.minLod = 0;
		samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		VkSampler sampler;
		SOUL_VK_CHECK(vkCreateSampler(_db.device, &samplerCreateInfo, nullptr, &sampler), "");
		_db.samplerMap.add(hashKey, sampler);

		return SamplerID(sampler);
	}

	ShaderArgSetID System::_shaderArgSetRequest(const ShaderArgSetDesc& desc) {
		uint64 hash = 0;
		int offsetCount = 0;
		int offset[MAX_DYNAMIC_BUFFER_PER_SET];
		for (int i = 0; i < desc.bindingCount; i++) {
			Descriptor& descriptorDesc = desc.bindingDescriptions[i];
			switch (descriptorDesc.type) {
			case DescriptorType::NONE:
				{
					hash = hashFNV1((uint8*)&descriptorDesc.type, sizeof(descriptorDesc.type), hash);
					break;
				}
			case DescriptorType::UNIFORM_BUFFER: {
				const UniformDescriptor& uniformDescriptor = descriptorDesc.uniformInfo;
				hash = hashFNV1((uint8*)&descriptorDesc.type, sizeof(descriptorDesc.type), hash);
				Buffer* buffer = _bufferPtr(descriptorDesc.uniformInfo.bufferID);
				hash = hashFNV1((uint8*)&descriptorDesc.uniformInfo.bufferID, sizeof(BufferID), hash);
				offset[offsetCount++] =
					uniformDescriptor.unitIndex * _bufferPtr(uniformDescriptor.bufferID)->unitSize;
				break;
			}
			case DescriptorType::SAMPLED_IMAGE: {
				hash = hashFNV1((uint8*)&descriptorDesc.type, sizeof(descriptorDesc.type), hash);
				SampledImageDescriptor& descriptor = descriptorDesc.sampledImageInfo;
				hash = hashFNV1((uint8*)&(_texturePtr(descriptor.textureID)->view), sizeof(VkImageView), hash);
				hash = hashFNV1((uint8*)&descriptorDesc.sampledImageInfo.samplerID, sizeof(SamplerID), hash);
				break;
			}
			case DescriptorType::INPUT_ATTACHMENT: {
				hash = hashFNV1((uint8*)&descriptorDesc.type, sizeof(descriptorDesc.type), hash);
				hash = hashFNV1((uint8*)&(_texturePtr(descriptorDesc.inputAttachmentInfo.textureID)->view), sizeof(VkImageView), hash);
				break;
			}
			case DescriptorType::STORAGE_IMAGE: {
				hash = hashFNV1((uint8*)&descriptorDesc.type, sizeof(descriptorDesc.type), hash);
				hash = hashFNV1((uint8*)&(_texturePtr(descriptorDesc.storageImageInfo.textureID)->view), sizeof(VkImageView), hash);
				hash = hashFNV1((uint8*)&descriptorDesc.storageImageInfo.mipLevel, sizeof(descriptorDesc.storageImageInfo.mipLevel), hash);
				break;
			}
			default: {
				SOUL_NOT_IMPLEMENTED();
				break;
			}
			}
		}
		VkDescriptorSet descriptorSet;
		ShaderArgSetID argSetID;
		{
			std::lock_guard<std::mutex> lock(_db.shaderArgSetRequestMutex);
			if (_db.descriptorSets.isExist(hash)) {
				descriptorSet = _db.descriptorSets[hash];
			}
			else {

				DescriptorSetLayoutKey setLayoutKey = {};

				for (int i = 0; i < desc.bindingCount; i++) {
					Descriptor& descriptorDesc = desc.bindingDescriptions[i];
					if (descriptorDesc.type == DescriptorType::NONE) continue;
					setLayoutKey.bindings[i].descriptorCount = 1;
					setLayoutKey.bindings[i].descriptorType = vkCast(descriptorDesc.type);
					setLayoutKey.bindings[i].stageFlags = vkCast(descriptorDesc.stageFlags);
				}

				VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
				allocInfo.descriptorPool = _frameContext().descriptorPool;
				allocInfo.descriptorSetCount = 1;
				VkDescriptorSetLayout setLayout = _descriptorSetLayoutRequest(setLayoutKey);
				allocInfo.pSetLayouts = &setLayout;

				SOUL_VK_CHECK(vkAllocateDescriptorSets(_db.device, &allocInfo, &descriptorSet), "");

				for (int i = 0; i < desc.bindingCount; i++) {
					VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					descriptorWrite.dstSet = descriptorSet;
					descriptorWrite.dstBinding = i;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorCount = 1;

					Descriptor& descriptorDesc = desc.bindingDescriptions[i];
					descriptorWrite.descriptorType = vkCast(descriptorDesc.type);

					VkDescriptorBufferInfo bufferInfo;
					VkDescriptorImageInfo imageInfo;
					switch (descriptorDesc.type) {
					case DescriptorType::NONE:
						{
							continue;
						}
					case DescriptorType::UNIFORM_BUFFER: {
						bufferInfo.buffer = _bufferPtr(descriptorDesc.uniformInfo.bufferID)->vkHandle;
						Buffer* buffer = _bufferPtr(descriptorDesc.uniformInfo.bufferID);
						bufferInfo.offset = 0;
						bufferInfo.range = buffer->unitSize;
						descriptorWrite.pBufferInfo = &bufferInfo;
						break;
					}
					case DescriptorType::SAMPLED_IMAGE: {
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						TextureID texID = descriptorDesc.sampledImageInfo.textureID;
						imageInfo.imageView = _texturePtr(texID)->view;
						imageInfo.sampler = descriptorDesc.sampledImageInfo.samplerID.id;
						descriptorWrite.pImageInfo = &imageInfo;
						break;
					}
					case DescriptorType::INPUT_ATTACHMENT: {
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfo.imageView = _texturePtr(descriptorDesc.inputAttachmentInfo.textureID)->view;
						imageInfo.sampler = VK_NULL_HANDLE;
						descriptorWrite.pImageInfo = &imageInfo;
						break;
					}
					case DescriptorType::STORAGE_IMAGE: {
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageInfo.imageView = _textureGetMipView(descriptorDesc.storageImageInfo.textureID, descriptorDesc.storageImageInfo.mipLevel);
						imageInfo.sampler = VK_NULL_HANDLE;
						descriptorWrite.pImageInfo = &imageInfo;
						break;
					}
					default: {
						SOUL_NOT_IMPLEMENTED();
						break;
					}
					}

					vkUpdateDescriptorSets(_db.device, 1, &descriptorWrite, 0, nullptr);
				}
				_db.descriptorSets.add(hash, descriptorSet);
			}
			argSetID = ShaderArgSetID(_db.shaderArgSetIDs.add(ShaderArgSet()));

		}

		ShaderArgSet& argSet = _db.shaderArgSetIDs[argSetID.id];

		argSet.vkHandle = descriptorSet;
		argSet.offsetCount = offsetCount;
		for (int i = 0; i < offsetCount; i++) {
			argSet.offset[i] = offset[i];
		}

		return argSetID;
	}

	const ShaderArgSet& System::_shaderArgSetRef(ShaderArgSetID argSetID) {
		SOUL_ASSERT(!argSetID.isNull(), "");
		return _db.shaderArgSetIDs[argSetID.id];
	}

	VkRenderPass System::_renderPassRequest(const RenderPassKey& key)
	{
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();
		if (_db.renderPassMaps.isExist(key))
		{
			return _db.renderPassMaps[key];
		}

		auto attachmentFlagToLoadOp = [](AttachmentFlags flags) -> VkAttachmentLoadOp
		{
			if (flags & ATTACHMENT_CLEAR_BIT)
			{
				return VK_ATTACHMENT_LOAD_OP_CLEAR;
			}
			else if ((flags & ATTACHMENT_FIRST_PASS_BIT) && !(flags & ATTACHMENT_EXTERNAL_BIT))
			{
				return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}
			
			return VK_ATTACHMENT_LOAD_OP_LOAD;
			
		};

		auto attachmentFlagToStoreOp = [](AttachmentFlags flags) -> VkAttachmentStoreOp
		{
			if ((flags & ATTACHMENT_LAST_PASS_BIT) && !(flags & ATTACHMENT_EXTERNAL_BIT))
			{
				return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}
			return VK_ATTACHMENT_STORE_OP_STORE;
		};

		VkAttachmentDescription attachments[MAX_COLOR_ATTACHMENT_PER_SHADER + MAX_INPUT_ATTACHMENT_PER_SHADER + 1] = {};
		VkAttachmentReference attachmentRefs[MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
		uint8 attachmentCount = 0;
		for (int i = 0; i < MAX_COLOR_ATTACHMENT_PER_SHADER; i++)
		{
			if (key.colorAttachments[i].flags & ATTACHMENT_ACTIVE_BIT)
			{
				VkAttachmentDescription& attachment = attachments[attachmentCount];
				attachment.format = vkCast(key.colorAttachments[i].format);
				attachment.samples = VK_SAMPLE_COUNT_1_BIT;
				AttachmentFlags flags = key.colorAttachments[i].flags;
				
				attachment.loadOp = attachmentFlagToLoadOp(flags);
				attachment.storeOp = attachmentFlagToStoreOp(flags);
				
				attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

				attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				attachmentRefs[attachmentCount].attachment = attachmentCount;
				attachmentRefs[attachmentCount].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				attachmentCount++;
			}
		}
		uint8 colorAttachmentCount = attachmentCount;

		for (int i = 0; i < MAX_INPUT_ATTACHMENT_PER_SHADER; i++) {
			const Attachment& attachmentKey = key.inputAttachments[i];
			if (attachmentKey.flags & ATTACHMENT_ACTIVE_BIT) {
				VkAttachmentDescription& attachment = attachments[attachmentCount];
				attachment.format = vkCast(attachmentKey.format);
				attachment.samples = VK_SAMPLE_COUNT_1_BIT;

				AttachmentFlags flags = attachmentKey.flags;

				attachment.loadOp = attachmentFlagToLoadOp(flags);
				attachment.storeOp = attachmentFlagToStoreOp(flags);

				attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

				attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				attachmentRefs[attachmentCount].attachment = attachmentCount;
				attachmentRefs[attachmentCount].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				attachmentCount++;
			}
		}
		uint8 inputAttachmentCount = attachmentCount - colorAttachmentCount;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = colorAttachmentCount;
		subpass.pColorAttachments = attachmentRefs;
		subpass.inputAttachmentCount = inputAttachmentCount;
		subpass.pInputAttachments = attachmentRefs + colorAttachmentCount;

		if (key.depthAttachment.flags & ATTACHMENT_ACTIVE_BIT)
		{
			AttachmentFlags flags = key.depthAttachment.flags;
			VkAttachmentDescription& attachment = attachments[attachmentCount];
			attachment.format = vkCast(key.depthAttachment.format);
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;

			attachment.loadOp = attachmentFlagToLoadOp(flags);
			attachment.storeOp = attachmentFlagToStoreOp(flags);

			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference& depthAttachmentRef = attachmentRefs[attachmentCount];
			depthAttachmentRef.attachment = attachmentCount;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpass.pDepthStencilAttachment = &depthAttachmentRef;
			attachmentCount++;
		}
		
		VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount = attachmentCount;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = nullptr;

		VkRenderPass renderPass;
		SOUL_VK_CHECK(vkCreateRenderPass(_db.device, &renderPassInfo, nullptr, &renderPass), "");

		_db.renderPassMaps.add(key, renderPass);
		
		return renderPass;
	}

	VkRenderPass System::_renderPassCreate(const VkRenderPassCreateInfo &info) {
		SOUL_PROFILE_ZONE();
		VkRenderPass renderPass;
		SOUL_VK_CHECK(vkCreateRenderPass(_db.device, &info, nullptr, &renderPass), "");
		return renderPass;
	}

	void System::_renderPassDestroy(VkRenderPass renderPass) {
		_frameContext().garbages.renderPasses.add(renderPass);
	}
	
	VkFramebuffer System::_framebufferCreate(const VkFramebufferCreateInfo &info) {
		SOUL_PROFILE_ZONE();
		VkFramebuffer framebuffer;
		SOUL_VK_CHECK(vkCreateFramebuffer(_db.device, &info, nullptr, &framebuffer), "");
		return framebuffer;
	}

	void System::_framebufferDestroy(VkFramebuffer framebuffer) {
		_frameContext().garbages.frameBuffers.add(framebuffer);
	}

	VkEvent System::_eventCreate() {
		SOUL_ASSERT_MAIN_THREAD();
		VkEvent event;
		VkEventCreateInfo eventInfo = {VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};
		SOUL_VK_CHECK(vkCreateEvent(_db.device, &eventInfo, nullptr, &event), "");
		return event;
	}

	void System::_eventDestroy(VkEvent event) {
		SOUL_ASSERT_MAIN_THREAD();
		_frameContext().garbages.events.add(event);
	}

	SemaphoreID System::_semaphoreCreate() {
		SOUL_ASSERT_MAIN_THREAD();
		SemaphoreID semaphoreID = SemaphoreID(_db.semaphores.add(Semaphore()));
		Semaphore &semaphore = _db.semaphores[semaphoreID.id];
		semaphore = {};
		VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		SOUL_VK_CHECK(vkCreateSemaphore(_db.device, &semaphoreInfo, nullptr, &semaphore.vkHandle), "");
		return semaphoreID;
	}

	void System::_semaphoreReset(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		_semaphorePtr(ID)->stageFlags = 0;
		_semaphorePtr(ID)->state = semaphore_state::INITIAL;
	}

	Semaphore *System::_semaphorePtr(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		return &_db.semaphores[ID.id];
	}

	void System::_semaphoreDestroy(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		_frameContext().garbages.semaphores.add(ID);
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
		submit(commandBuffer, Soul::Cast<uint32>(semaphores.size()), semaphores.data(), fence);
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

		SOUL_VK_CHECK(vkEndCommandBuffer(commandBuffer), "");

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
		submitInfo.commandBufferCount = Soul::Cast<uint32>(commands.size());
		submitInfo.pCommandBuffers = commands.data();


		SOUL_ASSERT(0, waitSemaphores.size() == waitStages.size(), "");
		submitInfo.waitSemaphoreCount = Soul::Cast<uint32>(waitSemaphores.size());
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();

		VkSemaphore signalSemaphores[MAX_SIGNAL_SEMAPHORE];
		for (soul_size i = 0; i < semaphoreCount; i++) {
			signalSemaphores[i] = semaphores[i]->vkHandle;
		}

		submitInfo.signalSemaphoreCount = semaphoreCount;
		submitInfo.pSignalSemaphores = signalSemaphores;

		SOUL_VK_CHECK(vkQueueSubmit(vkHandle, 1, &submitInfo, fence), "");

		commands.resize(0);
		waitSemaphores.resize(0);
		waitStages.resize(0);
	}

	void CommandQueue::present(const VkPresentInfoKHR& presentInfo)
	{
		SOUL_VK_CHECK(vkQueuePresentKHR(vkHandle, &presentInfo), "");
	}
	
	void System::renderGraphExecute(const RenderGraph &renderGraph) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();

		RenderGraphExecution execution(&renderGraph, this, Runtime::GetContextAllocator(), _db.queues, _frameContext().commandPools);
		execution.init();
		if (!_frameContext().stagingSynced) _stagingFlush();
		execution.run();
		execution.cleanup();
	}

	void System::frameFlush() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		_frameEnd();
		_frameBegin();
	}

	void System::_frameBegin() {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		_FrameContext &frameContext = _frameContext();
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Wait fence");
			SOUL_VK_CHECK(vkWaitForFences(_db.device, 1, &frameContext.fence, VK_TRUE, UINT64_MAX), "");
		}
		
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Reset command pools");
			frameContext.commandPools.reset();
		}
		
		_stagingFrameBegin();

		_semaphoreReset(frameContext.imageAvailableSemaphore);
		_semaphoreReset(frameContext.renderFinishedSemaphore);

		uint32 swapchainIndex;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Acquire Next Image KHR");
			vkAcquireNextImageKHR(_db.device, _db.swapchain.vkHandle, UINT64_MAX,
			                      _semaphorePtr(frameContext.imageAvailableSemaphore)->vkHandle, VK_NULL_HANDLE,
			                      &swapchainIndex);
		}
		
		_semaphorePtr(frameContext.imageAvailableSemaphore)->state = semaphore_state::SUBMITTED;
		frameContext.swapchainIndex = swapchainIndex;
		TextureID swapchainTextureID = _db.swapchain.textures[swapchainIndex];
		Texture *swapchainTexture = _texturePtr(swapchainTextureID);
		swapchainTexture->owner = ResourceOwner::PRESENTATION_ENGINE;
		swapchainTexture->layout = VK_IMAGE_LAYOUT_UNDEFINED;

		{
			SOUL_PROFILE_ZONE_WITH_NAME("Reset descriptor pool");
			vkResetDescriptorPool(_db.device, frameContext.descriptorPool, 0);
		}
		
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Destroy images");
			for (TextureID textureID : frameContext.garbages.textures) {
				Texture& texture = *_texturePtr(textureID);
				vmaDestroyImage(_db.gpuAllocator, texture.vkHandle, texture.allocation);
				vkDestroyImageView(_db.device, texture.view, nullptr);
				if (texture.mipViews != nullptr) {
					for (int i = 0; i < texture.mipCount; i++) {
						vkDestroyImageView(_db.device, texture.mipViews[i], nullptr);
					}
					_db.cpuAllocator.deallocate(texture.mipViews, sizeof(VkImageView) * texture.mipCount);
					texture.mipViews = nullptr;
				}
				_db.textures.remove(textureID.id);
			}
			frameContext.garbages.textures.resize(0);
		}
		

		{
			SOUL_PROFILE_ZONE_WITH_NAME("Destroy buffers");
			for (BufferID bufferID : frameContext.garbages.buffers) {
				Buffer& buffer = *_bufferPtr(bufferID);
				vmaDestroyBuffer(_db.gpuAllocator, buffer.vkHandle, buffer.allocation);
				_db.buffers.remove(bufferID.id);
			}
			frameContext.garbages.buffers.resize(0);
		}

		for (ShaderID shaderID : frameContext.garbages.shaders) {
			Shader &shader = _db.shaders[shaderID.id];
			vkDestroyShaderModule(_db.device, shader.module, nullptr);
			_db.shaders.remove(shaderID.id);
		}
		frameContext.garbages.shaders.resize(0);

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
			vkDestroySemaphore(_db.device, _semaphorePtr(semaphoreID)->vkHandle, nullptr);
			_db.semaphores.remove(semaphoreID.id);
		}
		frameContext.garbages.semaphores.resize(0);

		frameContext.stagingAvailable = false;
		frameContext.stagingSynced = true;

		_db.shaderArgSetIDs.resize(1);
	}

	void System::_frameEnd() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		_FrameContext &frameContext = _frameContext();
		uint32 swapchainIndex = frameContext.swapchainIndex;
		if (_db.swapchain.fences[swapchainIndex] != VK_NULL_HANDLE) {
			SOUL_VK_CHECK(vkWaitForFences(_db.device, 1, &_db.swapchain.fences[swapchainIndex], VK_TRUE, UINT64_MAX), "");
		}
		_db.swapchain.fences[swapchainIndex] = frameContext.fence;

		Texture &swapchainTexture = *_texturePtr(_db.swapchain.textures[swapchainIndex]);
		SOUL_VK_CHECK(vkResetFences(_db.device, 1, &frameContext.fence), "");
		{
			SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::LastSubmission");
			_stagingFrameEnd();

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

			static constexpr EnumArray<ResourceOwner, QueueType> RESOURCE_ONWER_TO_QUEUE_TYPE({
				QueueType::COUNT,
				QueueType::GRAPHIC,
				QueueType::COMPUTE,
				QueueType::TRANSFER,
				QueueType::GRAPHIC
			});

			auto _syncQueueToGraphic = [this](QueueType queueType) {
				SemaphoreID semaphoreID = _semaphoreCreate();
				VkCommandBuffer cmdBuffer = _frameContext().commandPools.requestCommandBuffer(queueType);
				Semaphore* semaphorePtr = _semaphorePtr(semaphoreID);
				_db.queues[queueType].submit(cmdBuffer, 1, &semaphorePtr);
				_db.queues[QueueType::GRAPHIC].wait(semaphorePtr, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
				_semaphoreDestroy(semaphoreID);
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
				_db.queues[QueueType::GRAPHIC].wait(_semaphorePtr(_frameContext().imageAvailableSemaphore), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			}

			_db.queues[RESOURCE_ONWER_TO_QUEUE_TYPE[swapchainTexture.owner]].submit(cmdBuffer, _semaphorePtr(_frameContext().renderFinishedSemaphore), frameContext.fence);
			swapchainTexture.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &_semaphorePtr(frameContext.renderFinishedSemaphore)->vkHandle;
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

		_db.descriptorSets.clear();
	}

	Vec2ui32 System::getSwapchainExtent() {
		return {_db.swapchain.extent.width, _db.swapchain.extent.height};
	}

	TextureID System::getSwapchainTexture() {
		return _db.swapchain.textures[_frameContext().swapchainIndex];
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

			SOUL_VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer), "");
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
	
	void CommandPools::init(VkDevice device, const CommandQueues& queues, soul_size threadCount)
	{
		Runtime::PushAllocator(allocator);
		secondaryPools.resize(threadCount);
		for (auto& pool: secondaryPools)
		{
			pool.init(device, VK_COMMAND_BUFFER_LEVEL_SECONDARY, queues[QueueType::GRAPHIC].getFamilyIndex());
		}

		for (const auto queueType : EnumIter<QueueType>::Iterates())
		{
			primaryPools[queueType].init(device, VK_COMMAND_BUFFER_LEVEL_PRIMARY, queues[queueType].getFamilyIndex());
		}
		Runtime::PopAllocator();
	}

	void CommandPools::reset()
	{
		for (auto& pool : primaryPools)
		{
			pool.reset();
		}

		for (auto& pool : secondaryPools)
		{
			pool.reset();
		}
	}

	VkCommandBuffer CommandPools::requestCommandBuffer(QueueType queueType)
	{
		SOUL_ASSERT_MAIN_THREAD();
		VkCommandBuffer cmdBuffer = primaryPools[queueType].request();
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		SOUL_VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo), "");
		return cmdBuffer;
	}

	VkCommandBuffer CommandPools::requestSecondaryCommandBuffer()
	{
		return secondaryPools[Runtime::GetThreadId()].request();
	}


}
