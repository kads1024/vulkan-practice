#include "VulkanRenderDevice.h"

#include <stb_image.h>

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

bool VulkanRenderDevice::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {
			.width = width,
			.height = height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_CHECK(vkCreateImage(m_device, &imageInfo, nullptr, &image));
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device, image, &memRequirements);

	const VkMemoryAllocateInfo ai = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};
	VK_CHECK(vkAllocateMemory(m_device, &ai, nullptr, &imageMemory));
	vkBindImageMemory(m_device, image, imageMemory, 0);
	return true;
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

bool VulkanRenderDevice::fillCommandBuffers(size_t i)
{
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

uint32_t VulkanRenderDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	return 0xFFFFFFFF;
}

bool VulkanRenderDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	VK_CHECK(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

	const VkMemoryAllocateInfo ai = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(m_device, &ai, nullptr, &bufferMemory));

	vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
	return true;
}

void VulkanRenderDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	const VkBufferCopy copyParam = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyParam);
	endSingleTimeCommands(commandBuffer);
}

void VulkanRenderDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = VkOffset3D{
			.x = 0,
			.y = 0,
			.z = 0
		},
		.imageExtent = VkExtent3D{
			.width = width,
			.height = height,
			.depth = 1
		}
	};
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	endSingleTimeCommands(commandBuffer);
}

bool VulkanRenderDevice::createTextureSampler(VkSampler* sampler)
{
	const VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};
	VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, sampler));
	return true;
}

void VulkanRenderDevice::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	transitionImageLayoutCmd(commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels);
	endSingleTimeCommands(commandBuffer);
}

void VulkanRenderDevice::transitionImageLayoutCmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
{
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	VkPipelineStageFlags sourceStage, destinationStage;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VkFormat VulkanRenderDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	const bool isLin = tiling == VK_IMAGE_TILING_LINEAR;
	const bool isOpt = tiling == VK_IMAGE_TILING_OPTIMAL;
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);
		if (isLin && (props.linearTilingFeatures & features) == features)
			return format;
		else
			if (isOpt && (props.optimalTilingFeatures & features) == features)
				return format;
	}
	printf("Failed to find supported format!\n");
	exit(0);
}

VkFormat VulkanRenderDevice::findDepthFormat()
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanRenderDevice::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool VulkanRenderDevice::createTextureImage(const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory)
{
	// Initialize Width, Height, and channels
	int texWidth, texHeight, texChannels;

	// Load file while saving pixel data, and initialized data
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	// Image size is resolution * 4 bytes
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	// Exit if failed to load texture file
	if (!pixels) {
		printf("Failed to load [%s] texture\n", filename);
		return false;
	}

	// Initialize Staging Buffer and memory
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	// Create staging buffer and memory, map memory to pointer, copy data to pointer, unmap memory
	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
	void* data;
	vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_device, stagingMemory);

	// Create Image and memory
	createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

	// Transition image from undefined to transfer destination (for copying buffer into image)
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1);

	// Copy buffer to image
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	// after copying to image, transition to shader read only
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);

	// destroy staging buffer after copying from staging buffer to image
	vkDestroyBuffer(m_device, stagingBuffer, nullptr);

	// free the staging memory
	vkFreeMemory(m_device, stagingMemory, nullptr);

	// free pixel data
	stbi_image_free(pixels);

	return true;
}

void VulkanRenderDevice::createDepthResources(uint32_t width, uint32_t height, VulkanTexture& depth)
{
	VkFormat depthFormat = findDepthFormat();
	createImage(width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth.image, depth.imageMemory);
	createImageViews(depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth.imageView);
	transitionImageLayout(depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

bool VulkanRenderDevice::createDescriptorPool(uint32_t imageCount, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descPool)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	if (uniformBufferCount)
		poolSizes.push_back(
			VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = imageCount * uniformBufferCount
			});
	if (storageBufferCount)
		poolSizes.push_back(
			VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = imageCount * storageBufferCount
			});
	if (samplerCount)
		poolSizes.push_back(
			VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = imageCount * samplerCount
			});

	const VkDescriptorPoolCreateInfo pi = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,    
		.pNext = nullptr, 
		.flags = 0,    
		.maxSets = static_cast<uint32_t>(imageCount),    
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),    
		.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data() 
	};

	VK_CHECK(vkCreateDescriptorPool(m_device, &pi, nullptr, descPool));
	return true;
}

VkCommandBuffer VulkanRenderDevice::beginSingleTimeCommands()
{
	VkCommandBuffer commandBuffer;
	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VulkanRenderDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);
	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};
	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);
	vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}
