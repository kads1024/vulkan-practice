#include "VulkanContext.hpp"

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
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Vulkan Application",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo
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
