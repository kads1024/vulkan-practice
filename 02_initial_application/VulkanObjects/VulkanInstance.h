#pragma once
#include <Volk/volk.h>
#include <vector>

// Debug Callbacks ==================================================================================================================================================
static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* UserData);
// =================================================================================================================================================================

class VulkanInstance final
{
public:
	VulkanInstance();
	~VulkanInstance();

	const VkInstance& getInstance() { return m_instance; };
	const VkSurfaceKHR& getSurface() { return m_surface; };
	const VkDebugUtilsMessengerEXT& getMessenger() { return m_messenger; };
	const VkDebugReportCallbackEXT& getReportCallback() { return m_reportCallback; };

private:
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkDebugUtilsMessengerEXT m_messenger;
	VkDebugReportCallbackEXT m_reportCallback;

    bool setupDebugCallbacks();
};

