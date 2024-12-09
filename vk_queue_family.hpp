#pragma once

#include "my_utils.hpp"
#include <optional>


struct QueueFamilyIndices {

    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};


// Returns an index to the queue families that are found.
QueueFamilyIndices find_queue_families(VkSurfaceKHR vk_surface, VkPhysicalDevice phys_device);