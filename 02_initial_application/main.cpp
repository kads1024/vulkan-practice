#include <Volk/volk.h>
#include <vector>
#include <functional>

const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

static constexpr VkClearColorValue clearValueColor = { 1.0f, 1.0f, 1.0f, 1.0f };

VulkanInstance vk;
VulkanRenderDevice vkDev;

static void VK_ASSERT(bool check) {
    if (!check) exit(EXIT_FAILURE);
}

#define VK_CHECK(value) \  if ( value != VK_SUCCESS ) \    { VK_ASSERT(false); return false; }
#define VK_CHECK_RET(value) \  if ( value != VK_SUCCESS ) \    { VK_ASSERT(false); return value; }

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanInstance {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT messenger;
    VkDebugReportCallbackEXT reportCallback;
};

struct VulkanRenderDevice {
    VkDevice device;
    VkQueue graphicsQueue;
    VkPhysicalDevice physicalDevice;
    uint32_t graphicsFamily;
    VkSemaphore semaphore;
    VkSemaphore renderSemaphore;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

const std::vector<const char*> layers = {
   "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> exts = {
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

static VKAPI_ATTR VkBool32 VKAPI_CALL 
vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    printf("Validation layer: %s\n", callbackData->pMessage);
    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* UserData) {
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        return VK_FALSE;
    printf("Debug callback (%s): %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}

bool setupDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback) {
    const VkDebugUtilsMessengerCreateInfoEXT ci1 = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext = nullptr,
    .flags = 0,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,    
    .pfnUserCallback = &vulkanDebugCallback,
    .pUserData = nullptr
    };

    VK_ASSERT(vkCreateDebugUtilsMessengerEXT(instance, &ci1, nullptr, messenger) == VK_SUCCESS);

    const VkDebugReportCallbackCreateInfoEXT ci2 = { 
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,    
        .pNext = nullptr,    
        .flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,    
        .pfnCallback = &vulkanDebugReportCallback,
        .pUserData = nullptr 
    };
    VK_ASSERT(vkCreateDebugReportCallbackEXT(instance, &ci2, nullptr, reportCallback) == VK_SUCCESS);
    return true;
}

bool setVkObjectName(VulkanRenderDevice& vkDev, void* object, VkObjectType objType, const char* name) {
    VkDebugUtilsObjectNameInfoEXT nameInfo = { 
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = objType,
        .objectHandle = (uint64_t)object,
        .pObjectName = name 
    };
    return (vkSetDebugUtilsObjectNameEXT(vkDev.device, &nameInfo) == VK_SUCCESS);
}

VkResult createSemaphore(VkDevice device, VkSemaphore* outSemaphore) {
    const VkSemaphoreCreateInfo ci = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    return vkCreateSemaphore(device, &ci, nullptr, outSemaphore);
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }
   
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }
    
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& mode : availablePresentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
    const uint32_t imageCount = capabilities.minImageCount + 1;
    const bool imageCountExceeded = capabilities.maxImageCount && capabilities.maxImageCount < imageCount;
    return imageCountExceeded ? capabilities.maxImageCount : imageCount;
}

VkResult createSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsIndex, uint32_t width, uint32_t height, VkSwapchainKHR* swapChain) {
    auto swapChainSupport = querySwapchainSupport(physicalDevice, surface);
    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto surfacePresentModes = chooseSwapPresentMode(swapChainSupport.presentModes);

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
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
        .pQueueFamilyIndices = &graphicsIndex,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = surfacePresentModes,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    return vkCreateSwapchainKHR(device, &createInfo, nullptr, swapChain);
}

size_t createSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews) {
    uint32_t imageCount = 0;
    VK_ASSERT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) == VK_SUCCESS);
    
    swapchainImages.resize(imageCount);
    swapchainImageViews.resize(imageCount);
    VK_ASSERT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()) == VK_SUCCESS);

    for (uint32_t i = 0; i < imageCount; i++) {
        if (!createImageViews(device, swapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImageViews[i])) {
            exit(EXIT_FAILURE);

        }
    }

    return imageCount;
}

bool createImageViews(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView) {
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

    VkResult result = vkCreateImageView(device, &createInfo, nullptr, imageView);
    if (result != VK_SUCCESS) 
    { 
        VK_ASSERT(false); 
        return false;
    }
    return true;
}

VkResult createDevice(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures physicalDeviceFeatures, uint32_t queueIndex, VkDevice* device) {
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

    return vkCreateDevice(physicalDevice, &deviceInfo, nullptr, device);
}

VkResult findSuitablePhysicalDevice(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice* physicalDevice) {
    uint32_t deviceCount = 0;
    VK_ASSERT(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) == VK_SUCCESS);

    if (!deviceCount)    return VK_ERROR_INITIALIZATION_FAILED;
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VK_ASSERT(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()) == VK_SUCCESS);

    for (const auto& device : physicalDevices) {
        if (selector(device)) {
            *physicalDevice = device;
            return VK_SUCCESS;
        }
    }
    return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t findQueueFamilies(VkPhysicalDevice physicalDevice, VkQueueFlags desiredFlags) {
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, queueFamilies.data());
    for (uint32_t i = 0; i != queueFamilies.size(); i++) {
        if (queueFamilies[i].queueCount && (queueFamilies[i].queueFlags & desiredFlags)) {
            return i;
        }
    }
    return 0;
}

bool initVulkanRenderDevice(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures) {
    VK_ASSERT(findSuitablePhysicalDevice(vk.instance, selector, &vkDev.physicalDevice) == VK_SUCCESS);

    vkDev.graphicsFamily = findQueueFamilies(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);

    VK_ASSERT(createDevice(vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, &vkDev.device) == VK_SUCCESS);

    vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);

    if (vkDev.graphicsQueue == nullptr)    exit(EXIT_FAILURE);

    VkBool32 presentSupported = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
    if (!presentSupported) exit(EXIT_FAILURE);

    VK_ASSERT(createSwapChain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain) == VK_SUCCESS);
    const size_t imageCount = createSwapchainImages(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
    vkDev.commandBuffers.resize(imageCount);

    VK_ASSERT(createSemaphore(vkDev.device, &vkDev.semaphore) == VK_SUCCESS);
    VK_ASSERT(createSemaphore(vkDev.device, &vkDev.renderSemaphore) == VK_SUCCESS);

    const VkCommandPoolCreateInfo cpi = { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,    
        .flags = 0,    
        .queueFamilyIndex = vkDev.graphicsFamily 
    };
    VK_ASSERT(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool) == VK_SUCCESS);

    const VkCommandBufferAllocateInfo ai = { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,     
        .pNext = nullptr,     
        .commandPool = vkDev.commandPool,     
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,     
        .commandBufferCount = (uint32_t)(vkDev.swapchainImages.size())
    };
    VK_ASSERT(vkAllocateCommandBuffers(vkDev.device, &ai, vkDev.commandBuffers.data()) == VK_SUCCESS);
    return true;
}

void destroyVulkanRenderDevice(VulkanRenderDevice& vkDev)
{
    for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
        vkDestroyImageView(vkDev.device, vkDev.swapchainImageViews[i], nullptr);

    vkDestroySwapchainKHR(vkDev.device, vkDev.swapchain, nullptr);
    vkDestroyCommandPool(vkDev.device, vkDev.commandPool, nullptr);
    vkDestroySemaphore(vkDev.device, vkDev.semaphore, nullptr);
    vkDestroySemaphore(vkDev.device, vkDev.renderSemaphore, nullptr);
    vkDestroyDevice(vkDev.device, nullptr);
}

void destroyVulkanInstance(VulkanInstance& vk)
{
    vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);
    vkDestroyDebugReportCallbackEXT(vk.instance, vk.reportCallback, nullptr);
    vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, nullptr);
    vkDestroyInstance(vk.instance, nullptr);
}

bool fillCommandBuffers(size_t i) {
   /* const VkCommandBufferBeginInfo bi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr
    };
    const std::array<VkClearValue, 2> clearValues = { 
    VkClearValue{.color = clearValueColor},
    VkClearValue{.depthStencil = {1.0f, 0}}
    };
    const VkRect2D 
        screenRect = { 
        .offset = { 0, 0 },     
        .extent = { 
            .width = kScreenWidth,                 
            .height = kScreenHeight 
        }
    };

    VK_ASSERT(vkBeginCommandBuffer(vkDev.commandBuffers[i], &bi) == VK_SUCCESS);

    const VkRenderPassBeginInfo renderPassInfo = { 
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,    
        .pNext = nullptr,    
        .renderPass = vkState.renderPass,    
        .framebuffer = vkState.swapchainFramebuffers[i],    
        .renderArea = screenRect,    
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),    
        .pClearValues = clearValues.data() 
    };

    vkCmdBeginRenderPass(vkDev.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(vkDev.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkState.graphicsPipeline);

    vkCmdBindDescriptorSets(vkDev.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkState.pipelineLayout, 0, 1, &vkState.descriptorSets[i], 0, nullptr);

    vkCmdDraw(vkDev.commandBuffers[i], static_cast<uint32_t>(indexBufferSize / sizeof(uint32_t)), 1, 0, 0);

    vkCmdEndRenderPass(vkDev.commandBuffers[i]);

    VK_ASSERT(vkEndCommandBuffer(vkDev.commandBuffers[i]) == VK_SUCCESS);*/
    return true;
}
int main()
{
    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Vulkan Application",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_1
    };

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo  = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(exts.size()),
        .ppEnabledExtensionNames = exts.data(),
    };

    VulkanInstance instance = {};

    VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &instance.instance) == VK_SUCCESS);

    volkLoadInstance(instance.instance);

    return 0;
}