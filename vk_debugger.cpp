#include "my_utils.hpp"
#include "vk_debugger.hpp"


void create_debug_messenger(VkDebugUtilsMessengerEXT& debug_messenger, VkInstance vk_instance) {

    std::cout << "Creating Vulkan Debugger... \n";

    if (!ENABLE_VALIDATION_LAYERS)
        return;

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
    build_debug_messenger(debug_messenger_create_info);

    if (create_func_debug_messenger(&debug_messenger, &debug_messenger_create_info, vk_instance, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up the Debug messenger! \n");
    }

    std::cout << "Vulkan Debugger created. \n\n";
}


void destroy_debug_messenger(
    VkDebugUtilsMessengerEXT debug_messenger,
    VkInstance vk_instance,
    const VkAllocationCallbacks* ptr_allocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(vk_instance, debug_messenger, ptr_allocator);
    }
}


void build_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT& debug_messenger_create_info) {

    debug_messenger_create_info = {};
    debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_messenger_create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_create_info.pfnUserCallback = CALLBACK_FUNC_debug_messenger;
}


VkResult create_func_debug_messenger(
    VkDebugUtilsMessengerEXT* ptr_debug_messenger,
    const VkDebugUtilsMessengerCreateInfoEXT* ptr_debug_messenger_create_info,
    VkInstance vk_instance,
    const VkAllocationCallbacks* ptr_allocator) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(vk_instance, ptr_debug_messenger_create_info, ptr_allocator, ptr_debug_messenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


static VKAPI_ATTR VkBool32 VKAPI_CALL CALLBACK_FUNC_debug_messenger(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* ptr_callback_data,
    void* ptr_user_data) {

    std::cerr << "\t Validation layer: " << ptr_callback_data->pMessage << std::endl;

    return VK_FALSE;
}