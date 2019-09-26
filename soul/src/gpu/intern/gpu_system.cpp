#include "gpu/system.h"
#include "gpu/render_graph.h"

#include "job/system.h"

#include "core/array.h"
#include "core/math.h"
#include "core/debug.h"

#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include <VulkanMemoryAllocator/src/vk_mem_alloc.h>

#include <cstddef>
#include <limits>

namespace Soul { namespace GPU {

	static VkFormat FORMAT_MAP[uint64(TextureFormat::COUNT)] = {
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_UNDEFINED,

		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,

		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALL _debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		SOUL_LOG_INFO("_debugCallback");
		switch(messageSeverity) {
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

		return VK_FALSE;
	}

	void System::init(const Config& config) {

		SOUL_ASSERT(0, config.windowHandle != nullptr, "Invalid configuration value | windowHandle = nullptr");
		SOUL_ASSERT(0, config.threadCount > 0, "Invalid configuration value | threadCount = %d", config.threadCount);
		SOUL_ASSERT(0, config.maxFrameInFlight > 0, "Invalid configuration value | maxFrameInFlight = %d", config.maxFrameInFlight);
		
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

		static const char* requiredExtensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			
			#ifdef SOUL_OS_WIN32
			"VK_KHR_win32_surface",
			#endif // SOUL_PLATFORM_OS_WIN32
			
			#ifdef SOUL_OS_APPLE
			"VK_MVK_macos_surface",
			#endif // SOUL_PLATFORM_OS_APPLE
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		static const uint32_t requiredExtensionCount = sizeof(requiredExtensions)/sizeof(requiredExtensions[0]);
		instanceCreateInfo.enabledExtensionCount = requiredExtensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions;

		static const char* requiredLayers[] = {
			
			#ifdef SOUL_OPTION_VULKAN_ENABLE_VALIDATION
			"VK_LAYER_KHRONOS_validation",
			#endif // SOUL_OPTION_VULKAN_ENABLE_VALIDATION
			
			#ifdef SOUL_OPTION_VULKAN_ENABLE_RENDERDOC
			"VK_LAYER_RENDERDOC_Capture",
			#endif // SOUL_OPTION_VULKAN_ENABLE_RENDERDOC			
		
		};
		static const uint32_t requiredLayerCount = sizeof(requiredLayers) / sizeof(requiredLayers[0]);

		static const auto checkLayerSupport = [](int count, const char** layers) -> bool {
			SOUL_LOG_INFO("Check vulkan layer support.");
			uint32 layerCount;

			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			Array<VkLayerProperties> availableLayers;
			availableLayers.reserve(layerCount);

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
		SOUL_VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &_db.instance), "Vulkan instance creation fail!");
		SOUL_LOG_INFO("Vulkan instance creation succesful");

		volkLoadInstance(_db.instance);

		static const auto createDebugUtilsMessenger = [](Database* db) {
			SOUL_LOG_INFO("Creating vulkan debug utils messenger");
			VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
			debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugMessengerCreateInfo.pfnUserCallback = _debugCallback;
			debugMessengerCreateInfo.pUserData = nullptr;
			SOUL_VK_CHECK(vkCreateDebugUtilsMessengerEXT(db->instance, &debugMessengerCreateInfo, nullptr, &db->debugMessenger), "Vulkan debug messenger creation fail!");
			SOUL_LOG_INFO("Vulkan debug messenger creation sucesfull");
		};
		createDebugUtilsMessenger(&_db);

		_surfaceCreate(config.windowHandle, &_db.surface);

		static const auto pickPhysicalDevice = [](Database* db, int requiredExtensionCount, const char* requiredExtensions[]) {
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
					i, physicalDeviceProperties.deviceName, vendorID, deviceID, apiVersion, driverVersion);

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
				vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], db->surface, &presentModeCount, presentModes.data());

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

					const VkQueueFamilyProperties& queueProps = queueFamilies[j];
					VkQueueFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
					if (presentSupport && queueProps.queueCount > 0 && (queueProps.queueFlags & requiredFlags) == requiredFlags) {
						graphicsQueueFamilyIndex = j;
						presentQueueFamilyIndex = j;
					}
				}

				if (graphicsQueueFamilyIndex == -1) {
					for (int j = 0; j < queueFamilyCount; j++) {
						const VkQueueFamilyProperties& queueProps = queueFamilies[j];
						VkQueueFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
						if (queueProps.queueCount > 0 && (queueProps.queueFlags & requiredFlags) == requiredFlags) {
							graphicsQueueFamilyIndex = j;
							break;
						}
					}	
				}

				for (int j = 0; j < queueFamilyCount; j++) {
					for (int j = 0; j < queueFamilyCount; j++) {
						const VkQueueFamilyProperties& queueProps = queueFamilies[j];
						VkQueueFlags requiredFlags = VK_QUEUE_COMPUTE_BIT;
						if (j != graphicsQueueFamilyIndex && queueProps.queueCount > 0 && (queueProps.queueFlags & requiredFlags) == requiredFlags) {
							computeQueueFamilyIndex = j;
							break;
						}
					}
				}
				if (computeQueueFamilyIndex == -1) computeQueueFamilyIndex = graphicsQueueFamilyIndex;

				for (int j = 0; j < queueFamilyCount; j++) {
					for (int j = 0; j < queueFamilyCount; j++) {
						const VkQueueFamilyProperties& queueProps = queueFamilies[j];
						VkQueueFlags requiredFlags = VK_QUEUE_TRANSFER_BIT;
						if (j != graphicsQueueFamilyIndex && j != computeQueueFamilyIndex && (queueProps.queueFlags & requiredFlags) == requiredFlags) {
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

			SOUL_ASSERT(0, db->physicalDevice != VK_NULL_HANDLE, "Cannot find physical device that satisfy the requirements.");

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
				db->physicalDeviceProperties.deviceName, vendorID, deviceID, apiVersion, driverVersion,
				db->graphicsQueueFamilyIndex, db->presentQueueFamilyIndex, db->transferQueueFamilyIndex, db->computeQueueFamilyIndex);

			formats.cleanup();
			presentModes.cleanup();
			devices.cleanup();
			
			SOUL_ASSERT(0, db->physicalDevice != VK_NULL_HANDLE, "Fail to find a suitable GPU!");
		};
		pickPhysicalDevice(&_db, requiredExtensionCount, requiredExtensions);
		

		static const auto createDevice = [](Database* db) {
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

			// TODO(kevinyu): THis is temporary hack to avoid molten vk bug when queue family is not sorted in queueCreateInfo
			queueCreateInfo[3] = queueCreateInfo[0];
			queueCreateInfo[0] = queueCreateInfo[1];
			queueCreateInfo[1] = queueCreateInfo[2];
			queueCreateInfo[2] = queueCreateInfo[3];

			VkPhysicalDeviceFeatures deviceFeatures = {};

			VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
			deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
			deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

			static const char* deviceExtensions[] = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			const uint32 deviceExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);

			deviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

			SOUL_ASSERT(0, db->graphicsQueueFamilyIndex == db->presentQueueFamilyIndex, "Different queue for graphics and present not supported yet!");

			SOUL_VK_CHECK(vkCreateDevice(db->physicalDevice, &deviceCreateInfo, nullptr, &db->device), "Vulkan logical device creation fail!");
			SOUL_LOG_INFO("Vulkan logical device creation sucessful");

			vkGetDeviceQueue(db->device, db->graphicsQueueFamilyIndex, graphicsQueueIndex, &db->graphicsQueue);
			SOUL_LOG_INFO("Vulkan retrieve graphics queue successful");

			vkGetDeviceQueue(db->device, db->computeQueueFamilyIndex, computeQueueIndex, &db->computeQueue);
			SOUL_LOG_INFO("Vulkan retrieve compute queue successful");

			vkGetDeviceQueue(db->device, db->transferQueueFamilyIndex, transferQueueIndex, &db->transferQueue);
			SOUL_LOG_INFO("Vulkan retrieve transfer queue successful");

			SOUL_LOG_INFO("Vulkan device queue retrieval sucessful");

		};
		createDevice(&_db);

		static const auto createSwapchain = [](Database* db, uint32 swapchainWidth, uint32 swapchainHeight) {
			SOUL_LOG_INFO("createSwapchain");
			static const auto pickSurfaceFormat = [](VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) -> VkSurfaceFormatKHR {
				SOUL_LOG_INFO("Picking surface format.");
				Array<VkSurfaceFormatKHR> formats;
				uint32 formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
				SOUL_ASSERT(0, formatCount != 0, "Surface format count is zero!");
				formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

				for (int i = 0; i < formats.size(); i++) {
					const VkSurfaceFormatKHR& surfaceFormat = formats.get(i);
					if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
						formats.cleanup();
						return surfaceFormat;
					}
				}
				VkSurfaceFormatKHR format = formats.get(0);
				formats.cleanup();
				return format;
			};
			db->swapchain.format = pickSurfaceFormat(db->physicalDevice, db->surface);
			
			static const auto pickSurfaceExtent = [](VkSurfaceCapabilitiesKHR capabilities, uint32 swapchainWidth, uint32 swapchainHeight) -> VkExtent2D {
				SOUL_LOG_INFO("Picking vulkan swap extent");
				if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
					SOUL_LOG_INFO("vulkanPickSwapExtent | Extent = %d %d", capabilities.currentExtent.width, capabilities.currentExtent.height);
					return capabilities.currentExtent;
				} else {
					VkExtent2D actualExtent = {static_cast<uint32>(swapchainWidth), static_cast<uint32>(swapchainHeight)};

					actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
					actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));

					return actualExtent;
				}
			};
			db->swapchain.extent = pickSurfaceExtent(db->surfaceCaps, swapchainWidth, swapchainHeight);

			uint32 imageCount = db->surfaceCaps.minImageCount + 1;
			if (db->surfaceCaps.maxImageCount > 0 && imageCount > db->surfaceCaps.maxImageCount) {
				imageCount = db->surfaceCaps.maxImageCount;
			}
			SOUL_LOG_INFO("Swapchain image count = %d", imageCount);

			 VkSwapchainCreateInfoKHR swapchainInfo {
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = db->surface,
				.minImageCount = imageCount,
				.imageFormat = db->swapchain.format.format,
				.imageColorSpace = db->swapchain.format.colorSpace,
				.imageExtent = db->swapchain.extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.presentMode = VK_PRESENT_MODE_FIFO_KHR,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.clipped = VK_TRUE
			};
			SOUL_VK_CHECK(vkCreateSwapchainKHR(db->device, &swapchainInfo, nullptr, &db->swapchain.vkID), "Fail to create vulkan swapchain!");

			vkGetSwapchainImagesKHR(db->device, db->swapchain.vkID, &imageCount, nullptr);
			db->swapchain.images.resize(imageCount);
			vkGetSwapchainImagesKHR(db->device, db->swapchain.vkID, &imageCount, db->swapchain.images.data());
			
			db->swapchain.imageViews.resize(imageCount);
			for (int i = 0; i < imageCount; i++) {
				VkImageViewCreateInfo createInfo {
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = db->swapchain.images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = db->swapchain.format.format,
					.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
					.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.subresourceRange.baseMipLevel = 0,
					.subresourceRange.levelCount = 1,
					.subresourceRange.baseArrayLayer = 0,
					.subresourceRange.layerCount = 1
				};
				SOUL_VK_CHECK(vkCreateImageView(db->device, &createInfo, nullptr, &db->swapchain.imageViews[i]), "Fail to creat swapchain imageview %d", i);
			}

			SOUL_LOG_INFO("Vulkan swapchain creation sucessful");
			
		};
		createSwapchain(&_db, config.swapchainWidth, config.swapchainHeight);

		SOUL_ASSERT(0, _db.graphicsQueueFamilyIndex == _db.presentQueueFamilyIndex, "Current implementation does not support different queue family for graphics and presentation yet!");
		_db.frameContexts.resize(config.maxFrameInFlight);
		for (int i = 0; i < _db.frameContexts.size(); i++) {
			FrameContext& frameContext = _db.frameContexts[i];
			frameContext = {};
			frameContext.threadContexts.resize(config.threadCount);
			
			for (int j = 0; j < config.threadCount; j++) {

				ThreadContext& threadContext = frameContext.threadContexts[j];
				threadContext = {};

				VkCommandPoolCreateInfo createInfo = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
					.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
					.queueFamilyIndex = _db.graphicsQueueFamilyIndex
				};

				vkCreateCommandPool(_db.device, &createInfo, nullptr, &threadContext.graphicsCommandPool);
				vkCreateCommandPool(_db.device, &createInfo, nullptr, &threadContext.transferCommandPool);
				vkCreateCommandPool(_db.device, &createInfo, nullptr, &threadContext.computeCommandPool);

				VkCommandBufferAllocateInfo allocInfo = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					.commandPool = threadContext.transferCommandPool,
					.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					.commandBufferCount = (uint32_t) 1
				};
 
				SOUL_VK_CHECK(vkAllocateCommandBuffers(_db.device, &allocInfo, &threadContext.stagingCommandBuffer), "Allocate staging command buffer fail!");
			}
		}

		static const auto initAllocator = [](Database* db) {
			VmaVulkanFunctions vulkanFunctions = {
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
				.vkBindImageMemory2KHR = vkBindImageMemory2KHR
			};
			VmaAllocatorCreateInfo allocatorInfo = {
				.physicalDevice = db->physicalDevice,
				.device = db->device,
				.preferredLargeHeapBlockSize = 0,
				.pAllocationCallbacks = nullptr,
				.pDeviceMemoryCallbacks = nullptr,
				.frameInUseCount = 0,
				.pHeapSizeLimit = 0,
				.pVulkanFunctions = &vulkanFunctions
			};
		
			vmaCreateAllocator(&allocatorInfo, &db->allocator);

			SOUL_LOG_INFO("Vulkan init allocator sucessful");
		};
		initAllocator(&_db);
	
	}

	TextureID System::samplerTextureCreate(const SamplerTextureDesc& desc) {

		Texture texture;
		VkFormat format = FORMAT_MAP[static_cast<uint8>(desc.format)];
		uint32_t queueFamilyIndices[3] = {
			_db.graphicsQueueFamilyIndex,
			_db.computeQueueFamilyIndex,
			_db.transferQueueFamilyIndex
		};
		VkImageCreateInfo imageInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = {
				.width = desc.width,
				.height = desc.height,
				.depth = 1
			},
			.mipLevels = desc.mipLevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.sharingMode = VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = 3,
			.pQueueFamilyIndices = queueFamilyIndices,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		VmaAllocationCreateInfo allocInfo = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};
		SOUL_VK_CHECK(vmaCreateImage(_db.allocator, 
			&imageInfo, &allocInfo, &texture.image, 
			&texture.allocation, nullptr)
			, "Create Image fail");

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

		_stagingBufferRequest(texture.allocation, desc.dataSize, desc.data);
		SOUL_LOG_INFO("Staging Buffer Request");
		VkImageViewCreateInfo viewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = texture.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = {},
			.subresourceRange = {
				.aspectMask = formatToAspectMask(format),
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = VK_REMAINING_ARRAY_LAYERS
			}
		};
		SOUL_VK_CHECK(vkCreateImageView(_db.device, 
			&viewInfo, nullptr, &texture.imageView)
			, "Create Image View fail");

		SOUL_LOG_INFO("Create Image View");
		PoolID poolID = _db.texturePool.add(texture);
		return poolID;
	}

	void System::textureDestroy(TextureID id) {
		vmaDestroyImage(_db.allocator, _db.texturePool[id].image, _db.texturePool[id].allocation);
		_db.texturePool.remove(id);
	}

	void System::_stagingBufferRequest(VmaAllocation allocation, uint32 size, void* data) {
		SOUL_ASSERT(0, data!= nullptr, "");
		Buffer stagingBuffer = {};
		VkBufferCreateInfo bufferInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		};
		VmaAllocationCreateInfo allocInfo = {
			.usage = VMA_MEMORY_USAGE_CPU_ONLY
		};
		SOUL_VK_CHECK(vmaCreateBuffer(_db.allocator, &bufferInfo, &allocInfo, &stagingBuffer.vkHandle, &stagingBuffer.allocation, nullptr),"");

		ThreadContext& threadContext = _getCurrentThreadContext();
		threadContext.stagingBuffers.add(stagingBuffer);

		void* mappedData;
		vmaMapMemory(_db.allocator, stagingBuffer.allocation, &mappedData);
		memcpy(mappedData, data, size);
		vmaUnmapMemory(_db.allocator, stagingBuffer.allocation);

	}

	BufferID System::vertexBufferCreate(const VertexBufferDesc& desc) {
		
		SOUL_ASSERT(0, desc.size != 0, "");
		SOUL_ASSERT(0, Job::System::Get().getThreadID() == 0, "This method is not thread safe. Please only call it only from main thread");
		
		Buffer vertexBuffer = {};
		VkBufferCreateInfo bufferInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = desc.size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};
		VmaAllocationCreateInfo allocInfo = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};
		SOUL_VK_CHECK(vmaCreateBuffer(_db.allocator, &bufferInfo, &allocInfo, &vertexBuffer.vkHandle, &vertexBuffer.allocation, nullptr), "");
		PoolID poolID = _db.buffers.add(vertexBuffer);

		_stagingBufferRequest(vertexBuffer.allocation, desc.size, desc.data);

		return poolID;
	}

	BufferID System::indexBufferCreate(const IndexBufferDesc& desc) {

		SOUL_ASSERT(0, desc.size != 0, "");
		SOUL_ASSERT(0, Job::System::Get().getThreadID() == 0, "This method is not thread safe. Please only call it only from main thread");
		
		Buffer buffer = {};
		VkBufferCreateInfo bufferInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = desc.size,
			.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};
		VmaAllocationCreateInfo allocInfo = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};
		SOUL_VK_CHECK(vmaCreateBuffer(_db.allocator, &bufferInfo, &allocInfo, &buffer.vkHandle, &buffer.allocation, nullptr), "");
		PoolID poolID = _db.buffers.add(buffer);

		_stagingBufferRequest(buffer.allocation, desc.size, desc.data);

		return poolID;
	}

	void System::bufferDestroy(BufferID id) {
		vmaDestroyBuffer(_db.allocator, _db.buffers[id].vkHandle, _db.buffers[id].allocation);
		_db.buffers.remove(id);
	}


	FrameContext& System::_getCurrentFrameContext() {
		return _db.frameContexts[_db.currentFrame % _db.frameContexts.size()];
	}

	ThreadContext& System::_getCurrentThreadContext() {
		return _getCurrentFrameContext().threadContexts[Job::System::Get().getThreadID()];
	}


	void System::renderGraphExecute(RenderGraph* renderGraph) {
		for (int i = 0; i < renderGraph->passNodes.size(); i++) {
			const PassNode* passNode = renderGraph->passNodes.get(i);
			switch (passNode->type) {
			case PassType::GRAPHIC:
				// get or create render pass
				// get or create pipeline
				// get or create framebuffer

			default:
				SOUL_ASSERT(0, false, "Pass Type not implemented yet");
			}
		}
	}

}}