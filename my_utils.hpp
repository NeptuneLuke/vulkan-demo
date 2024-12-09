#pragma once

#define GLFW_INCLUDE_VULKAN // implicitly #include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>


#ifdef _DEBUG
const bool ENABLE_VALIDATION_LAYERS = true;
#else
const bool ENABLE_VALIDATION_LAYERS = false;
#endif

// Stores all validation layers explicitly required.
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

// Stores all required extensions (for now only VK_KHR_swapchain).
const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


// Reads all of the bytes from the specified file and
// return them in a byte array managed by std::vector.
std::vector<char> read_file(const std::string& file_name);

// Checks if the validation layers specified
// in VALIDION_LAYERS array are available.
bool check_validation_layers_support();

// Checks if the extensions specified
// in DEVICE_EXTENSIONS are available for our device.
bool check_extensions_support(VkPhysicalDevice phys_device);

// Gets required GLFW extensions.
std::vector<const char*> get_required_extensions();