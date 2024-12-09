#include "my_utils.hpp"

#include <fstream>
#include <set>


// Reads all of the bytes from the specified file and
// return them in a byte array managed by std::vector.
std::vector<char> read_file(const std::string& file_name) {

    // std::ios::ate = start reading at the end of the file
    // std::ios::binary = read the file as a binary file (avoid text transformations)
    // We start reading at the end of the file because so that we can
    // determine the size of the file and allocate a buffer.
    std::ifstream file(file_name, std::ios::ate | std::ios::binary);

    const std::string error_file = "Failed to open file: " + file_name + " \n";
    if (!file.is_open()) {
        throw std::runtime_error(error_file);
    }

    // Now i get the size of the file
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    // Now i go back to the top of the file and start reading
    // all of the bytes at once.
    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}


// Checks if the validation layers specified
// in VALIDION_LAYERS array are available.
bool check_validation_layers_support() {

    uint32_t layers_count = 0;
    vkEnumerateInstanceLayerProperties(&layers_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layers_count);
    vkEnumerateInstanceLayerProperties(&layers_count, available_layers.data());

    for (const char* layer_name : VALIDATION_LAYERS) {

        bool layer_found = false;

        for (const auto& layer_properties : available_layers) {

            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false; // Vulkan specific error: VK_ERROR_LAYER_NOT_PRESENT
        }
    }

    return true;
}


// Checks if the extensions specified
// in DEVICE_EXTENSIONS are available for our device.
bool check_extensions_support(VkPhysicalDevice phys_device) {

    // Technically, the availability of a presentation queue
    // implies that the swapchain extension VK_KHR_SWAPCHAIN_EXTENSION_NAME
    // is supported. However it's still good to be explicit.

    uint32_t extensions_count = 0;
    vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extensions_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extensions_count);
    vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extensions_count, available_extensions.data());

    // Unconfirmed required extensions
    std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    // We could also use a nested loop like in check_validation_layers_support.
    for (const auto& ext : available_extensions) {
        required_extensions.erase(ext.extensionName);
    }

    return required_extensions.empty();
}


// Gets the required GLFW extensions.
std::vector<const char*> get_required_extensions() {

    uint32_t glfw_extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extensions_count);

    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Debug messenger extension
    }

    return extensions;
}