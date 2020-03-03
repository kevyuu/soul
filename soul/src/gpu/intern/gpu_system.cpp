#include "gpu/system.h"
#include "gpu/render_graph.h"
#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_graph_execution.h"

#include "job/system.h"

#include "memory/allocators/scope_allocator.h"
#include "memory/allocators/mt_linear_allocator.h"

#include "core/array.h"
#include "core/math.h"
#include "core/dev_util.h"

#include <volk/volk.h>

#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>
#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>

#include <cstddef>
#include <limits>

namespace Soul { namespace GPU {

	static VKAPI_ATTR VkBool32 VKAPI_CALL _debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
			void *pUserData) {
		SOUL_LOG_INFO("_debugCallback");
		switch (messageSeverity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
				SOUL_LOG_INFO("VkDebugUtils: %s", pCallbackData->pMessage);
				break;
			}
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
				SOUL_LOG_WARN("VkDebugUtils: %s", pCallbackData->pMessage);
				break;
			}
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
				SOUL_LOG_ERROR("VkDebugUtils: %s", pCallbackData->pMessage);
				break;
			}
			default: {
				SOUL_LOG_INFO("VkDebugUtils: %s", pCallbackData->pMessage);
				break;
			}
		}
		SOUL_PANIC(0, "Vulkan Error!");
		return VK_FALSE;
	}

	void System::init(const Config &config) {
		SOUL_ASSERT_MAIN_THREAD();
		Memory::ScopeAllocator<> scopeAllocator("GPU::System::init");
		SOUL_MEMORY_ALLOCATOR_ZONE(&scopeAllocator);

		_db.currentFrame = 0;

		_db.buffers.add({});
		_db.textures.add({});
		_db.shaderArgSets.add({});
		_db.shaders.add({});
		_db.programs.add({});
		_db.semaphores.add({});

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
		appInfo.apiVersion = VK_API_VERSION_1_1;

		SOUL_LOG_INFO("Creating vulkan instance");
		VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		instanceCreateInfo.pApplicationInfo = &appInfo;

		static const char *requiredExtensions[] = {
				VK_KHR_SURFACE_EXTENSION_NAME,

#if defined(SOUL_OS_WINDOWS)
				"VK_KHR_win32_surface",
#endif // SOUL_PLATFORM_OS_WIN32

#ifdef SOUL_OS_APPLE
				"VK_MVK_macos_surface",
#endif // SOUL_PLATFORM_OS_APPLE
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		static const uint32_t requiredExtensionCount = sizeof(requiredExtensions) / sizeof(requiredExtensions[0]);
		instanceCreateInfo.enabledExtensionCount = requiredExtensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions;

		static const char *requiredLayers[] = {

#ifdef SOUL_OPTION_VULKAN_ENABLE_VALIDATION
				"VK_LAYER_KHRONOS_validation",
#endif // SOUL_OPTION_VULKAN_ENABLE_VALIDATION

#ifdef SOUL_OPTION_VULKAN_ENABLE_RENDERDOC
				"VK_LAYER_RENDERDOC_Capture",
#endif // SOUL_OPTION_VULKAN_ENABLE_RENDERDOC

		};
		static const uint32_t requiredLayerCount = sizeof(requiredLayers) / sizeof(requiredLayers[0]);

		static const auto checkLayerSupport = [](int count, const char **layers) -> bool {
			SOUL_LOG_INFO("Check vulkan layer support.");
			uint32 layerCount;

			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			Array<VkLayerProperties> availableLayers;
			availableLayers.resize(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
			for (int i = 0; i < count; i++) {
				bool layerFound = false;
				for (int j = 0; j < layerCount; j++) {
					if (strcmp(layers[i], availableLayers[j].layerName) == 0) {
						layerFound = true;
						break;
					}
				}
				if (!layerFound) {
					SOUL_LOG_INFO("Validation layer %s not found!", layers[i]);
					return false;
				}
			}
			availableLayers.cleanup();

			return true;
		};

		SOUL_ASSERT(0, checkLayerSupport(requiredLayerCount, requiredLayers), "");
		instanceCreateInfo.enabledLayerCount = requiredLayerCount;
		instanceCreateInfo.ppEnabledLayerNames = requiredLayers;
		SOUL_VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &_db.instance),
					  "Vulkan instance creation fail!");
		SOUL_LOG_INFO("Vulkan instance creation succesful");

		volkLoadInstance(_db.instance);

		static const auto createDebugUtilsMessenger = [](_Database *db) {
			SOUL_LOG_INFO("Creating vulkan debug utils messenger");
			VkDebugUtilsMessengerCreateInfoEXT
					debugMessengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
			debugMessengerCreateInfo.messageSeverity =
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
					| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugMessengerCreateInfo.messageType =
					VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
					| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugMessengerCreateInfo.pfnUserCallback = _debugCallback;
			debugMessengerCreateInfo.pUserData = nullptr;
			SOUL_VK_CHECK(vkCreateDebugUtilsMessengerEXT(db->instance, &debugMessengerCreateInfo, nullptr,
														 &db->debugMessenger),
						  "Vulkan debug messenger creation fail!");
			SOUL_LOG_INFO("Vulkan debug messenger creation sucesfull");
		};
		createDebugUtilsMessenger(&_db);

		_surfaceCreate(config.windowHandle, &_db.surface);

		static const auto
				pickPhysicalDevice = [](_Database *db, int requiredExtensionCount,
										const char *requiredExtensions[]) {
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
							  " -- Api Version = 0x%.8X\n"
							  " -- Driver Version = 0x%.8X\n",
							  i, physicalDeviceProperties.deviceName, vendorID, deviceID, apiVersion,
							  driverVersion);

				uint32_t extensionCount;
				vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensionCount, nullptr);

				Soul::Array<VkExtensionProperties> availableExtensions;
				availableExtensions.resize(extensionCount);
				bool allExtensionFound = true;
				for (int j = 0; j < requiredExtensionCount; j++) {
					bool extensionFound = false;
					for (int k = 0; k < extensionCount; k++) {
						if (strcmp(availableExtensions[k].extensionName, requiredExtensions[j])) {
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

				SOUL_LOG_INFO("vkGetPhysicalDeviceSurfaceCapabilities");
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[i], db->surface, &db->surfaceCaps);

				uint32 formatCount;
				SOUL_LOG_INFO("vkGetPhysicalDeviceSurfaceFormatsKHR");
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
				if (score > bestScore) {
					db->physicalDevice = devices[i];
					db->graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
					db->presentQueueFamilyIndex = presentQueueFamilyIndex;
					db->transferQueueFamilyIndex = transferQueueFamilyIndex;
					db->computeQueueFamilyIndex = computeQueueFamilyIndex;
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
						  " -- Presentation queue family index = %d\n"
						  " -- Transfer queue family index = %d\n"
						  " -- Compute queue family index = %d\n",
						  db->physicalDeviceProperties.deviceName,
						  vendorID,
						  deviceID,
						  apiVersion,
						  driverVersion,
						  db->graphicsQueueFamilyIndex,
						  db->presentQueueFamilyIndex,
						  db->transferQueueFamilyIndex,
						  db->computeQueueFamilyIndex);
			db->queueFamilyIndices[QueueType::NONE] = 0;
			db->queueFamilyIndices[QueueType::GRAPHIC] = db->graphicsQueueFamilyIndex;
			db->queueFamilyIndices[QueueType::TRANSFER] = db->transferQueueFamilyIndex;
			db->queueFamilyIndices[QueueType::COMPUTE] = db->computeQueueFamilyIndex;

			formats.cleanup();
			presentModes.cleanup();
			devices.cleanup();

			SOUL_ASSERT(0, db->physicalDevice != VK_NULL_HANDLE, "Fail to find a suitable GPU!");
		};
		pickPhysicalDevice(&_db, requiredExtensionCount, requiredExtensions);

		static const auto createDevice = [](_Database *db) {
			SOUL_LOG_INFO("Creating vulkan logical device");

			int graphicsQueueCount = 1;
			int graphicsQueueIndex = 0;
			int computeQueueIndex = 0;
			int transferQueueIndex = 0;
			int presentQueueIndex = 0;

			if (db->computeQueueFamilyIndex == db->graphicsQueueFamilyIndex) {
				graphicsQueueCount++;
				computeQueueIndex = graphicsQueueIndex + 1;
			}
			if (db->transferQueueFamilyIndex == db->graphicsQueueFamilyIndex) {
				graphicsQueueCount++;
				transferQueueIndex = computeQueueIndex + 1;
			}

			VkDeviceQueueCreateInfo queueCreateInfo[4] = {};
			float priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};
			int queueCreateInfoCount = 1;

			queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo[0].queueFamilyIndex = db->graphicsQueueFamilyIndex;
			queueCreateInfo[0].queueCount = graphicsQueueCount;
			queueCreateInfo[0].pQueuePriorities = priorities;

			if (db->computeQueueFamilyIndex != db->graphicsQueueFamilyIndex) {
				queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = db->computeQueueFamilyIndex;
				queueCreateInfo[queueCreateInfoCount].queueCount = 1;
				queueCreateInfo[queueCreateInfoCount].pQueuePriorities = priorities + queueCreateInfoCount;
				queueCreateInfoCount++;
			}

			if (db->transferQueueFamilyIndex != db->graphicsQueueFamilyIndex) {
				queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = db->transferQueueFamilyIndex;
				queueCreateInfo[queueCreateInfoCount].queueCount = 1;
				queueCreateInfo[queueCreateInfoCount].pQueuePriorities = priorities + queueCreateInfoCount;
				queueCreateInfoCount++;
			}

			if (db->presentQueueFamilyIndex != db->graphicsQueueFamilyIndex &&
				db->presentQueueFamilyIndex != db->computeQueueFamilyIndex &&
				db->presentQueueFamilyIndex != db->transferQueueFamilyIndex) {
				queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = db->presentQueueFamilyIndex;
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

			VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
			deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
			deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

			static const char *deviceExtensions[] = {
					VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			const uint32 deviceExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);

			deviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

			SOUL_ASSERT(0,
						db->graphicsQueueFamilyIndex == db->presentQueueFamilyIndex,
						"Different queue for graphics and present not supported yet!");

			SOUL_VK_CHECK(vkCreateDevice(db->physicalDevice, &deviceCreateInfo, nullptr, &db->device),
						  "Vulkan logical device creation fail!");
			SOUL_LOG_INFO("Vulkan logical device creation sucessful");

			vkGetDeviceQueue(db->device, db->graphicsQueueFamilyIndex, graphicsQueueIndex, &db->graphicsQueue);
			SOUL_LOG_INFO("Vulkan retrieve graphics queue successful");
			db->presentQueue = db->graphicsQueue;

			vkGetDeviceQueue(db->device, db->computeQueueFamilyIndex, computeQueueIndex, &db->computeQueue);
			SOUL_LOG_INFO("Vulkan retrieve compute queue successful");

			vkGetDeviceQueue(db->device, db->transferQueueFamilyIndex, transferQueueIndex, &db->transferQueue);
			SOUL_LOG_INFO("Vulkan retrieve transfer queue successful");

			db->queues[QueueType::GRAPHIC] = db->graphicsQueue;
			db->queues[QueueType::COMPUTE] = db->computeQueue;
			db->queues[QueueType::TRANSFER] = db->transferQueue;

			SOUL_LOG_INFO("Vulkan device queue retrieval sucessful");

		};
		createDevice(&_db);

		static const auto createSwapchain = [this](_Database *db, uint32 swapchainWidth, uint32 swapchainHeight) {
			SOUL_LOG_INFO("createSwapchain");

			static const auto
					pickSurfaceFormat = [](VkPhysicalDevice physicalDevice,
										   VkSurfaceKHR surface) -> VkSurfaceFormatKHR {
				Memory::ScopeAllocator<> scopeAllocator("GPU::System::init::pickSurfaceFormat");
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

			static const auto pickSurfaceExtent =
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
									max(capabilities.minImageExtent.width,
										min(capabilities.maxImageExtent.width, actualExtent.width));
							actualExtent.height =
									max(capabilities.minImageExtent.height,
										min(capabilities.maxImageExtent.height, actualExtent.height));

							return actualExtent;
						}
					};
			db->swapchain.extent = pickSurfaceExtent(db->surfaceCaps, swapchainWidth, swapchainHeight);

			uint32 imageCount = db->surfaceCaps.minImageCount + 1;
			if (db->surfaceCaps.maxImageCount > 0 && imageCount > db->surfaceCaps.maxImageCount) {
				imageCount = db->surfaceCaps.maxImageCount;
			}
			SOUL_LOG_INFO("Swapchain image count = %d", imageCount);

			uint32_t queueFamily = db->presentQueueFamilyIndex;
			SOUL_ASSERT(0, db->presentQueueFamilyIndex == db->graphicsQueueFamilyIndex,
						"Currently we create swapchain with sharing mode exclusive."
						"If present and graphic queue is different we need to update sharingMode and queueFamilyIndices.");
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
			swapchainInfo.pQueueFamilyIndices = &queueFamily;
			swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
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
						{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
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
				db->swapchain.textures[i] = TextureID(db->textures.add(_Texture()));
				_Texture &texture = *_texturePtr(db->swapchain.textures[i]);
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

		SOUL_ASSERT(0,
					_db.graphicsQueueFamilyIndex == _db.presentQueueFamilyIndex,
					"Current implementation does not support different queue family for graphics and presentation yet!");
		_frameContextInit(config);

		static const auto initAllocator = [](_Database *db) {
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

			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = db->physicalDevice;
			allocatorInfo.device = db->device;
			allocatorInfo.preferredLargeHeapBlockSize = 0;
			allocatorInfo.pAllocationCallbacks = nullptr;
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
			frameContext.threadContexts.resize(config.threadCount);

			for (int j = 0; j < config.threadCount; j++) {
				_ThreadContext &threadContext = frameContext.threadContexts[j];
				threadContext = {};
			}

			for (int j = 0; j < uint64(QueueType::COUNT); j++) {
				QueueType queueType = QueueType(j);
				if (queueType == QueueType::NONE) continue;
				VkCommandPoolCreateInfo cmdPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
				cmdPoolCreateInfo.flags =
						VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				cmdPoolCreateInfo.queueFamilyIndex = _db.queueFamilyIndices[queueType];
				vkCreateCommandPool(_db.device, &cmdPoolCreateInfo, nullptr, &frameContext.commandPools[queueType]);
			}

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

	_QueueData System::_getQueueDataFromQueueFlags(QueueFlags flags) {
		_QueueData queueData;
		const uint32 queueIndexMapping[] = {
				_db.graphicsQueueFamilyIndex,
				_db.computeQueueFamilyIndex,
				_db.transferQueueFamilyIndex
		};
		Util::ForEachBit(flags, [&queueData, &queueIndexMapping](uint32 bit) {
			SOUL_ASSERT(0, bit < SOUL_ARRAY_LEN(queueIndexMapping), "");
			queueData.indices[queueData.count++] = queueIndexMapping[bit];
		});
		return queueData;
	}

	TextureID System::textureCreate(const TextureDesc &desc) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		TextureID textureID = TextureID(_db.textures.add(_Texture()));
		_Texture &texture = *_texturePtr(textureID);

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
		_QueueData queueData = _getQueueDataFromQueueFlags(desc.queueFlags);
		imageInfo.sharingMode = queueData.count == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
		imageInfo.queueFamilyIndexCount = queueData.count;
		imageInfo.pQueueFamilyIndices = queueData.indices;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		SOUL_VK_CHECK(vmaCreateImage(_db.gpuAllocator,
									 &imageInfo, &allocInfo, &texture.vkHandle,
									 &texture.allocation, nullptr), "");

		auto formatToAspectMask = [](VkFormat format) -> VkImageAspectFlags {
			switch (format) {
				case VK_FORMAT_UNDEFINED:
					return 0;

				case VK_FORMAT_S8_UINT:
					return VK_IMAGE_ASPECT_STENCIL_BIT;

				case VK_FORMAT_D16_UNORM_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
					return VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;

				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_D32_SFLOAT:
				case VK_FORMAT_X8_D24_UNORM_PACK32:
					return VK_IMAGE_ASPECT_DEPTH_BIT;

				default:
					return VK_IMAGE_ASPECT_COLOR_BIT;
			}
		};

		VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		imageViewInfo.image = texture.vkHandle;
		imageViewInfo.viewType = vkCastToImageViewType(desc.type);
		imageViewInfo.format = format;
		imageViewInfo.components = {};
		imageViewInfo.subresourceRange = {
				formatToAspectMask(format),
				0,
				1,
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
		}

		TextureID textureID = textureCreate(newDesc);
		_Texture &texture = *_texturePtr(textureID);

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
				_frameContext().stagingCommandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &beforeTransferBarrier);

		_stagingTransferToTexture(dataSize, data, desc, textureID);

		texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		texture.owner = ResourceOwner::TRANSFER_QUEUE;

		return textureID;
	}

	void System::textureDestroy(TextureID id) {
		SOUL_ASSERT_MAIN_THREAD();
		_frameContext().garbages.textures.add(id);
	}

	_Texture *System::_texturePtr(TextureID textureID) {
		return &_db.textures[textureID.id];
	}

	void System::_stagingFrameBegin() {
		SOUL_ASSERT_MAIN_THREAD();
		_FrameContext &frameContext = _frameContext();
		for (const _Buffer &buffer : frameContext.stagingBuffers) {
			vkDestroyBuffer(_db.device, buffer.vkHandle, nullptr);
		}
		_frameContext().stagingBuffers.resize(0);
	}

	void System::_stagingFrameEnd() {
		if (!_frameContext().stagingSynced) _stagingFlush();
	}

	void System::_stagingSetup() {
		SOUL_ASSERT_MAIN_THREAD();

		_FrameContext &frameContext = _frameContext();
		frameContext.stagingCommandBuffer = _queueRequestCommandBuffer(QueueType::TRANSFER);
		frameContext.stagingAvailable = true;
		frameContext.stagingSynced = false;
	}

	void System::_stagingFlush() {
		SOUL_ASSERT_MAIN_THREAD();
		_FrameContext &frameContext = _frameContext();

		_queueSubmitCommandBuffer(QueueType::TRANSFER, frameContext.stagingCommandBuffer,
								  0, nullptr,
								  VK_NULL_HANDLE);

		frameContext.stagingCommandBuffer = VK_NULL_HANDLE;
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

		_ThreadContext &threadContext = _threadContext();

		vkCmdCopyBuffer(_frameContext().stagingCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}

	_Buffer System::_stagingBufferRequest(const byte *data, uint32 size) {
		SOUL_ASSERT_MAIN_THREAD();

		SOUL_ASSERT(0, data != nullptr, "");
		_Buffer stagingBuffer = {};
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

		_ThreadContext &threadContext = _threadContext();
		_frameContext().stagingBuffers.add(stagingBuffer);

		void *mappedData;
		vmaMapMemory(_db.gpuAllocator, stagingBuffer.allocation, &mappedData);
		memcpy(mappedData, data, size);
		vmaUnmapMemory(_db.gpuAllocator, stagingBuffer.allocation);

		return stagingBuffer;
	}

	void System::_stagingTransferToBuffer(const byte *data, uint32 size, BufferID bufferID) {
		SOUL_ASSERT_MAIN_THREAD();

		_Buffer stagingBuffer = _stagingBufferRequest(data, size);
		_transferBufferToBuffer(stagingBuffer.vkHandle, _bufferPtr(bufferID)->vkHandle, size);
	}

	void
	System::_stagingTransferToTexture(uint32 size, const byte *data, const TextureDesc &desc, TextureID textureID) {
		SOUL_ASSERT_MAIN_THREAD();

		_Buffer stagingBuffer = _stagingBufferRequest(data, size);

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

		_ThreadContext &threadContext = _threadContext();

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
		_Buffer &buffer = *_bufferPtr(bufferID);

		int queueCount = 0;
		uint32 queueIndices[3] = {};
		const uint32 queueIndexMapping[] = {
				_db.graphicsQueueFamilyIndex,
				_db.computeQueueFamilyIndex,
				_db.transferQueueFamilyIndex
		};

		Util::ForEachBit(desc.queueFlags, [&queueCount, &queueIndices, &queueIndexMapping](uint32 bit) {
			SOUL_ASSERT(0, bit < SOUL_ARRAY_LEN(queueIndexMapping), "");
			queueIndices[queueCount++] = queueIndexMapping[bit];
		});

		SOUL_ASSERT(0, queueCount > 0, "");

		uint32 alignment = desc.typeSize;

		if (desc.usageFlags & BUFFER_USAGE_UNIFORM_BIT) {
			size_t minUboAlignment = _db.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
			SOUL_ASSERT(0, isPowerOfTwo(minUboAlignment), "");
			size_t dynamicAlignment = (desc.typeSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
			alignment = max(alignment, dynamicAlignment);
		}

		if (desc.usageFlags & BUFFER_USAGE_STORAGE_BIT) {
			size_t minSsboAlignment = _db.physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
			SOUL_ASSERT(0, isPowerOfTwo(minSsboAlignment), "");
			size_t dynamicAlignment = (desc.typeSize + minSsboAlignment - 1) & ~(minSsboAlignment - 1);
			alignment = max(alignment, dynamicAlignment);
		}

		VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		bufferInfo.size = alignment * desc.count;
		bufferInfo.usage = vkCastBufferUsageFlags(desc.usageFlags);
		bufferInfo.sharingMode = queueCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.queueFamilyIndexCount = queueCount;
		bufferInfo.pQueueFamilyIndices = queueIndices;

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

	_Buffer *System::_bufferPtr(BufferID bufferID) {
		return &_db.buffers[bufferID.id];
	}

	_FrameContext &System::_frameContext() {
		return _db.frameContexts[_db.currentFrame % _db.frameContexts.size()];
	}

	_ThreadContext &System::_threadContext() {
		return _frameContext().threadContexts[Job::System::Get().getThreadID()];
	}

	ShaderID System::shaderCreate(const ShaderDesc &desc, ShaderStage stage) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, desc.sourceSize > 0, "");
		SOUL_ASSERT(0, desc.source != nullptr, "");
		SOUL_ASSERT(0, desc.name != nullptr, "");

		Memory::ScopeAllocator<> scopeAllocator("shaderCreate");
		SOUL_MEMORY_ALLOCATOR_ZONE(&scopeAllocator);

		ShaderID shaderID = ShaderID(_db.shaders.add({}));
		_Shader &shader = _db.shaders[shaderID.id];

		shaderc::Compiler glslCompiler;

		uint32 shadercShaderStageMap[uint32(ShaderStage::COUNT)] = {
				0,
				shaderc_shader_kind::shaderc_glsl_vertex_shader,
				shaderc_shader_kind::shaderc_glsl_geometry_shader,
				shaderc_shader_kind::shaderc_glsl_fragment_shader,
				shaderc_shader_kind::shaderc_glsl_compute_shader
		};

		shaderc::CompileOptions options;
		shaderc::SpvCompilationResult glslCompileResult = glslCompiler.CompileGlslToSpv(
				desc.source,
				desc.sourceSize,
				(shaderc_shader_kind) shadercShaderStageMap[(uint32) stage],
				desc.name,
				options);

		std::string errorMessage = glslCompileResult.GetErrorMessage();

		SOUL_ASSERT(0, glslCompileResult.GetCompilationStatus() == shaderc_compilation_status_success,
					"Fail when compiling pass : %d, shader_type = vertex shader.\n"
					"Error : \n %s\n", desc.name, errorMessage.c_str());

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

		SOUL_ASSERT(0, resources.separate_images.empty(), "texture2D is not supported yet!");
		SOUL_ASSERT(0, resources.storage_images.empty(), "image2D is not supported yet!");
		SOUL_ASSERT(0, resources.subpass_inputs.empty(), "subpassInput is not supported yet!");
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
			return shaderID;
		}

		struct AttrExtraInfo {
			uint8 location;
			uint8 alignment;
			uint8 size;
		};

		Array<AttrExtraInfo> attrExtraInfos;
		attrExtraInfos.resize(resources.stage_inputs.size());

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
			vertexAlignment = max(vertexAlignment, attrExtraInfos[i].alignment);
		}

		uint32 vertexSize = (currentOffset + vertexAlignment - 1) & ~(vertexAlignment - 1);
		shader.inputStride = vertexSize;

		attrExtraInfos.cleanup();

		return shaderID;
	}

	void System::shaderDestroy(ShaderID shaderID) {
		_frameContext().garbages.shaders.add(shaderID);
	}

	_Shader *System::_shaderPtr(ShaderID shaderID) {
		return &_db.shaders[shaderID.id];
	}

	ProgramID System::programCreate(const GraphicBaseNode &node) {
		SOUL_ASSERT_MAIN_THREAD();
		Memory::ScopeAllocator<> scopeAllocator("GPU::System::programCreate");
		const GraphicPipelineConfig &pipelineConfig = node.pipelineConfig;

		SOUL_ASSERT(0, pipelineConfig.vertexShaderID != SHADER_ID_NULL, "");
		SOUL_ASSERT(0, pipelineConfig.fragmentShaderID != SHADER_ID_NULL, "");

		_FrameContext &frameContext = _frameContext();

		ProgramID programID = ProgramID(_db.programs.add({}));
		_Program &program = _db.programs[programID.id];

		ShaderID shaderIDs[uint32(ShaderStage::COUNT)] = {};
		shaderIDs[uint32(ShaderStage::VERTEX)] = pipelineConfig.vertexShaderID;
		shaderIDs[uint32(ShaderStage::FRAGMENT)] = pipelineConfig.fragmentShaderID;

		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			for (int j = 0; j < uint32(ShaderStage::COUNT); j++) {
				ShaderID shaderID = shaderIDs[j];
				if (shaderID == SHADER_ID_NULL) continue;
				const _Shader &shader = *_shaderPtr(shaderID);

				for (int k = 0; k < MAX_BINDING_PER_SET; k++) {

					VkShaderStageFlags vulkanShaderStageBitsMap[uint64(ShaderStage::COUNT)] = {
							0,
							VK_SHADER_STAGE_VERTEX_BIT,
							VK_SHADER_STAGE_GEOMETRY_BIT,
							VK_SHADER_STAGE_FRAGMENT_BIT,
							VK_SHADER_STAGE_COMPUTE_BIT
					};

					VkPipelineStageFlags vulkanPipelineStageBitsMap[uint32(ShaderStage::COUNT)] = {
							0,
							VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
							VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					};

					_ProgramDescriptorBinding &progBinding = program.bindings[i][k];
					const _ShaderDescriptorBinding &shaderBinding = shader.bindings[i][k];

					if (shader.bindings[i][k].count == 0) continue;

					if (progBinding.shaderStageFlags == 0) {
						progBinding.type = shaderBinding.type;
						progBinding.count = shaderBinding.count;
					}
					else {
						SOUL_ASSERT(0, program.bindings[i][k].type == shader.bindings[i][k].type, "");
						SOUL_ASSERT(0, program.bindings[i][k].count == shader.bindings[i][k].count, "");
					}
					progBinding.shaderStageFlags |= vulkanShaderStageBitsMap[j];
					progBinding.pipelineStageFlags |= vulkanPipelineStageBitsMap[j];
				}
			}
		}

		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			Array<VkDescriptorSetLayoutBinding> bindings(&scopeAllocator);
			for (int j = 0; j < MAX_BINDING_PER_SET; j++) {
				const _ProgramDescriptorBinding &progBinding = program.bindings[i][j];
				if (progBinding.shaderStageFlags == 0) continue;
				bindings.add({});

				bindings.back().stageFlags = progBinding.shaderStageFlags;
				bindings.back().descriptorType = vkCast(progBinding.type);
				bindings.back().descriptorCount = progBinding.count;
				bindings.back().binding = j;
			}
			VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();
			SOUL_VK_CHECK(
					vkCreateDescriptorSetLayout(_db.device, &layoutInfo, nullptr, &program.descriptorLayouts[i]),
					"");
			bindings.cleanup();
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount = MAX_SET_PER_SHADER_PROGRAM;
		pipelineLayoutInfo.pSetLayouts = program.descriptorLayouts;

		SOUL_VK_CHECK(vkCreatePipelineLayout(_db.device, &pipelineLayoutInfo, nullptr, &program.pipelineLayout),
					  "");

		return programID;
	}

	void System::programDestroy(ProgramID programID) {
		SOUL_ASSERT_MAIN_THREAD();
		_frameContext().garbages.programs.add(programID);
	}

	_Program *System::_programPtr(ProgramID programID) {
		return &_db.programs[programID.id];
	}

	VkPipeline System::_pipelineCreate(const GraphicBaseNode &node, ProgramID programID, VkRenderPass renderPass) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		Memory::ScopeAllocator<> scopeAllocator("GPU::System::_pipelineCreate");
		const GraphicPipelineConfig &pipelineConfig = node.pipelineConfig;

		Array<VkPipelineShaderStageCreateInfo> shaderStageInfos(&scopeAllocator);
		const _Shader &vertShader = *_shaderPtr(pipelineConfig.vertexShaderID);
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		vertShaderStageInfo.module = vertShader.module;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.pName = "main";
		shaderStageInfos.add(vertShaderStageInfo);

		const _Shader &fragShader = *_shaderPtr(pipelineConfig.fragmentShaderID);
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		fragShaderStageInfo.module = fragShader.module;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.pName = "main";
		shaderStageInfos.add(fragShaderStageInfo);

		static VkPrimitiveTopology primitiveTopologyMap[(uint32) Topology::COUNT] = {
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
		};
		VkPipelineInputAssemblyStateCreateInfo
				inputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
		inputAssemblyState.topology = primitiveTopologyMap[(uint32) pipelineConfig.inputLayout.topology];
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = pipelineConfig.viewport.offsetX;
		viewport.y = pipelineConfig.viewport.offsetY;
		viewport.width = pipelineConfig.viewport.width;
		viewport.height = pipelineConfig.viewport.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = {int32(pipelineConfig.scissor.offsetX), int32(pipelineConfig.scissor.offsetY)};
		scissor.extent = {uint32(pipelineConfig.scissor.width), uint32(pipelineConfig.scissor.height)};

		VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		static VkPolygonMode polygonModeMap[(uint32) PolygonMode::COUNT] = {
				VK_POLYGON_MODE_FILL,
				VK_POLYGON_MODE_LINE,
				VK_POLYGON_MODE_POINT
		};

		VkPipelineRasterizationStateCreateInfo rasterizerState = {
				VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
		rasterizerState.depthClampEnable = VK_FALSE;
		rasterizerState.rasterizerDiscardEnable = VK_FALSE;
		rasterizerState.polygonMode = polygonModeMap[(uint32) pipelineConfig.raster.polygonMode];
		rasterizerState.lineWidth = pipelineConfig.raster.lineWidth;

		// TODO(kevinyu) : Enable multisampling configuration
		VkPipelineMultisampleStateCreateInfo multisampleState = {
				VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};

		VkVertexInputAttributeDescription attrDescs[MAX_INPUT_PER_SHADER];
		uint32 attrDescCount = 0;
		for (int i = 0; i < MAX_INPUT_PER_SHADER; i++) {
			if (vertShader.inputs[i].format == VK_FORMAT_UNDEFINED) continue;
			attrDescs[attrDescCount].format = vertShader.inputs[i].format;
			attrDescs[attrDescCount].binding = 0;
			attrDescs[attrDescCount].location = i;
			attrDescs[attrDescCount].offset = vertShader.inputs[i].offset;
			attrDescCount++;
		}

		VkVertexInputBindingDescription inputBindingDesc = {};
		inputBindingDesc.binding = 0;
		inputBindingDesc.stride = vertShader.inputStride;
		inputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkPipelineVertexInputStateCreateInfo inputStateInfo = {
				VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
		inputStateInfo.vertexAttributeDescriptionCount = attrDescCount;
		inputStateInfo.pVertexAttributeDescriptions = attrDescs;
		inputStateInfo.vertexBindingDescriptionCount = 1;
		inputStateInfo.pVertexBindingDescriptions = &inputBindingDesc;

		VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
		pipelineInfo.stageCount = shaderStageInfos.size();
		pipelineInfo.pStages = shaderStageInfos.data();
		pipelineInfo.pVertexInputState = &inputStateInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerState;
		pipelineInfo.pMultisampleState = &multisampleState;

		VkPipelineColorBlendAttachmentState colorBlendAttachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
		for (int i = 0; i < node.colorAttachments.size(); i++) {
			VkPipelineColorBlendAttachmentState &blendState = colorBlendAttachments[i];
			const ColorAttachment &attachment = node.colorAttachments[i];
			blendState.blendEnable = attachment.desc.blendEnable ? VK_TRUE : VK_FALSE;
			blendState.srcColorBlendFactor = vkCast(attachment.desc.srcColorBlendFactor);
			blendState.dstColorBlendFactor = vkCast(attachment.desc.dstColorBlendFactor);
			blendState.colorBlendOp = vkCast(attachment.desc.colorBlendOp);
			blendState.srcAlphaBlendFactor = vkCast(attachment.desc.srcAlphaBlendFactor);
			blendState.dstAlphaBlendFactor = vkCast(attachment.desc.dstAlphaBlendFactor);
			blendState.alphaBlendOp = vkCast(attachment.desc.alphaBlendOp);
			blendState.colorWriteMask =
					VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
					VK_COLOR_COMPONENT_A_BIT;
		}
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = node.colorAttachments.size();
		colorBlending.pAttachments = colorBlendAttachments;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;
		pipelineInfo.pColorBlendState = &colorBlending;

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
				VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
		depthStencilInfo.depthTestEnable = node.depthStencilAttachment.desc.depthTestEnable;
		depthStencilInfo.depthWriteEnable = node.depthStencilAttachment.desc.depthWriteEnable;
		depthStencilInfo.depthCompareOp = vkCast(node.depthStencilAttachment.desc.depthCompareOp);
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilInfo.stencilTestEnable = VK_FALSE;
		depthStencilInfo.front = {};
		depthStencilInfo.back = {};
		depthStencilInfo.minDepthBounds = 0.0f;
		depthStencilInfo.maxDepthBounds = 1.0f;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;

		if (pipelineConfig.scissor.dynamic) {
			VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
					VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
			VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_SCISSOR};
			dynamicStateInfo.dynamicStateCount = 1;
			dynamicStateInfo.pDynamicStates = dynamicStates;
			pipelineInfo.pDynamicState = &dynamicStateInfo;
		}

		pipelineInfo.layout = _programPtr(programID)->pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VkPipeline pipeline;
		SOUL_VK_CHECK(vkCreateGraphicsPipelines(_db.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
					  "");

		shaderStageInfos.cleanup();

		return pipeline;
	}

	void System::_pipelineDestroy(VkPipeline pipeline) {
		SOUL_ASSERT(0, pipeline != VK_NULL_HANDLE, "");
		_frameContext().garbages.pipelines.add(pipeline);
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
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		VkSampler sampler;
		SOUL_VK_CHECK(vkCreateSampler(_db.device, &samplerCreateInfo, nullptr, &sampler), "");
		_db.samplerMap.add(hashKey, sampler);

		return SamplerID(sampler);
	}

	ShaderArgSetID System::_shaderArgSetRequest(const ShaderArgSetDesc &desc, ProgramID programID, uint32 set) {

		uint64 hash = 0;
		int offsetCount = 0;
		int offset[MAX_DYNAMIC_BUFFER_PER_SET];
		for (int i = 0; i < desc.bindingCount; i++) {
			Descriptor &descriptorDesc = desc.bindingDescriptions[i];
			switch (descriptorDesc.type) {
				case DescriptorType::UNIFORM_BUFFER: {
					const UniformDescriptor &uniformDescriptor = descriptorDesc.uniformInfo;
					hash = hashFNV1((uint8 *) &descriptorDesc.type, sizeof(descriptorDesc.type), hash);
					hash = hashFNV1((uint8 *) &descriptorDesc.uniformInfo.bufferID, sizeof(BufferID), hash);
					offset[offsetCount++] =
							uniformDescriptor.unitIndex * _bufferPtr(uniformDescriptor.bufferID)->unitSize;
					break;
				}
				case DescriptorType::SAMPLED_IMAGE: {
					hash = hashFNV1((uint8 *) &descriptorDesc.type, sizeof(descriptorDesc.type), hash);
					hash = hashFNV1((uint8 *) &descriptorDesc.sampledImageInfo.textureID, sizeof(TextureID), hash);
					hash = hashFNV1((uint8 *) &descriptorDesc.sampledImageInfo.samplerID, sizeof(SamplerID), hash);
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
			} else {
				VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
				allocInfo.descriptorPool = _frameContext().descriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &(_programPtr(programID)->descriptorLayouts[set]);

				SOUL_VK_CHECK(vkAllocateDescriptorSets(_db.device, &allocInfo, &descriptorSet), "");

				for (int i = 0; i < desc.bindingCount; i++) {
					VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
					descriptorWrite.dstSet = descriptorSet;
					descriptorWrite.dstBinding = i;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorCount = 1;

					Descriptor &descriptorDesc = desc.bindingDescriptions[i];
					descriptorWrite.descriptorType = DESCRIPTOR_TYPE_MAP[uint64(descriptorDesc.type)];

					VkDescriptorBufferInfo bufferInfo;
					VkDescriptorImageInfo imageInfo;
					switch (descriptorDesc.type) {
						case DescriptorType::UNIFORM_BUFFER: {
							bufferInfo.buffer = _bufferPtr(descriptorDesc.uniformInfo.bufferID)->vkHandle;
							bufferInfo.offset = 0;
							bufferInfo.range = VK_WHOLE_SIZE;
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
						default: {
							SOUL_NOT_IMPLEMENTED();
							break;
						}
					}

					vkUpdateDescriptorSets(_db.device, 1, &descriptorWrite, 0, nullptr);
				}
				_db.descriptorSets.add(hash, descriptorSet);
			}
			argSetID = ShaderArgSetID(_db.shaderArgSets.add(_ShaderArgSet()));

		}

		_ShaderArgSet &argSet = _db.shaderArgSets[argSetID.id];

		argSet.vkHandle = descriptorSet;
		argSet.offsetCount = offsetCount;
		for (int i = 0; i < offsetCount; i++) {
			argSet.offset[i] = offset[i];
		}

		return argSetID;
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
		SemaphoreID semaphoreID = SemaphoreID(_db.semaphores.add(_Semaphore()));
		_Semaphore &semaphore = _db.semaphores[semaphoreID.id];
		VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		SOUL_VK_CHECK(vkCreateSemaphore(_db.device, &semaphoreInfo, nullptr, &semaphore.vkHandle), "");
		return semaphoreID;
	}

	void System::_semaphoreReset(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		_semaphorePtr(ID)->stageFlags = 0;
	}

	_Semaphore *System::_semaphorePtr(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		return &_db.semaphores[ID.id];
	}

	void System::_semaphoreDestroy(SemaphoreID ID) {
		SOUL_ASSERT_MAIN_THREAD();
		_frameContext().garbages.semaphores.add(ID);
	}

	void System::_queueFlush(QueueType queueType,
							 uint32 semaphoreCount, SemaphoreID *semaphoreID,
							 VkFence fence) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();

		VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		_Submission &submission = _db.submissions[queueType];
		submitInfo.commandBufferCount = submission.commands.size();
		submitInfo.pCommandBuffers = submission.commands.data();
		SOUL_ASSERT(0, submission.waitSemaphores.size() == submission.waitStages.size(), "");
		submitInfo.waitSemaphoreCount = submission.waitSemaphores.size();
		submitInfo.pWaitSemaphores = submission.waitSemaphores.data();
		submitInfo.pWaitDstStageMask = submission.waitStages.data();

		VkSemaphore signalSemaphores[MAX_SIGNAL_SEMAPHORE];
		for (int i = 0; i < semaphoreCount; i++) {
			signalSemaphores[i] = _semaphorePtr(semaphoreID[i])->vkHandle;
		}

		submitInfo.signalSemaphoreCount = semaphoreCount;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkQueueSubmit(_db.queues[queueType], 1, &submitInfo, fence);

		submission.commands.resize(0);
		submission.waitSemaphores.resize(0);
		submission.waitStages.resize(0);
	}

	VkCommandBuffer System::_queueRequestCommandBuffer(QueueType queueType) {
		SOUL_ASSERT_MAIN_THREAD();

		_FrameContext &frameContext = _frameContext();

		Array<VkCommandBuffer> &cmdBuffers = frameContext.commandBuffers[queueType];

		if (frameContext.usedCommandBuffers[queueType] == cmdBuffers.size()) {
			VkCommandBuffer cmdBuffer;
			VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			allocInfo.commandPool = frameContext.commandPools[queueType];
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			SOUL_VK_CHECK(vkAllocateCommandBuffers(_db.device, &allocInfo, &cmdBuffer), "");
			cmdBuffers.add(cmdBuffer);
		}

		VkCommandBuffer cmdBuffer = cmdBuffers[frameContext.usedCommandBuffers[queueType]];

		VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmdBuffer, &beginInfo);

		frameContext.usedCommandBuffers[queueType]++;

		return cmdBuffer;
	}

	void System::_queueSubmitCommandBuffer(QueueType queueType, VkCommandBuffer commandBuffer,
										   uint32 semaphoreCount, SemaphoreID *semaphoreID, VkFence fence) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, semaphoreCount <= MAX_SIGNAL_SEMAPHORE, "");
		SOUL_PROFILE_ZONE();

		vkEndCommandBuffer(commandBuffer);

		_Submission &submission = _db.submissions[queueType];
		_db.submissions[queueType].commands.add(commandBuffer);

		// TODO : Fix this
		if (semaphoreCount != 0 || fence != VK_NULL_HANDLE) {
			_queueFlush(queueType, semaphoreCount, semaphoreID, fence);
		}

	}

	void System::_queueWaitSemaphore(QueueType queueType, SemaphoreID ID, VkPipelineStageFlags waitStages) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_ASSERT(0, queueType != QueueType::NONE, "");
		SOUL_ASSERT(0, waitStages != 0, "");

		_Semaphore *semaphore = _semaphorePtr(ID);
		if ((waitStages & semaphore->stageFlags) == waitStages) {
			return;
		}

		_Submission &submission = _db.submissions[queueType];
		bool found = false;
		for (int i = 0; i < submission.waitSemaphores.size(); i++) {
			if (submission.waitSemaphores[i] == semaphore->vkHandle) {
				found = true;
				submission.waitStages[i] |= waitStages;
			}
		}

		if (!found) {
			if (submission.commands.size() != 0) {
				_queueFlush(queueType, 0, nullptr, VK_NULL_HANDLE);
			}
			submission.waitSemaphores.add(semaphore->vkHandle);
			submission.waitStages.add(waitStages);
		}

		semaphore->stageFlags |= waitStages;
	}

	void System::renderGraphExecute(const RenderGraph &renderGraph) {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		if (!_frameContext().stagingSynced) _stagingFlush();

		Memory::ScopeAllocator<> scopeAllocator("renderGraphExecute");

		_RenderGraphExecution execution(&renderGraph, this, &scopeAllocator);
		execution.init();
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

		SOUL_LOG_INFO("Frame begin()");

		_FrameContext &frameContext = _frameContext();
		vkWaitForFences(_db.device, 1, &frameContext.fence, VK_TRUE, UINT64_MAX);

		for (int i = 0; i < uint64(QueueType::COUNT); i++) {
			QueueType queueType = QueueType(i);
			if (queueType == QueueType::NONE) continue;
			vkResetCommandPool(_db.device, frameContext.commandPools[queueType], 0);
		}
		for (uint16 &usedCommandBuffer : frameContext.usedCommandBuffers) {
			usedCommandBuffer = 0;
		}

		_stagingFrameBegin();

		_semaphoreReset(frameContext.imageAvailableSemaphore);
		_semaphoreReset(frameContext.renderFinishedSemaphore);

		uint32 swapchainIndex;
		vkAcquireNextImageKHR(_db.device, _db.swapchain.vkHandle, UINT64_MAX,
							  _semaphorePtr(frameContext.imageAvailableSemaphore)->vkHandle, VK_NULL_HANDLE,
							  &swapchainIndex);
		frameContext.swapchainIndex = swapchainIndex;
		TextureID swapchainTextureID = _db.swapchain.textures[swapchainIndex];
		_Texture *swapchainTexture = _texturePtr(swapchainTextureID);
		swapchainTexture->owner = ResourceOwner::PRESENTATION_ENGINE;
		swapchainTexture->layout = VK_IMAGE_LAYOUT_UNDEFINED;

		vkResetDescriptorPool(_db.device, frameContext.descriptorPool, 0);

		for (TextureID textureID : frameContext.garbages.textures) {
			_Texture &texture = *_texturePtr(textureID);
			vmaDestroyImage(_db.gpuAllocator, texture.vkHandle, texture.allocation);
		}
		frameContext.garbages.textures.resize(0);

		for (BufferID bufferID : frameContext.garbages.buffers) {
			_Buffer &buffer = *_bufferPtr(bufferID);
			vmaDestroyBuffer(_db.gpuAllocator, buffer.vkHandle, buffer.allocation);
		}
		frameContext.garbages.buffers.resize(0);

		for (ShaderID shaderID : frameContext.garbages.shaders) {
			_Shader &shader = _db.shaders[shaderID.id];
			vkDestroyShaderModule(_db.device, shader.module, nullptr);
			_db.shaders.remove(shaderID.id);
		}
		frameContext.garbages.shaders.resize(0);

		for (ProgramID programID : frameContext.garbages.programs) {
			_Program program = _db.programs[programID.id];
			vkDestroyPipelineLayout(_db.device, program.pipelineLayout, nullptr);
			for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
				vkDestroyDescriptorSetLayout(_db.device, program.descriptorLayouts[i], nullptr);
			}
			_db.programs.remove(programID.id);
		}
		frameContext.garbages.programs.resize(0);

		for (VkRenderPass renderPass: frameContext.garbages.renderPasses) {
			vkDestroyRenderPass(_db.device, renderPass, nullptr);
		}
		frameContext.garbages.renderPasses.resize(0);

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

		_db.shaderArgSets.resize(1);
	}

	void System::_frameEnd() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		_FrameContext &frameContext = _frameContext();
		uint32 swapchainIndex = frameContext.swapchainIndex;
		if (_db.swapchain.fences[swapchainIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(_db.device, 1, &_db.swapchain.fences[swapchainIndex], VK_TRUE, UINT64_MAX);
		}
		_db.swapchain.fences[swapchainIndex] = frameContext.fence;

		_Texture &swapchainTexture = *_texturePtr(_db.swapchain.textures[swapchainIndex]);
		vkResetFences(_db.device, 1, &frameContext.fence);
		{
			SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::LastSubmission");
			_stagingFrameEnd();

			SOUL_ASSERT(0,
						swapchainTexture.owner == ResourceOwner::GRAPHIC_QUEUE
						|| swapchainTexture.owner == ResourceOwner::PRESENTATION_ENGINE,
						"");
			// TODO: Handle when swapchain texture is untouch (ResourceOwner is PRESENTATION_ENGINE)
			VkCommandBuffer cmdBuffer = _queueRequestCommandBuffer(QueueType::GRAPHIC);

			VkImageMemoryBarrier imageBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
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

			EnumArray<ResourceOwner, QueueType> RESOURCE_ONWER_TO_QUEUE_TYPE({
																					 QueueType::NONE,
																					 QueueType::GRAPHIC,
																					 QueueType::COMPUTE,
																					 QueueType::TRANSFER,
																					 QueueType::GRAPHIC
																			 });

			auto _syncQueueToGraphic = [this](QueueType queueType) {
				SemaphoreID semaphoreID = _semaphoreCreate();
				_queueWaitSemaphore(QueueType::GRAPHIC, semaphoreID, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
				VkCommandBuffer cmdBuffer = _queueRequestCommandBuffer(queueType);
				_queueSubmitCommandBuffer(queueType, cmdBuffer, 1, &semaphoreID, VK_NULL_HANDLE);
				_semaphoreDestroy(semaphoreID);
			};

			{
				SOUL_PROFILE_ZONE_WITH_NAME("Sync transfer");
				_syncQueueToGraphic(QueueType::TRANSFER);
			}

			{
				SOUL_PROFILE_ZONE_WITH_NAME("Sync compute");
				_syncQueueToGraphic(QueueType::COMPUTE);
			}

			if (swapchainTexture.owner == ResourceOwner::PRESENTATION_ENGINE) {
				_queueWaitSemaphore(QueueType::GRAPHIC,
									_frameContext().imageAvailableSemaphore,
									VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			}

			_queueSubmitCommandBuffer(RESOURCE_ONWER_TO_QUEUE_TYPE[swapchainTexture.owner], cmdBuffer,
									  1, &_frameContext().renderFinishedSemaphore, frameContext.fence);
			swapchainTexture.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &_semaphorePtr(frameContext.renderFinishedSemaphore)->vkHandle;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &_db.swapchain.vkHandle;
		presentInfo.pImageIndices = &frameContext.swapchainIndex;

		{
			SOUL_PROFILE_ZONE_WITH_NAME("GPU::System::QueuePresent");
			vkQueuePresentKHR(_db.presentQueue, &presentInfo);
		}

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

	}
}