#include "vk_renderpass.hpp"



void create_render_pass(VkRenderPass& vk_render_pass, VkDevice vk_logic_device, VkFormat& vk_swapchain_image_format) {

    std::cout << "Creating Vulkan Render pass... \n";

    // We need to specify the framebuffer attachments that will be used while
    // rendering. We need to specify how many color and depth buffers there will be.
    // How many samples to use for each of them and how their contents should be handled 
    // throughout the rendering operations. This informations are wrapped in a render pass object.
    // In our case we will have just a single color buffer attachment represented by one of the
    // images from the swapchain.
    VkAttachmentDescription color_attachment{};
    color_attachment.format = vk_swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // we are not doing anything with multisampling

    // This are for color and depth data.
    // What to do before rendering, in our case clear the framebuffer 
    // (sets the window to black)
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // What to do after rendering, in our case render the triangle to screen (store it in the framebuffer)
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // This are for stencil data. We are not doing anything with stencils
    // so we set it in the following way:
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Which layout the image will have before render pass begins.
    // We do not care what the previous layout the image was.
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Which layout the image will have after render pass finishes.
    // We want the image to be ready for presentation using the swapchain
    // after rendering.
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // A single render pass can consist of multiple subpasses. Subpasses are subsequent
    // rendering operations that depend on the contents of framebuffers in previous
    // passes, for example a sequence of post - processing effects that are applied one
    // after another. In our case we will stick to a single subpass.
    // Every subpass references one or more of the attachments that we’ve described.
    VkAttachmentReference color_attachment_reference{};

    // Specifies which attachment to reference by its
    // index in the attachment descriptions array.Our array consists of a single
    // VkAttachmentDescription, so its index is 0.
    color_attachment_reference.attachment = 0;

    // Specifies which layout we would like the attachment to have during a subpass that uses this
    // reference. We intend to use the attachment to function as a color buffer and
    // the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout will give us the best performance,
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    if (vkCreateRenderPass(
        vk_logic_device,
        &render_pass_create_info,
        nullptr,
        &vk_render_pass) != VK_SUCCESS) {

        throw std::runtime_error("Failed to create Vulkan Render pass! \n");
    }

    std::cout << "Vulkan Render pass created. \n\n";
}