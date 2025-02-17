#include <Volk/volk.h>
#include <vector>
#include <functional>


static void VK_ASSERT(bool check) {
    if (!check) exit(EXIT_FAILURE);
}

//#define VK_CHECK(value) \  if ( value != VK_SUCCESS ) \    { VK_ASSERT(false); return false; }
//#define VK_CHECK_RET(value) \  if ( value != VK_SUCCESS ) \    { VK_ASSERT(false); return value; }

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
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

    VkInstance instance;
    VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS);

    volkLoadInstance(instance);


    return 0;
}