#include "VulkanInstance.h"

#include "Utils/Utils.h"

VulkanInstance::VulkanInstance()
{
	const std::vector<const char*> layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> extensions = {
		"VK_KHR_surface",
	#if defined (WIN32)
		 "VK_KHR_win32_surface",
	#endif
	#if defined (__APPLE__)
		 "VK_MVK_macos_surface",
	#endif
	#if defined (__linux__)
		 "VK_KHR_xcb_surface",
	#endif
		 VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		 VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Initial Application",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_1
	};

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(layers.size()),
		.ppEnabledLayerNames = layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));
	volkLoadInstance(m_instance);

	setupDebugCallbacks();
}

VulkanInstance::~VulkanInstance()
{
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDebugReportCallbackEXT(m_instance, m_reportCallback, nullptr);
	vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

bool VulkanInstance::setupDebugCallbacks()
{
	const VkDebugUtilsMessengerCreateInfoEXT ci1 = {
	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.pNext = nullptr,
	.flags = 0,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = &vulkanDebugCallback,
	.pUserData = nullptr
	};

	VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_instance, &ci1, nullptr, &m_messenger));

	const VkDebugReportCallbackCreateInfoEXT ci2 = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		.pfnCallback = &vulkanDebugReportCallback,
		.pUserData = nullptr
	};
	VK_CHECK(vkCreateDebugReportCallbackEXT(m_instance, &ci2, nullptr, &m_reportCallback));

	return true;
}

// Debug Callbacks ==================================================================================================================================================
VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* UserData)
{
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		return VK_FALSE;
	printf("Debug callback (%s): %s\n", pLayerPrefix, pMessage);
	return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
	printf("Validation layer: %s\n", callbackData->pMessage);
	return VK_FALSE;
}