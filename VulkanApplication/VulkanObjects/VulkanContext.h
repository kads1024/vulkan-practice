#pragma once
#include <vulkan/vulkan.hpp>

class VulkanContext
{
public:
    VulkanContext();
    ~VulkanContext();

    VkDevice getDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }

private:
    void initialize();
    void cleanup();

    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();

    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
};
