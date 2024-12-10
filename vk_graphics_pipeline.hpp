#pragma once

#include "my_utils.hpp"


void create_graphics_pipeline(
    VkPipeline& vk_graphics_pipeline, VkPipelineLayout& vk_pipeline_layout,
    VkDevice vk_logic_device,
    VkRenderPass vk_render_pass,
    VkExtent2D vk_swapchain_extent);


// Before we can pass the code to the pipeline,
// we have to wrap it in a VkShaderModule object.
VkShaderModule create_shader_module(const std::vector<char>& shader_code, VkDevice vk_logic_device);


void create_render_pass(VkRenderPass& vk_render_pass, VkDevice vk_logic_device, VkFormat& vk_swapchain_image_format);


void create_framebuffers(
    std::vector<VkFramebuffer> vk_swapchain_framebuffers,
    VkDevice vk_logic_device,
    VkRenderPass vk_render_pass,
    std::vector<VkImageView> vk_swapchain_image_views, VkExtent2D vk_swapchain_extent);

void create_command_pool(
    VkCommandPool& vk_command_pool,
    VkSurfaceKHR vk_surface,
    VkPhysicalDevice vk_phys_device, VkDevice vk_logic_device);