#pragma once
#include <Volk/volk.h>
#include <vector>

#include "VulkanObjects/VulkanInstance.h"
#include <functional>

struct VulkanTexture {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

};
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

    // Image and Image View Creation
    bool createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    bool createImageViews(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView);

    // Command Buffers
    bool fillCommandBuffers(size_t i);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // Memory
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Buffer related
    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    bool createTextureSampler(VkSampler* sampler);

    // Synchronization
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels);
    void transitionImageLayoutCmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels);

    // Image related
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);
    bool createTextureImage(const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
    void createDepthResources(uint32_t width, uint32_t height, VulkanTexture& depth);

    // Descriptor Set
    bool createDescriptorPool(uint32_t imageCount, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descPool);
};

