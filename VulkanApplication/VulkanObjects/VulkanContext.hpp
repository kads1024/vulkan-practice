#pragma once
#include <string>
#include <unordered_set>

#include <Volk/volk.h>

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

    std::unordered_set<std::string> filterNames(std::vector<std::string> availableNames, std::vector<std::string> requestedNames);

    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
};
