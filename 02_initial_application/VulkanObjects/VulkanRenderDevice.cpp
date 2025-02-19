#include "VulkanRenderDevice.h"

#include "Utils/Utils.h"

VulkanRenderDevice::VulkanRenderDevice(const VulkanInstance& vulkanInstance, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures) 
    : m_vulkanInstance{ vulkanInstance }
{
    initVulkanRenderDevice(width, height, selector, deviceFeatures);
}

VulkanRenderDevice::~VulkanRenderDevice()
{
    for (size_t i = 0; i < m_swapchainImages.size(); i++)
        vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroySemaphore(m_device, m_semaphore, nullptr);
    vkDestroySemaphore(m_device, m_renderSemaphore, nullptr);
    vkDestroyDevice(m_device, nullptr);
}


bool VulkanRenderDevice::initVulkanRenderDevice(uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures)
{
    VK_CHECK(findSuitablePhysicalDevice(selector));

    m_graphicsFamily = findQueueFamilies(VK_QUEUE_GRAPHICS_BIT);

    VK_CHECK(createDevice(deviceFeatures, m_graphicsFamily));

    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);

    if (m_graphicsQueue == nullptr) exit(EXIT_FAILURE);

    VkBool32 presentSupported = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_graphicsFamily, m_vulkanInstance.getSurface(), &presentSupported);

    if (!presentSupported) exit(EXIT_FAILURE);

    VK_CHECK(createSwapChain(width, height));
    const size_t imageCount = createSwapchainImages();
   m_commandBuffers.resize(imageCount);

    VK_CHECK(createSemaphore(m_device, &m_semaphore) == VK_SUCCESS);
    VK_CHECK(createSemaphore(m_device, &m_renderSemaphore) == VK_SUCCESS);

    const VkCommandPoolCreateInfo cpi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = m_graphicsFamily
    };
    VK_CHECK(vkCreateCommandPool(m_device, &cpi, nullptr, &m_commandPool));

    const VkCommandBufferAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)(m_swapchainImages.size())
    };
    VK_CHECK(vkAllocateCommandBuffers(m_device, &ai, m_commandBuffers.data()));
    return true;
}

VkResult VulkanRenderDevice::findSuitablePhysicalDevice(std::function<bool(VkPhysicalDevice)> selector)
{
    uint32_t deviceCount = 0;
    VK_CHECK_RET(vkEnumeratePhysicalDevices(m_vulkanInstance.getInstance(), &deviceCount, nullptr));

    if (!deviceCount) return VK_ERROR_INITIALIZATION_FAILED;

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VK_CHECK_RET(vkEnumeratePhysicalDevices(m_vulkanInstance.getInstance(), &deviceCount, physicalDevices.data()));

    for (const auto& device : physicalDevices) {
        if (selector(device)) {
            m_physicalDevice = device;
            return VK_SUCCESS;
        }
    }
    return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t VulkanRenderDevice::findQueueFamilies(VkQueueFlags desiredFlags)
{
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &familyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &familyCount, queueFamilies.data());

    for (uint32_t i = 0; i != queueFamilies.size(); i++) 
        if (queueFamilies[i].queueCount && (queueFamilies[i].queueFlags & desiredFlags)) 
            return i;
        
    return 0;
}

VkResult VulkanRenderDevice::createDevice(VkPhysicalDeviceFeatures physicalDeviceFeatures, uint32_t queueIndex)
{
    const std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    const float queuePriority = 1.0f;
    const VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    const VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &physicalDeviceFeatures
    };

    return vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device);
}

VkResult VulkanRenderDevice::createSwapChain(uint32_t width, uint32_t height)
{
    auto swapChainSupport = querySwapchainSupport();
    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto surfacePresentModes = chooseSwapPresentMode(swapChainSupport.presentModes);

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = m_vulkanInstance.getSurface(),
        .minImageCount = chooseSwapImageCount(swapChainSupport.capabilities),
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = {
            .width = width,
            .height = height
        },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &m_graphicsFamily,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = surfacePresentModes,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    return vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
}

SwapchainSupportDetails VulkanRenderDevice::querySwapchainSupport()
{
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_vulkanInstance.getSurface(), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_vulkanInstance.getSurface(), &formatCount, nullptr);
    if (formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_vulkanInstance.getSurface(), &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_vulkanInstance.getSurface(), &presentModeCount, nullptr);
    if (presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_vulkanInstance.getSurface(), &presentModeCount, details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR VulkanRenderDevice::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
}

VkPresentModeKHR VulkanRenderDevice::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& mode : availablePresentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t VulkanRenderDevice::chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
    const uint32_t imageCount = capabilities.minImageCount + 1;
    const bool imageCountExceeded = capabilities.maxImageCount && capabilities.maxImageCount < imageCount;
    return imageCountExceeded ? capabilities.maxImageCount : imageCount;
}

size_t VulkanRenderDevice::createSwapchainImages()
{
    uint32_t imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr));

    m_swapchainImages.resize(imageCount);
    m_swapchainImageViews.resize(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data()));

    for (uint32_t i = 0; i < imageCount; i++) {
        if (!createImageViews(m_swapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &m_swapchainImageViews[i])) {
            exit(EXIT_FAILURE);

        }
    }

    return imageCount;
}

bool VulkanRenderDevice::createImageViews(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView)
{
    const VkImageViewCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, imageView));
    return true;
}
