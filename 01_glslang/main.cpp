#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <span>

#include <vulkan/vulkan.h>
#include <glslang/Public/resource_limits_c.h>

// ShaderModule struct
struct ShaderModule {
    std::vector<uint32_t> SPIRV;
    VkShaderModule shaderModule{};
};

// Function declarations
bool endsWith(std::string_view s, std::string_view part);
std::string readShaderFile(const std::string& fileName);
void saveSPIRVBinaryFile(const std::string& filename, std::span<const uint32_t> code) noexcept;

// Shader stage lookup map for faster retrieval
glslang_stage_t glslangShaderStageFromFileName(std::string_view fileName) {
    static const std::unordered_map<std::string_view, glslang_stage_t> stageMap = {
        {".vert", GLSLANG_STAGE_VERTEX},
        {".frag", GLSLANG_STAGE_FRAGMENT},
        {".geom", GLSLANG_STAGE_GEOMETRY},
        {".comp", GLSLANG_STAGE_COMPUTE},
        {".tesc", GLSLANG_STAGE_TESSCONTROL},
        {".tese", GLSLANG_STAGE_TESSEVALUATION}
    };

    for (const auto& [ext, stage] : stageMap) {
        if (endsWith(fileName, ext)) return stage;
    }

    return GLSLANG_STAGE_VERTEX; // Default to vertex shader
}

// Compile shader source code into SPIR-V
size_t compileShader(glslang_stage_t stage, const char* shaderSource, ShaderModule& shaderModule) {
    glslang_input_t input{
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_1,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_3,
        .code = shaderSource,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource()
    };

    glslang_shader_t* shd = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shd, &input)) {
        std::cerr << "GLSL preprocessing failed:\n" << glslang_shader_get_info_log(shd) << '\n';
        glslang_shader_delete(shd);
        return 0;
    }

    if (!glslang_shader_parse(shd, &input)) {
        std::cerr << "GLSL parsing failed:\n" << glslang_shader_get_info_log(shd) << '\n';
        glslang_shader_delete(shd);
        return 0;
    }

    glslang_program_t* prg = glslang_program_create();
    glslang_program_add_shader(prg, shd);

    if (!glslang_program_link(prg, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        std::cerr << "GLSL linking failed:\n" << glslang_program_get_info_log(prg) << '\n';
        glslang_program_delete(prg);
        glslang_shader_delete(shd);
        return 0;
    }

    // Generate SPIR-V
    glslang_program_SPIRV_generate(prg, stage);
    shaderModule.SPIRV.resize(glslang_program_SPIRV_get_size(prg));
    glslang_program_SPIRV_get(prg, shaderModule.SPIRV.data());

    if (const char* spirv_messages = glslang_program_SPIRV_get_messages(prg)) {
        std::cerr << spirv_messages << '\n';
    }

    glslang_program_delete(prg);
    glslang_shader_delete(shd);

    return shaderModule.SPIRV.size();
}

// Compile shader from file
size_t compileShaderFromFile(const std::string& file, ShaderModule& shaderModule) {
    if (std::string shaderSource = readShaderFile(file); !shaderSource.empty()) {
        return compileShader(glslangShaderStageFromFileName(file), shaderSource.c_str(), shaderModule);
    }
    return 0;
}

// Function to test shader compilation
void testShaderCompilation(const std::string& sourceFileName, const std::string& destFileName) {
    ShaderModule shaderModule;
    if (compileShaderFromFile(sourceFileName, shaderModule) < 1) return;
    saveSPIRVBinaryFile(destFileName, shaderModule.SPIRV);
}

// Main function
int main() {
    glslang_initialize_process();

    testShaderCompilation("Shaders/VK01.vert", "Shaders/VK01.vert.spv");
    testShaderCompilation("Shaders/VK01.frag", "Shaders/VK01.frag.spv");

    glslang_finalize_process();
    return 0;
}
