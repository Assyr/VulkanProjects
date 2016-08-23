#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
	glfwInit(); //Initialize GLFW library

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Test Window", nullptr, nullptr); //Create our window

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); //Returns up to requested number of global extension properties and stores it to extensionCount
	
	std::cout << extensionCount << " extensions supported" << std::endl; //If a none-zero value is returned to 'extensionCount', we're good to go!

	glm::mat4 matrix;
	glm::vec4 vec;

	auto test = matrix * vec;

	while (!glfwWindowShouldClose(window)) //Game loop - while window is open
	{
		glfwPollEvents(); //Process pending events...
	}

	//If game loop is broken - glfwWindowShouldClose(window) will return a flag to close
	//and the following will be executed

	glfwDestroyWindow(window); //Destroy our window

	glfwTerminate(); //Corresponding call to glfwInit to terminate GLFW library

	return 0;
}