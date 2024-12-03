// #include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // EXIT_FAILURE | EXIT_SUCCESS
#include <vector>


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
		create_vulkan_instance();
	}

	void main_loop() {
		
		// Checks for events until the window is closed
		while (!glfwWindowShouldClose(window)) {
			
			glfwPollEvents();  // Check for events
		}
	}

	void cleanup() {
		
		vkDestroyInstance(vulkan_instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate(); // Shutdowns the GLFW lib 
	}
	/* ----------------------------------------------------------------- */


	/* ----------------------------------------------------------------- */
	void create_vulkan_instance() {
		
		std::cout << "Creating Vulkan Instance... \n";

		// Get the application info
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.apiVersion = VK_API_VERSION_1_3; // Vulkan API version
		app_info.pApplicationName = "Hello Triangle";
		app_info.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);

		/* [START] Get the extensions and validation layers - Not optional! */
		VkInstanceCreateInfo instance_create_info{};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo = &app_info;

		// Get the extensions required. Note that GLFW has a built in
		// function to get the required extensions needed.
		std::cout << "Getting extensions... \n\n";

		// Get all available extensions for our system
		uint32_t available_extensions_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(available_extensions_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, available_extensions.data());
		
		#if defined(_DEBUG)
				std::cout << "Available extensions: " << available_extensions_count << ".\n";
				std::cout << "Listing all extensions: \n";
				for (const auto& ext : available_extensions) {
					std::cout << ext.extensionName << " | v." << ext.specVersion << " \n";
				}
				std::cout << "\n\n";
		#endif

		// Extensions required by GLFW for the Vulkan Instance
		uint32_t glfw_extensions_count = 0;
		const char** glfw_extensions;
		
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
		instance_create_info.enabledExtensionCount = glfw_extensions_count;
		instance_create_info.ppEnabledExtensionNames = glfw_extensions;

		#if defined(_DEBUG)
			std::cout << "Extensions obtained by GLFW: " << glfw_extensions_count << ".\n";
			std::cout << "Listing all extensions: \n";
			for (uint32_t i = 0; i < glfw_extensions_count; i++) {
				std::cout << glfw_extensions[i] << " \n";
			}
			std::cout << "\n";
		#endif

		// Validation layers to enable
		instance_create_info.enabledLayerCount = 0; // set to 0 only temporary

		/* [END] Get the extensions and validation layers - Not optional! */

		// We have specified everything Vulkan needs to create an instance
		VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance);
		
		// Check if the instance is properly created
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create the Vulkan Instance!");
		}

		std::cout << "Vulkan Instance created. \n";
	}
	/* ----------------------------------------------------------------- */
};



int main() {
	
	VulkanDemo demo;

	try {
		demo.run();
	}
	catch (const std::exception &ex) {
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}