#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <span>
#include <vector>
#include <unordered_map>

bool endsWith(std::string_view s, std::string_view part) {
    return s.size() >= part.size() && s.ends_with(part);
}

std::string readShaderFile(const std::string& fileName, std::unordered_map<std::string, bool>& includedFiles) {
    if (!includedFiles.emplace(fileName, true).second) {
        std::cerr << "Circular dependency detected in shader includes: " << fileName << "\n";
        return {};
    }

    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "I/O error. Cannot open shader file '" << fileName << "'\n";
        return {};
    }

    std::vector<char> buffer(std::filesystem::file_size(fileName));
    file.read(buffer.data(), buffer.size());
    std::string code(buffer.begin(), buffer.end());

    // Remove UTF-8 BOM if present
    static constexpr unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };
    if (code.size() > 3 && std::equal(std::begin(BOM), std::end(BOM), code.begin())) {
        code.erase(0, 3);
    }

    // Handle #include directives
    std::string_view codeView = code;
    size_t pos;
    while ((pos = codeView.find("#include ")) != std::string::npos) {
        size_t p1 = codeView.find('<', pos);
        size_t p2 = codeView.find('>', pos);

        if (p1 == std::string::npos || p2 == std::string::npos || p2 <= p1) {
            std::cerr << "Error while processing shader includes in file: " << fileName << "\n";
            return {};
        }

        std::string includeName = std::string(codeView.substr(p1 + 1, p2 - p1 - 1));
        std::string includeContent = readShaderFile(includeName, includedFiles);
        code.replace(pos, p2 - pos + 1, includeContent);
        codeView = code;  // Update view after modification
    }

    return code;
}

std::string readShaderFile(const std::string& fileName) {
    std::unordered_map<std::string, bool> includedFiles;
    return readShaderFile(fileName, includedFiles);
}

void saveSPIRVBinaryFile(const std::string& filename, std::span<const uint32_t> code) noexcept {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Failed to write SPIR-V file '" << filename << "'\n";
        return;
    }
    file.write(reinterpret_cast<const char*>(code.data()), code.size() * sizeof(uint32_t));
}
