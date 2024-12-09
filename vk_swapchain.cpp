#include "vk_swapchain.hpp"
#include "vk_queue_family.hpp"
#include <algorithm> // std::clamp


void create_vulkan_swapchain(
    VkSwapchainKHR& vk_swapchain,
    GLFWwindow* window, VkSurfaceKHR vk_surface,
    VkPhysicalDevice vk_phys_device, VkDevice vk_logic_device,
    std::vector<VkImage> vk_swapchain_images, VkFormat& vk_swapchain_image_format, VkExtent2D vk_swapchain_extent) {

    std::cout << "Creating Vulkan Swapchain... \n\n";

    SwapchainSupportDetails swapchain_support = query_swapchain_support(vk_surface, vk_phys_device);

    // Setting up swapchain properties
    VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.present_modes);
    VkExtent2D swap_extent = choose_swapchain_extent(window, swapchain_support.capabilities);

    // Decide how many minimum images we want to have in the swapchain.
    // Sticking to the minimum means that sometimes it may occur that we have to wait
    // on the driver to complete internal operations before we can acquire
    // another image to render to. It is therefore recommended to request
    // at least one more image than the minimum.
    uint32_t images_in_swapchain_count = swapchain_support.capabilities.minImageCount + 1;

    // Make sure to not exceed the maximum number of images 
    if (swapchain_support.capabilities.maxImageCount > 0 &&
        images_in_swapchain_count > swapchain_support.capabilities.maxImageCount) {

        images_in_swapchain_count = swapchain_support.capabilities.maxImageCount;
    }

    // Fill the Swapchain struct
    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vk_surface;
    swapchain_create_info.minImageCount = images_in_swapchain_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = swap_extent;

    // The amount of layers each image consists of (always 1 unless developing stereoscopic 3D app)
    swapchain_create_info.imageArrayLayers = 1;

    // What kind of operation we will use the images in the swapchain for.
    // We are going to render them directly, which means they are used as color attachment.
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Select which type of sharing mode the images in the swapchain will use
    // with the queue families.
    QueueFamilyIndices family_indices = find_queue_families(vk_surface, vk_phys_device);
    uint32_t queue_family_indices[] = {
        family_indices.graphics_family.value(),
        family_indices.present_family.value()
    };

    // If the graphics and present queue are not the same queue family
    if (family_indices.graphics_family != family_indices.present_family) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2; // family and present
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
        // If the graphics and presentation queue are the same queue family
        // which in most hardware is the case, the we should stick to exclusive mode
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = nullptr;
    }

    // We can specify that a certain transform should be applied to images in the swapchain
    // (if supported -> supportedTransform in capabilities), like a 90 degree clockwise rotation.
    // If you do not want any transformation, simply specify the current one.
    swapchain_create_info.preTransform = swapchain_support.capabilities.currentTransform;

    // Specifies the if the alpha channel should be used for blending with other windows
    // in the window system. Almost always you will want to ignore it and set it
    // as the following.
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    swapchain_create_info.presentMode = present_mode;

    // We don't care about color of pixels that are obscured
    // (for example because another window is in front of them).
    // Which results in better performances.
    swapchain_create_info.clipped = VK_TRUE;

    // Swapchain can become invalid or unoptimized ar runtime, 
    // for example when the window is resized. In that case the swapchain
    // must be recreated from scratch and a copy of the old one must be specified.
    // For now we will assume to create only one swapchain.
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(
        vk_logic_device,
        &swapchain_create_info,
        nullptr,
        &vk_swapchain) != VK_SUCCESS) {

        throw std::runtime_error("Failed to create Vulkan Swapchain! \n");
    }

    // First query the final number of images, then resize and then call it again to
    // retrieve the handles. This is done because we only specified the minimum
    // number of images in the swapchain, so the implementation is allowed to
    // create a swapchain with more images.
    vkGetSwapchainImagesKHR(vk_logic_device, vk_swapchain, &images_in_swapchain_count, nullptr);
    vk_swapchain_images.resize(images_in_swapchain_count);
    vkGetSwapchainImagesKHR(vk_logic_device, vk_swapchain, &images_in_swapchain_count, vk_swapchain_images.data());

    // Written just because will need it in the future.
    vk_swapchain_image_format = surface_format.format;
    vk_swapchain_extent = swap_extent;

    std::cout << "Vulkan Swapchain created. \n\n";
}


VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {

    // We are choosing the best possible swapchain surface format.
    // The typical combination is:
    // format = RGBA 8 bit (32 bits per pixel).
    // color space = SRGB (more accurate perceived colors)
    // SRGB is pretty much the color space standard for images and textures.

    for (const auto& available_format : available_formats) {

        // This is the best possible (if available) format.
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {

            return available_format;
        }
    }

    // If we don't get the best one we wanted, we could rank the available
    // formats and choose the (second) best, but in most cases it's okay
    // to just settle with the first specified format.
    return available_formats[0];
}


VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {

    // Since we are on a computer and not a mobile device, 
    // the best present mode is probably VK_PRESENT_MODE_MAILBOX_KHR.
    // Even though it may be possible that it's not available.
    // The only present mode guaranteed to be available is
    // VK_PRESENT_MODE_FIFO_KHR, so we need to query even for
    // present modes.

    for (const auto& available_present_mode : available_present_modes) {

        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {

            return available_present_mode;
        }
    }

    // As we said, if VK_PRESENT_MODE_MAILBOX_KHR is not present
    // just return the guaranteed one.
    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D choose_swapchain_extent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {

    // The swap extent is the resolution of the swapchain images
    // and it's almost always equal to the resolution of the window
    // that we are drawing to (in pixels).
    // The range of the possible resolutions is defined
    // in the VkSurfaceCapabilitiesKHR struct.

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {

        // This is because GLFW uses two units: pixels and screen coordinates.
        // But Vulkan works only with pixels, so the swapchain extent must be
        // specified in pixels.
        // We use glfwGetFramebufferSize() to query the resolution of the window
        // in pixel before matching it against the min/max image extent.

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(w),
            static_cast<uint32_t>(h)
        };

        // Clamp to bound the values of width and height
        // between the allowed min/max extents that are supported
        // by the implementation.
        actual_extent.width = std::clamp(
            actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);

        actual_extent.height = std::clamp(
            actual_extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actual_extent;
    }
}


void create_swapchain_image_views(
    std::vector<VkImageView>& vk_swapchain_image_views,
    VkDevice vk_logic_device,
    std::vector<VkImage> vk_swapchain_images,
    VkFormat& vk_swapchain_image_format) {

    std::cout << "Creating Vulkan Image views for Vulkan Swapchain images... \n";

    // Resize to fit all the image views we will create
    vk_swapchain_image_views.resize(vk_swapchain_images.size());

    // Create an image view for every image
    for (size_t i = 0; i < vk_swapchain_images.size(); i++) {

        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = vk_swapchain_images[i];

        // How the image should be interpreted
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = vk_swapchain_image_format;

        // You can map color channels to swizzle them around.
        // You can also use values between 0,1.
        // We will stick to default mapping.
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Set what the image's purpose is and which part of the image
        // should be accessed. As we said, our images will be used as color targest
        // without any mimpapping levels or multiple layers.
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(
            vk_logic_device,
            &image_view_create_info,
            nullptr,
            &vk_swapchain_image_views[i]) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create Vulkan Image views for the Vulkan Swapchain images! \n");
        }
    }

    std::cout << "Vulkan Image views created. \n\n";
}


SwapchainSupportDetails query_swapchain_support(VkSurfaceKHR vk_surface, VkPhysicalDevice phys_device) {

    SwapchainSupportDetails details;

    // Get Surface details (capabilities)
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, vk_surface, &details.capabilities);

    // Get formats details
    uint32_t formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, vk_surface, &formats_count, nullptr);

    if (formats_count != 0) {
        details.formats.resize(formats_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, vk_surface, &formats_count, details.formats.data());
    }

    // Get present modes details
    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, vk_surface, &present_modes_count, nullptr);

    if (present_modes_count != 0) {
        details.present_modes.resize(present_modes_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, vk_surface, &present_modes_count, details.present_modes.data());
    }

    return details;
}