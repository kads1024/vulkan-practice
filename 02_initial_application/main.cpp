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

void destroyVulkanTexture(VkDevice device, VulkanTexture& texture) {
    vkDestroyImageView(device, texture.imageView, nullptr);
    vkDestroyImage(device, texture.image, nullptr);
    vkFreeMemory(device, texture.imageMemory, nullptr);
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
    VK_CHECK(vkCreateDescriptorPool(device, &pi, nullptr, descPool));
    return true;
}

int main()
{


    return 0;
}