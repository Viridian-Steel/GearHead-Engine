#include "ghpch.hpp"
#include "GHDevice.hpp"

namespace GearHead {
	GHDevice::GHDevice(GHWindow& window)
	{
		createInstance(window.getName());
		setupDebugMessenger();
		createSurface(window);
		pickPhysicalDevice();
		createLogicalDevice();
	}

	GHDevice::~GHDevice()
	{
		//order is important here. Debug callback uses instance, so it needs to be destroyed before instance device also uses instance, so best to destroy it first

		vkDestroyDevice(device, nullptr);
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(instance, surface, nullptr);

		vkDestroyInstance(instance, nullptr);
	}


	void GHDevice::createInstance(std::string name) {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			GEARHEAD_CORE_CRITICAL("validation layers requested, but not available!");
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName	= name.c_str();
		appInfo.applicationVersion	= VK_MAKE_VERSION(0, 0, 1);
		appInfo.pEngineName			= "GearHeadEngine";
		appInfo.engineVersion		= VK_MAKE_VERSION(0, 0, 6);
		appInfo.apiVersion			= VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		std::vector<const char*> requiredExtensions = getRequiredExtensions();

		//								VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR


		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		//Checks that required extensions are in the full extension list
		int c = 0;
		for (const auto& rf : requiredExtensions) {
			c = 0;

			for (const auto& ext : extensions) {
				if (ext.extensionName == rf) {
					break;
				}
				c++;
			}
			if (c == (extensionCount - 1)) {
				GEARHEAD_CORE_CRITICAL("Required Extension {0} not found. Aborting", rf);
				throw std::runtime_error("A Rquired Extension was not found.");
			}
		}
		//requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

		//createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			GEARHEAD_CORE_CRITICAL("Failed to Create Instance, Aborting");
			throw std::runtime_error("Failed to Create Instance!");
		} 
	}

	void GHDevice::createSurface(GHWindow& window) {

		if (glfwCreateWindowSurface(instance, window.GetWindow(), nullptr, &surface) != VK_SUCCESS) {
			GEARHEAD_CORE_ERROR("failed to create window surface!");
			throw std::runtime_error("failed to create window surface!");
		}

	}

	void GHDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void GHDevice::setupDebugMessenger()
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			GEARHEAD_CORE_CRITICAL("failed to set up debug messenger!");
			throw std::runtime_error("failed to set up debug messenger!");
		}

	}

	void GHDevice::pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			GEARHEAD_CORE_ERROR("No Vulkan-Capable GPU Found");
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		// Check if the best candidate is suitable at all
		if (candidates.rbegin()->first > 0) {
			physicalDevice = candidates.rbegin()->second;
			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

			GEARHEAD_CORE_INFO("GPU: {0}",deviceProps.deviceName);
		}
		else {
			GEARHEAD_CORE_ERROR("No Suitable GPU Found");
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void GHDevice::createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
			
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		VkPhysicalDeviceFeatures deviceFeatures{};
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			GEARHEAD_CORE_ERROR("failed to create logical device!");
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

		GEARHEAD_CORE_INFO("Created Logical Device");



	}

	SwapChainSupportDetails GHDevice::querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);


		return details;
	}

	int GHDevice::rateDeviceSuitability(VkPhysicalDevice device) {

		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		QueueFamilyIndices indices = findQueueFamilies(device);

		int score = 0;
		
		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		// Compute Capabilities is Massively Needed
		if (indices.isComplete()) {
			score += 1000;
		}

		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		// Application can't function without geometry shaders and 
		if (!deviceFeatures.geometryShader || indices.noQueues() || !extensionsSupported) {
			return 0;
		}

		return score;
	}

	QueueFamilyIndices GHDevice::findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		GEARHEAD_CORE_TRACE("Number of Families: {0}", queueFamilyCount);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				GEARHEAD_CORE_TRACE("Found GRAPHICS QUEUE at index: {0}", i);
				indices.graphicsFamily = i;
			}

			if (presentSupport) {
				GEARHEAD_CORE_TRACE("Found PRESENT QUEUE at index: {0}", i);
				indices.presentFamily = i; // likely to be the same as the graphics queue, but whatevs
			}

			if (indices.isComplete()) {
				GEARHEAD_CORE_TRACE("All requirements found");
				break;
			}

			i++;
		}

		return indices;
	}

	bool GHDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool GHDevice::checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerfound = false;

			for (const auto& layerProps : availableLayers) {
				if (strcmp(layerName, layerProps.layerName) == 0) {
					layerfound = true;
					break;
				}
			}
			if (!layerfound) { return false; }
		}

		return true;
	}

	
	std::vector<const char*> GHDevice::getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}
	
	
	VkResult GHDevice::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void GHDevice::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}
	
	
	VKAPI_ATTR VkBool32 VKAPI_CALL GHDevice::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		//Filter by message severity
		switch (messageSeverity) {

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			GEARHEAD_CORE_INFO("INFO: Allocation Layer: {0}", pCallbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			GEARHEAD_CORE_WARN("WARN: Allocation Layer: {0}", pCallbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			GEARHEAD_CORE_ERROR("ERROR: Allocation Layer: {0}", pCallbackData->pMessage);
			break;

		default:
			GEARHEAD_CORE_INFO("Allocation Layer: {0}", pCallbackData->pMessage);
			break;

		}
		return VK_FALSE;
	}
}
