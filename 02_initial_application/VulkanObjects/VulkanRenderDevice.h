#pragma once
#include <Volk/volk.h>
#include <vector>

#include "VulkanObjects/VulkanInstance.h"
#include <functional>

struct SwapchainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanRenderDevice
{
public:
	VulkanRenderDevice(const VulkanInstance& vulkanInstance, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures);
	~VulkanRenderDevice();
	
private:
    VulkanInstance m_vulkanInstance;

    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkPhysicalDevice m_physicalDevice;
    uint32_t m_graphicsFamily;
    VkSemaphore m_semaphore;
    VkSemaphore m_renderSemaphore;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

private:
    // Device Creation
    bool initVulkanRenderDevice(uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures);
    VkResult findSuitablePhysicalDevice(std::function<bool(VkPhysicalDevice)> selector);
    uint32_t findQueueFamilies(VkQueueFlags desiredFlags);
    VkResult createDevice(VkPhysicalDeviceFeatures physicalDeviceFeatures, uint32_t queueIndex);

    // Swap Chain Creation
    VkResult createSwapChain(uint32_t width, uint32_t height);
    SwapchainSupportDetails querySwapchainSupport();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
    size_t createSwapchainImages();

    bool createImageViews(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView);

};

