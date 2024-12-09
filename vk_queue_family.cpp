#include "vk_queue_family.hpp"


// Returns an index to the queue families that are found.
QueueFamilyIndices find_queue_families(VkSurfaceKHR vk_surface, VkPhysicalDevice phys_device) {

    std::cout << "\t Looking for Vulkan Queue Families... \n\n";

    QueueFamilyIndices family_indices;

    uint32_t queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_families_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_families_count, queue_families.data());

    // We need to find at least one queue family that supports
    // both VK_QUEUE_GRAPHICS_BIT (Graphics family) and Present family.
    // Not every device in the system necesseraly supports window system integration.
    // So we need to find a queue family that supports presenting to the surface created.
    // It is possible that queue families supporting drawing commands and the ones supporting
    // presentation do not overlap.
    int index = 0;
    for (const auto& queue_family : queue_families) {

        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            family_indices.graphics_family = index;
        }

        VkBool32 present_queue_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, index, vk_surface, &present_queue_support);

        if (present_queue_support) {
            family_indices.present_family = index;
        }

        if (family_indices.is_complete()) {
            break;
        }

        index++;
    }

#ifdef _DEBUG
    std::cout << "\t\t Available Vulkan Queue Families: " << queue_families_count << ".\n";
    std::cout << "\t\t Listing all queue families: \n";
    for (const auto& queue_family : queue_families) {
        std::cout << "\t\t\t " << queue_family.queueFlags << " \n";
    }
    std::cout << "\n";
#endif

    std::cout << "\t Vulkan Queue Families found. \n\n";

    return family_indices;
}