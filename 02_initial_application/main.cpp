#include <Volk/volk.h>
#include <vector>
#include <functional>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;
using glm::vec3;
using glm::vec2;
using glm::vec4;
const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

static constexpr VkClearColorValue clearValueColor = { 1.0f, 1.0f, 1.0f, 1.0f };

VulkanInstance vk;
VulkanRenderDevice vkDev;

static void VK_ASSERT(bool check) {
    if (!check) exit(EXIT_FAILURE);
}

struct VulkanTexture {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
};

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

uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    }
    return 0xFFFFFFFF;
}

bool createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
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

    VK_ASSERT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) == VK_SUCCESS);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    const VkMemoryAllocateInfo ai = { 
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,    
        .pNext = nullptr,    
        .allocationSize = memRequirements.size,    
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
    };

    VK_ASSERT(vkAllocateMemory(device, &ai, nullptr, &bufferMemory) == VK_SUCCESS);

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    return true;
}

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    // VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool, graphicsQueue);
    // const VkBufferCopy copyParam = { .srcOffset = 0,     .dstOffset = 0,     .size = size };
    // vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyParam);
    // endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

VkCommandBuffer beginSingleTimeCommands(VulkanRenderDevice& vkDev)
{
    VkCommandBuffer commandBuffer;
    const VkCommandBufferAllocateInfo allocInfo = { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,    
        .pNext = nullptr,    
        .commandPool = vkDev.commandPool,    
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,    
        .commandBufferCount = 1 
    };
    vkAllocateCommandBuffers(vkDev.device, &allocInfo, &commandBuffer);

    const VkCommandBufferBeginInfo beginInfo = { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,   
        .pNext = nullptr,   
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,   
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void endSingleTimeCommands(VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer)
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
    vkQueueSubmit(vkDev.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkDev.graphicsQueue);
    vkFreeCommandBuffers(vkDev.device, vkDev.commandPool, 1, &commandBuffer);
}


bool createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
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
    VK_ASSERT(vkCreateImage(device, &imageInfo, nullptr, &image) == VK_SUCCESS);
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    const VkMemoryAllocateInfo ai = { 
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,    
        .pNext = nullptr,    
        .allocationSize = memRequirements.size,    
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) 
    };
    VK_ASSERT(vkAllocateMemory(device, &ai, nullptr, &imageMemory) == VK_SUCCESS);
    vkBindImageMemory(device, image, imageMemory, 0);
    return true;
}

bool createTextureSampler(VkDevice device, VkSampler* sampler)
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
    VK_ASSERT(vkCreateSampler(device, &samplerInfo, nullptr, sampler) == VK_SUCCESS);
    return true;
}

void copyBufferToImage(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vkDev);
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
    endSingleTimeCommands(vkDev, commandBuffer);
}

void destroyVulkanTexture(VkDevice device, VulkanTexture& texture) {
    vkDestroyImageView(device, texture.imageView, nullptr);
    vkDestroyImage(device, texture.image, nullptr);
    vkFreeMemory(device, texture.imageMemory, nullptr);
}
void transitionImageLayout(VulkanRenderDevice& vkDev, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vkDev);
    transitionImageLayoutCmd(commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels);
    endSingleTimeCommands(vkDev, commandBuffer);
}

void transitionImageLayoutCmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
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

VkFormat findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    const bool isLin = tiling == VK_IMAGE_TILING_LINEAR;
    const bool isOpt = tiling == VK_IMAGE_TILING_OPTIMAL;
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        if (isLin && (props.linearTilingFeatures & features) == features)
            return format;
        else
            if (isOpt && (props.optimalTilingFeatures & features) == features)
                return format;
    }
    printf("Failed to find supported format!\n");
    exit(0);
}


VkFormat findDepthFormat(VkPhysicalDevice device) {
    return findSupportedFormat(device, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void createDepthResources(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VulkanTexture& depth)
{
    VkFormat depthFormat = findDepthFormat(vkDev.physicalDevice);
    createImage(vkDev.device, vkDev.physicalDevice, width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth.image, depth.imageMemory);
    createImageViews(vkDev.device, depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth.imageView);
    transitionImageLayout(vkDev, depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

bool createTextureImage(VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        printf("Failed to load [%s] texture\n", filename);
        return false;
    }
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(vkDev.device, vkDev.physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
    void* data;
    vkMapMemory(vkDev.device, stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(vkDev.device, stagingMemory);

    createImage(vkDev.device, vkDev.physicalDevice, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    transitionImageLayout(vkDev, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1);
    copyBufferToImage(vkDev, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(vkDev, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);
    vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
    vkFreeMemory(vkDev.device, stagingMemory, nullptr);
    stbi_image_free(pixels);
    return true;
}
bool createTexturedVertexBuffer(VulkanRenderDevice& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize)
{

    const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);
    if (!scene || !scene->HasMeshes()) {
        printf("Unable to load %s\n", filename);
        exit(255);
    }
    const aiMesh* mesh = scene->mMeshes[0];
    struct VertexData {
        vec3 pos;
        vec2 tc;
    };
    std::vector<VertexData> vertices;
    for (unsigned i = 0; i != mesh->mNumVertices; i++) {
        const aiVector3D v = mesh->mVertices[i];
        const aiVector3D t = mesh->mTextureCoords[0][i];
        vertices.push_back({ vec3(v.x, v.z, v.y), vec2(t.x, t.y) });
    }
    std::vector<unsigned int> indices;
    for (unsigned i = 0; i != mesh->mNumFaces; i++)
        for (unsigned j = 0; j != 3; j++)
            indices.push_back(mesh->mFaces[i].mIndices[j]);
    aiReleaseImport(scene);


    *vertexBufferSize = sizeof(VertexData) * vertices.size();
    *indexBufferSize = sizeof(unsigned int) * indices.size();
    VkDeviceSize bufferSize = *vertexBufferSize + *indexBufferSize;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(vkDev.device, vkDev.physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
    void* data;
    vkMapMemory(vkDev.device, stagingMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), *vertexBufferSize);
    memcpy((unsigned char*)data + *vertexBufferSize, indices.data(), *indexBufferSize);
    vkUnmapMemory(vkDev.device, stagingMemory);
    createBuffer(vkDev.device, vkDev.physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *storageBuffer, *storageBufferMemory);


    copyBuffer(vkDev.device, vkDev.commandPool, vkDev.graphicsQueue, stagingBuffer, *storageBuffer, bufferSize);
    vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
    vkFreeMemory(vkDev.device, stagingMemory, nullptr);
    return true;
}
//bool createUniformBuffers() {
//    VkDeviceSize bufferSize = sizeof(UniformBuffer);
//    vkState.uniformBuffers.resize(vkDev.swapchainImages.size());
//    vkState.uniformBuffersMemory.resize(vkDev.swapchainImages.size());
//    for (size_t i = 0; i < vkDev.swapchainImages.size(); i++) {
//        if (!createBuffer(vkDev.device, vkDev.physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkState.uniformBuffers[i], vkState.uniformBuffersMemory[i])) {
//            printf("Fail: buffers\n");
//            return false;
//        }
//    }
//    return true;
//}
//
//void updateUniformBuffer(uint32_t currentImage, const UniformBuffer& ubo)
//{
//    void* data = nullptr;
//    vkMapMemory(vkDev.device, vkState.uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
//    memcpy(data, &ubo, sizeof(ubo));
//    vkUnmapMemory(vkDev.device, vkState.uniformBuffersMemory[currentImage]);
//}

bool createDescriptorPool(VkDevice device, uint32_t imageCount, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descPool)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    if (uniformBufferCount) 
        poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        .descriptorCount = imageCount * uniformBufferCount
        });
    if (storageBufferCount) 
        poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,       .descriptorCount = imageCount * storageBufferCount });
    if (samplerCount) 
        poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,       .descriptorCount = imageCount * samplerCount });
    
    const VkDescriptorPoolCreateInfo pi = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,    .pNext = nullptr, .flags = 0,    .maxSets = static_cast<uint32_t>(imageCount),    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),    .pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data() };
    //VK_CHECK(vkCreateDescriptorPool(device, &pi, nullptr, descPool));
    return true;
}

int main()
{


    return 0;
}