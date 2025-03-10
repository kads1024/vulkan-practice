#include "VulkanContext.hpp"

#include <algorithm>
#include <functional>

#include "Utils.hpp"

VulkanContext::VulkanContext()
{
	initialize();
}

VulkanContext::~VulkanContext()
{
	cleanup();
}

void VulkanContext::initialize()
{
	volkInitialize();
	createInstance();
}

void VulkanContext::cleanup()
{
	vkDestroyInstance(m_instance, nullptr);
}

void VulkanContext::createInstance()
{
	// Layers
	uint32_t layerCount{ 0 };
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

	std::vector<VkLayerProperties> layers(layerCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()));

	std::vector<std::string> availableLayers;
	std::transform(
		layers.begin(), layers.end(),
		std::back_inserter(availableLayers),
		[](const VkLayerProperties& layerProperties) {
			return layerProperties.layerName;
		}
	);

	std::vector<std::string> requestedLayers = { "VK_LAYER_KHRONOS_validation" };
	std::unordered_set<std::string> layersToEnable = filterNames(availableLayers, requestedLayers);

	std::vector<const char*> instanceLayers(layersToEnable.size());
	std::transform(
		layersToEnable.begin(), layersToEnable.end(),
		instanceLayers.begin(),
		std::mem_fn(&std::string::c_str));

	// Extensions
	uint32_t extensionCount{ 0 };
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

	std::vector<VkExtensionProperties> extensions(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));

	std::vector<std::string> availableExtensions;
	std::transform(
		extensions.begin(), extensions.end(),
		std::back_inserter(availableExtensions),
		[](const VkExtensionProperties& extensionProperties) {
			return extensionProperties.extensionName;
		}
	);

	const std::vector<std::string> requestedExtensions = {
		#if defined(VK_KHR_win32_surface)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		#endif
		#if defined(VK_EXT_debug_utils),
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		#endif
		#if defined(VK_KHR_surface)
		VK_KHR_SURFACE_EXTENSION_NAME,
		#endif
	};
	std::unordered_set<std::string> extensionsToEnable = filterNames(availableExtensions, requestedExtensions);

	std::vector<const char*> instanceExtensions(extensionsToEnable.size());
	std::transform(
		extensionsToEnable.begin(), extensionsToEnable.end(),
		instanceExtensions.begin(),
		std::mem_fn(&std::string::c_str));

	// Application Info
	const VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Vulkan Application",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	const VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
		.ppEnabledLayerNames = instanceLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
		.ppEnabledExtensionNames = instanceExtensions.data()
	};

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));

	// Volk Loader
	volkLoadInstance(m_instance);
}

void VulkanContext::pickPhysicalDevice()
{
}

void VulkanContext::createLogicalDevice()
{
	// Volk Loader
	volkLoadDevice(m_device);
}

std::unordered_set<std::string> VulkanContext::filterNames(std::vector<std::string> availableNames, std::vector<std::string> requestedNames)
{
	std::sort(availableNames.begin(), availableNames.end());
	std::sort(requestedNames.begin(), requestedNames.end());

	std::vector<std::string> result;

	std::set_intersection(
		availableNames.begin(), availableNames.end(),
		requestedNames.begin(), requestedNames.end(),
		std::back_inserter(result));

	return std::unordered_set<std::string>(result.begin(), result.end());
}