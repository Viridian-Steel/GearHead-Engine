#pragma once
#include "GearHead/Core.hpp"
#include "GHWindow.hpp"


namespace GearHead {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}

		bool noQueues() {
			return !graphicsFamily.has_value() && !presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class GEARHEAD_API GHDevice {
	public:
#ifndef	GEARHEAD_DEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif
		GHDevice(GHWindow& window);
		~GHDevice();

		//Not Copyable or Moveable
		GHDevice(const GHDevice&) = delete;
		GHDevice& operator=(const GHDevice&) = delete;
		GHDevice(GHDevice&&) = delete;
		GHDevice& operator=(GHDevice&&) = delete;




	private:
		void createInstance(std::string name);
		void createSurface(GHWindow& window);
		void setupDebugMessenger();
		void pickPhysicalDevice();
		void createLogicalDevice();

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool checkValidationLayerSupport();
		
		int rateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);


		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		std::vector<const char*> getRequiredExtensions();


		VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance, 
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
			const VkAllocationCallbacks* pAllocator, 
			VkDebugUtilsMessengerEXT* pDebugMessenger);

		void DestroyDebugUtilsMessengerEXT(
			VkInstance instance, 
			VkDebugUtilsMessengerEXT 
			debugMessenger, 
			const VkAllocationCallbacks* pAllocator);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);



		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

	};
}
