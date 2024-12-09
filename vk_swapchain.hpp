#pragma once

#include "my_utils.hpp"


// Just checking if a swapchain is available is not sufficient, it may not be
// compatible with our window surface. Also we need to query for some details about
// the swapchain.
struct SwapchainSupportDetails {

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};


void create_vulkan_swapchain(
    VkSwapchainKHR& vk_swapchain,
    GLFWwindow* window, VkSurfaceKHR vk_surface,
    VkPhysicalDevice vk_phys_device, VkDevice vk_logic_device,
    std::vector<VkImage> vk_swapchain_images, VkFormat& vk_swapchain_image_format, VkExtent2D vk_swapchain_extent);

VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);

VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);

VkExtent2D choose_swapchain_extent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);

void create_swapchain_image_views(
    std::vector<VkImageView>& vk_swapchain_image_views,
    VkDevice vk_logic_device,
    std::vector<VkImage> vk_swapchain_images,
    VkFormat& vk_swapchain_image_format);

SwapchainSupportDetails query_swapchain_support(VkSurfaceKHR vk_surface, VkPhysicalDevice phys_device);