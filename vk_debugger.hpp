#pragma once

#include <vulkan/vulkan.h>


void create_debug_messenger(VkDebugUtilsMessengerEXT& debug_messenger, VkInstance vk_instance);

void destroy_debug_messenger(
    VkDebugUtilsMessengerEXT debug_messenger,
    VkInstance vk_instance,
    const VkAllocationCallbacks* ptr_allocator);

void build_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT& debug_messenger_create_info);

VkResult create_func_debug_messenger(
    VkDebugUtilsMessengerEXT* ptr_debug_messenger,
    const VkDebugUtilsMessengerCreateInfoEXT* ptr_debug_messenger_create_info,
    VkInstance vk_instance,
    const VkAllocationCallbacks* ptr_allocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL CALLBACK_FUNC_debug_messenger(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* ptr_callback_data,
    void* ptr_user_data);