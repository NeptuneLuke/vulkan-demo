#include "my_utils.hpp"
#include "vk_debugger.hpp"
#include "vk_core.hpp"
#include "vk_queue_family.hpp"
#include "vk_swapchain.hpp"
#include "vk_graphics_pipeline.hpp"


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
    VkCommandPool vulkan_command_pool;
    VkCommandBuffer vulkan_command_buffer; // Implicitly destroyed when vulkan_command_buffer is destroyed

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

        create_graphics_pipeline(
            vulkan_graphics_pipeline, vulkan_pipeline_layout,
            vulkan_logical_device,
            vulkan_render_pass,
            vulkan_swapchain_extent);

        create_framebuffers(vulkan_swapchain_framebuffers,
            vulkan_logical_device,
            vulkan_render_pass,
            vulkan_swapchain_image_views, vulkan_swapchain_extent);

        create_command_pool(
            vulkan_command_pool,
            vulkan_surface,
            vulkan_physical_device, vulkan_logical_device);

        create_command_buffer(
            vulkan_command_buffer,
            vulkan_command_pool,
            vulkan_logical_device);
    }

    void draw_frame() {
        

    }

    void main_loop() {

        // Checks for events until the window is closed
        while (!glfwWindowShouldClose(window)) {

            glfwPollEvents(); // Check for events

            draw_frame();
        }
    }

    void cleanup() {

        // Destroy objects in opposite order of creation.

        std::cout << "Destroying Vulkan Command pool... \n\n";
        vkDestroyCommandPool(vulkan_logical_device, vulkan_command_pool, nullptr);

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
        std::cout << "Unloading validation layers: \n";
        vkDestroyInstance(vulkan_instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate(); // Shutdowns the GLFW lib
    }
    /* ----------------------------------------------------------------- */
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