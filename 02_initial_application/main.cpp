#include <Volk/volk.h>
#include <vector>
#include <functional>


static void VK_ASSERT(bool check) {
    if (!check) exit(EXIT_FAILURE);
}

#define VK_CHECK(value) \  if ( value != VK_SUCCESS ) \    { VK_ASSERT(false); return false; }
#define VK_CHECK_RET(value) \  if ( value != VK_SUCCESS ) \    { VK_ASSERT(false); return value; }

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