#pragma once

#include "my_utils.hpp"


void create_vulkan_instance(VkInstance& vk_instance);

void create_vulkan_surface(VkSurfaceKHR& vk_surface, VkInstance vk_instance, GLFWwindow* window);

void select_physical_device(VkPhysicalDevice& vk_phys_device, VkInstance& vk_instance, VkSurfaceKHR& vk_surface);

void create_vulkan_logical_device(
    VkDevice& vk_logic_device,
    VkSurfaceKHR vk_surface,
    VkPhysicalDevice vk_phys_device,
    VkQueue& vk_graphics_queue,
    VkQueue& vk_present_queue);

bool is_device_suitable(VkSurfaceKHR vk_surface, VkPhysicalDevice phys_device);

void print_all_devices(
    const std::vector<VkPhysicalDevice> physical_devices,
    uint32_t devices_count);

void print_device_properties(VkPhysicalDevice phys_device);