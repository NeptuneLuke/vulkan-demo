// #include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // EXIT_FAILURE | EXIT_SUCCESS


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class VulkanDemo {

public:
	
	void run() {
		init_window();
		init_vulkan();
		main_loop();
		cleanup();
	}

private:

	GLFWwindow* window;

	void init_window() {

		glfwInit(); // Initializes the GLFW lib

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Specify to use VULKAN (by explicitly not using OpenGL)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable resizing window (temporary)

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan demo", nullptr, nullptr);
	}

	void init_vulkan() {
	
	}

	void main_loop() {
		
		// Checks for events until the window is closed
		while (!glfwWindowShouldClose(window)) {
			
			glfwPollEvents();  // Check for events
		}
	}

	void cleanup() {
		
		glfwDestroyWindow(window);

		glfwTerminate(); // Shutdowns the GLFW lib 
	}

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