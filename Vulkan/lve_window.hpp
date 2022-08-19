#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace lve
{
	class LveWindow
	{
	public:
		LveWindow(int w, int h, std::string name);
		~LveWindow();

		LveWindow(const LveWindow&) = delete;
		LveWindow& operator=(const LveWindow&) = delete;

		bool shouldClose() { return glfwWindowShouldClose(window); }
		bool wasWindowResized() { return frameBufferResized; }
		void resetWindowResizedFlag() { frameBufferResized = false; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

		VkExtent2D getExtend() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }

	private:
		void initWindow();
		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

		GLFWwindow* window;

		int width;
		int height;
		bool frameBufferResized = false;

		std::string windowName;
	};
}