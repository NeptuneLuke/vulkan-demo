#include "my_utils.hpp"
#include "vk_debugger.hpp"
#include "vk_core.hpp"
#include "vk_queue_family.hpp"
#include "vk_swapchain.hpp"
#include "vk_renderpass.hpp"


#include <stdexcept>
#include <cstdlib> // EXIT_FAILURE | EXIT_SUCCESS
#include <cstring> // strcmp
#include <cstdint> // uint32_t
#include <optional>
#include <algorithm> // std::clamp
#include <limits> // std::numeric_limits
#include <vector>
#include <set>


/* ----------------------------------------------------------------- */
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
/* ----------------------------------------------------------------- */


class VulkanDemo {

public:

    void run() {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

private:

    /* ----------------------------------------------------------------- */
    VkInstance vulkan_instance;
    VkSurfaceKHR vulkan_surface;

    VkPhysicalDevice vulkan_physical_device = VK_NULL_HANDLE; // Implicitly destroyed when vulkan_instance is destroyed
    VkDevice vulkan_logical_device;

    // Implicitly destroyed when vulkan_logical_device is destroyed.
    VkQueue vulkan_graphics_queue;
    VkQueue vulkan_present_queue;

    VkSwapchainKHR vulkan_swapchain;

    // Implicitly destroyed when vulkan_swapchain is destroyed
    std::vector<VkImage> vulkan_swapchain_images;
    std::vector<VkImageView> vulkan_swapchain_image_views;
    VkFormat vulkan_swapchain_image_format;
    VkExtent2D vulkan_swapchain_extent;

    VkPipeline vulkan_graphics_pipeline;
    VkPipelineLayout vulkan_pipeline_layout;
    VkRenderPass vulkan_render_pass;
    std::vector<VkFramebuffer> vulkan_swapchain_framebuffers;

    VkDebugUtilsMessengerEXT vulkan_debugger_messenger;

    GLFWwindow* window;
    /* ----------------------------------------------------------------- */


    /* ----------------------------------------------------------------- */
    void init_window() {

        glfwInit(); // Initializes the GLFW lib

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Specify to use VULKAN (by explicitly not using OpenGL)

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable resizing window (temporary)

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan demo", nullptr, nullptr);
    }

    void init_vulkan() {

        create_vulkan_instance(vulkan_instance);

        create_debug_messenger(vulkan_debugger_messenger, vulkan_instance);
        
        create_vulkan_surface(vulkan_surface, vulkan_instance, window);
        
        select_physical_device(vulkan_physical_device, vulkan_instance, vulkan_surface);
        
        create_vulkan_logical_device(
            vulkan_logical_device,
            vulkan_surface,
            vulkan_physical_device,
            vulkan_graphics_queue, vulkan_present_queue);
        
        create_vulkan_swapchain(
            vulkan_swapchain,
            window, vulkan_surface,
            vulkan_physical_device, vulkan_logical_device,
            vulkan_swapchain_images, vulkan_swapchain_image_format, vulkan_swapchain_extent);
        
        create_swapchain_image_views(vulkan_swapchain_image_views, vulkan_logical_device, vulkan_swapchain_images, vulkan_swapchain_image_format);

        create_render_pass(vulkan_render_pass, vulkan_logical_device, vulkan_swapchain_image_format);

        create_graphics_pipeline();

        create_framebuffers();
    }

    void main_loop() {

        // Checks for events until the window is closed
        while (!glfwWindowShouldClose(window)) {

            glfwPollEvents(); // Check for events
        }
    }

    void cleanup() {

        // Destroy objects in opposite order of creation.

        // Delete the framebuffers  before the image views and render pass they 
        // are based on, but only after the rendering is finished.
        std::cout << "Destroying Vulkan Swapchain framebuffers... \n\n";
        for (auto framebuffer : vulkan_swapchain_framebuffers) {
            vkDestroyFramebuffer(vulkan_logical_device, framebuffer, nullptr);
        }

        std::cout << "Destroying Vulkan Graphics Pipeline... \n\n";
        vkDestroyPipeline(vulkan_logical_device, vulkan_graphics_pipeline, nullptr);

        std::cout << "Destroying Vulkan Pipeline Layout... \n\n";
        vkDestroyPipelineLayout(vulkan_logical_device, vulkan_pipeline_layout, nullptr);

        std::cout << "Destroying Vulkan Render pass... \n\n";
        vkDestroyRenderPass(vulkan_logical_device, vulkan_render_pass, nullptr);

        std::cout << "Destroying Vulkan Image views... \n\n";
        for (auto img_view : vulkan_swapchain_image_views) {
            vkDestroyImageView(vulkan_logical_device, img_view, nullptr);
        }

        std::cout << "Destroying Vulkan Swapchain... \n\n";
        vkDestroySwapchainKHR(vulkan_logical_device, vulkan_swapchain, nullptr);

        std::cout << "Destroying Vulkan Logical device... \n\n";
        vkDestroyDevice(vulkan_logical_device, nullptr);

        if (ENABLE_VALIDATION_LAYERS) {
            std::cout << "Destroying Vulkan Debug messenger... \n\n";
            destroy_debug_messenger(vulkan_debugger_messenger, vulkan_instance, nullptr);
        }

        std::cout << "Destroying Vulkan Surface (Win32)... \n\n";
        vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, nullptr);

        std::cout << "Destroying Vulkan Instance... \n\n";
        vkDestroyInstance(vulkan_instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate(); // Shutdowns the GLFW lib
    }
    /* ----------------------------------------------------------------- */

    /* ----------------------------------------------------------------- */

    

    void create_graphics_pipeline() {
        
        std::cout << "Creating the Vulkan Graphics Pipeline ... \n\n";

        std::cout << "\t Creating the Vulkan Pipeline Layout... \n\n";

        auto vert_shader_bytecode = read_file("vert.spv");
        auto frag_shader_bytecode = read_file("frag.spv");

        std::cout << "\t\t Vert shader file size: " << vert_shader_bytecode.size() << " bytes. \n";
        std::cout << "\t\t Frag shader file size: " << frag_shader_bytecode.size() << " bytes. \n\n";

        std::cout << "\t\t Creating the shader modules... \n";
        VkShaderModule vert_shader_module = create_shader_module(vert_shader_bytecode);
        VkShaderModule frag_shader_module = create_shader_module(frag_shader_bytecode);
        
        if (vert_shader_module == VK_NULL_HANDLE) {
            throw std::runtime_error("Vert shader not created! \n");
        }
        else if (frag_shader_module == VK_NULL_HANDLE) {
            throw std::runtime_error("Frag shader not created! \n");
        }
        else {
            std::cout << "\t\t Shader modules created. \n\n";
        }


        std::cout << "\t\t Creating the shader stages... \n";
        // To actually use the shaders we will need to assign them to a specific
        // pipeline stage.
        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module;
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
        frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module;
        frag_shader_stage_info.pName = "main";
        
        VkPipelineShaderStageCreateInfo shader_stages[] = { 
            vert_shader_stage_info,
            frag_shader_stage_info };
        std::cout << "\t\t Shader stages created.  \n\n";

        // While most of the pipeline state needs to be baked into the pipeline static state, a
        // limited amount of the state can actually be dynamic, changing it without recreating the
        // pipeline at draw time. If we want to use dynamic state:
        std::vector<VkDynamicState> dynamic_state;

        VkPipelineDynamicStateCreateInfo dynamic_states_create_info{};
        dynamic_states_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_states_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state.size());
        dynamic_states_create_info.pDynamicStates = dynamic_state.data();

        // The VkPipelineVertexInputStateCreateInfo structure describes the format
        // of the vertex data that will be passed to the vertex shader.
        // Because we’re hard coding the vertex data directly in the vertex shader, we’ll
        // fill in this structure to specify that there is no vertex data to load for now.
        // The pVertexBindingDescriptions and pVertexAttributeDescriptions
        // members point to an array of structs that describe the aforementioned details
        // for loading vertex data.
        VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
        vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_create_info.vertexBindingDescriptionCount = 0;
        vertex_input_create_info.pVertexBindingDescriptions = nullptr;
        vertex_input_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

        // The VkPipelineInputAssemblyStateCreateInfo struct describes two things:
        // what kind of geometry will be drawn from the vertices and if primitive restart
        // should be enabled.
        // Normally, the vertices are loaded from the vertex buffer by index in sequential
        // order, but with an element buffer you can specify the indices to use yourself.
        // This allows you to perform optimizations like reusing vertices.
        // We intend to draw triangles throughout this tutorial, so we’ll stick to the 
        // following data for the structure :
        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
        input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

        // A viewport describes the region of the framebuffer that the output
        // will be rendered to. This will almost always be (0, 0) to (width, height)
        // and in this tutorial will also be the case.
        // Remember that the size of the swapchain and its images may differ from the WIDTH
        // and HEIGHT of the window. The swapchain images will be used as framebuffers later on,
        // so we should stick to their size.
        // The minDepth and maxDepth values specify the range of depth values to use
        // the framebuffer.These values must be within the[0.0f, 1.0f] range, but
        // minDepth may be higher than maxDepth.If you aren’t doing anything special,
        // then you should stick to the standard values of 0.0f and 1.0f.
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) vulkan_swapchain_extent.width;
        viewport.height = (float) vulkan_swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // While viewports define the transformation from the image to the framebuffer,
        // scissor rectangles define in which regions pixels will actually be stored.
        // Any pixels outside the scissor rectangles will be discarded by the rasterizer.
        // They function like a filter rather than a transformation.
        // If we want to draw to the entire framebuffer we should specify a scissor
        // rectangle that covers it entirely.
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vulkan_swapchain_extent;

        // Most of the times viewport and scissor are set as dynamic state in the command buffer
        // rather than as a static part of the pipeline.
        dynamic_state = {
           VK_DYNAMIC_STATE_VIEWPORT,
           VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state.size());
        dynamic_state_create_info.pDynamicStates = dynamic_state.data();

        // Specify their count at pipeline creation time.
        // With dynamic state, the actual viewport(s) and scissor rectangle(s) will be set up at draw time.
        // Without dynamic state, the viewport and scissor rectangle need to be set in the
        // pipeline using the VkPipelineViewportStateCreateInfo struct.This makes
        // the viewport and scissor rectangle for this pipeline immutable.
        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.viewportCount = 1;
        // viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        // viewport_state_create_info.pScissors = &scissor;

        std::cout << "\t\t Creating Vulkan Rasterizer... \n";
        // The rasterizer takes the geometry that is shaped by the vertices from the
        // vertex shader and turns it into fragments to be colored by the fragment shader.
        VkPipelineRasterizationStateCreateInfo rasterizer_create_info{};
        rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer_create_info.depthClampEnable = VK_FALSE;

        // Geometry passes through the rasterizer stage, basically disabling any output
        // to the framebuffer.
        rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;

        // Determines how fragments are generated from geometry.
        // Fill the area of the polygon with fragments.
        rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer_create_info.lineWidth = 1.0f; // thickness of line in terms of number of fragments
        rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT; // enables back-face culling

        // Specifies the vertex order for faces to be considered front-facing
        // and can be clockwise/counterclockwise.
        rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer_create_info.depthBiasEnable = VK_FALSE;
        std::cout << "\t\t Vulkan Rasterizer created. \n\n";


        // Multisampling is one of the ways to perform anti-aliasing. It works by combining
        // the fragment shader results of multiple polygons that rasterize to the same pixel.
        // This mainly occurs around edges, which is also where the most noticeable aliasing
        // artifacts occur.
        // For now we will keep it disabled, and revisit it later.
        VkPipelineMultisampleStateCreateInfo multisampling_create_info{};
        multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_create_info.sampleShadingEnable = VK_FALSE;
        multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // After a fragment shader has returned a color, it needs to be combined with the color
        // already present in the framebuffer.
        // There are two types of structs to configure color blending.
        // VkPipelineColorBlendAttachmentState contains the config per attached framebuffer
        // VkPipelineColorBlendStateCreateInfo contains the global color blending settings
        // We only have one framebuffer.
        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;

        // The second structure references the array of structures for all of the framebuffers
        VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
        color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending_create_info.logicOpEnable = VK_FALSE;
        color_blending_create_info.attachmentCount = 1;
        color_blending_create_info.pAttachments = &color_blend_attachment;

        // You can use uniform values in shaders, which are globals similar to dynamic
        // state variables that can be changed at drawing time to alter the behavior of
        // your shaders without having to recreate them.
        // These uniform values need to be specified during pipeline creation by creating a
        // VkPipelineLayout object.Even though we won’t be using them until a future
        // chapter, we are still required to create an empty pipeline layout.
        VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if (vkCreatePipelineLayout(
            vulkan_logical_device,
            &pipeline_layout_create_info,
            nullptr,
            &vulkan_pipeline_layout) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create Vulkan Pipeline Layout! \n");
        }
       
        std::cout << "\t Vulkan Pipeline Layout created. \n\n";

        // We create the Graphics pipeline using all the previously built
        // structs describing the fixed-function stage.
        VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{};
        graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphics_pipeline_create_info.stageCount = 2;
        graphics_pipeline_create_info.pStages = shader_stages;
        graphics_pipeline_create_info.pVertexInputState = &vertex_input_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
        graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &rasterizer_create_info;
        graphics_pipeline_create_info.pMultisampleState = &multisampling_create_info;
        graphics_pipeline_create_info.pDepthStencilState = nullptr;
        graphics_pipeline_create_info.pColorBlendState = &color_blending_create_info;
        graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;

        graphics_pipeline_create_info.layout = vulkan_pipeline_layout;
        graphics_pipeline_create_info.renderPass = vulkan_render_pass;
        // index of the subpass where the graphics pipeline will be used
        graphics_pipeline_create_info.subpass = 0;

        if (vkCreateGraphicsPipelines(
            vulkan_logical_device,
            VK_NULL_HANDLE,
            1,
            &graphics_pipeline_create_info,
            nullptr,
            &vulkan_graphics_pipeline) != VK_SUCCESS) {
            
            throw std::runtime_error("Failed to create Vulkan Graphics Pipeline! \n");
        }

        std::cout << "Vulkan Graphics Pipeline created. \n\n";

        // We can destroy the shader modules as soon as the pipeline
        // is finished.
        std::cout << "Destroying shader modules... \n\n";
        vkDestroyShaderModule(vulkan_logical_device, vert_shader_module, nullptr);
        vkDestroyShaderModule(vulkan_logical_device, frag_shader_module, nullptr);
    }

    // Before we can pass the code to the pipeline,
    // we have to wrap it in a VkShaderModule object.
    VkShaderModule create_shader_module(const std::vector<char>& shader_code) {

        VkShaderModuleCreateInfo shader_module_create_info{};
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = shader_code.size();
        // The size of the shader bytecode (a char specified in bytes) is different from the bytecode pointer
        // (a uint32_t) so we need to cast it with reinterpret_cast.
        // Technically with a cast like this we should also need to ensure data satisfies the 
        // alignment requirement of uint32_t, but the std::vector default allocator already ensures it.
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

        VkShaderModule shader_module;

        if (vkCreateShaderModule(
            vulkan_logical_device,
            &shader_module_create_info,
            nullptr,
            &shader_module) != VK_SUCCESS) {
            
            throw std::runtime_error("Failed to create the shader module! \n");
        }

        return shader_module;
    }

    void create_framebuffers() {
        
        std::cout << "Creating Vulkan Swapchain framebuffers... \n\n";

        vulkan_swapchain_framebuffers.resize(vulkan_swapchain_image_views.size());

        for (size_t i = 0; i < vulkan_swapchain_image_views.size(); i++) {
        
            VkImageView attachments[] = { vulkan_swapchain_image_views[i] };

            // You can only use a framebuffer with the render passes that it is compatible
            // with, so they roughly use the same number and type of attachments.
            VkFramebufferCreateInfo framebuffer_create_info{};
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.renderPass = vulkan_render_pass;
            framebuffer_create_info.attachmentCount = 1;
            framebuffer_create_info.pAttachments = attachments;
            framebuffer_create_info.width = vulkan_swapchain_extent.width;
            framebuffer_create_info.height = vulkan_swapchain_extent.height;
            framebuffer_create_info.layers = 1; // number of layers in image arrays
            
            if (vkCreateFramebuffer(
                    vulkan_logical_device,
                    &framebuffer_create_info,
                    nullptr,
                    &vulkan_swapchain_framebuffers[i]) != VK_SUCCESS) {

                throw std::runtime_error("Failed to create Vulkan Swapchain framebuffers! \n");
            }
        }

        std::cout << "Vulkan Swapchain framebuffers created. \n\n";
    }

};


int main() {

    VulkanDemo demo;

    try {
        demo.run();
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}