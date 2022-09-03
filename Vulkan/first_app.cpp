#include "first_app.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <array>
#include <stdexcept>

#include <chrono>

namespace lve
{
	// Push Constants is a easy and performatic way to delivery data to vertex and fragment shader
	// but its limited in size (only 128 bytes is guaranteed)
	// and is limited per model, so we cannot combine model in a single vertex buffer
	// when using push contants
	struct SimplePushConstantData
	{
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};

	FirstApp::FirstApp()
	{
		loadModels();
		createPipelineLayout();
		recreateSwapChain(); // calls createPipeline() too
		createCommandBuffers();
	}
	
	FirstApp::~FirstApp()
	{
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}
	
	void FirstApp::run()
	{
		std::cout << "Max Push Constants Size: " << lveDevice.properties.limits.maxPushConstantsSize << "\n";
		while (!lveWindow.shouldClose())
		{
			// auto start = std::chrono::high_resolution_clock::now();
			glfwPollEvents();
			drawFrame();
			// std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
			// std::cout << "Took: " << duration.count() * 1000.0 << "ms - FPS: " << 1 / (duration.count()) << "\n";
		}
	}

	void createInverseSierpinskiTriangle(
		std::vector<LveModel::Vertex>& vertices,
		int depth,
		glm::vec2 left, glm::vec2 right, glm::vec2 top)
	{
		if (depth <= 0) return;

		auto nLeft = 0.5f * (left + top);
		auto nRight = 0.5f * (right + top);
		auto nBottom = 0.5f * (left + right);

		vertices.push_back({{ nLeft }   , {1.0f, 0.0f, 0.0f}});
		vertices.push_back({{ nRight }  , {0.0f, 1.0f, 0.0f}});
		vertices.push_back({{ nBottom } , {0.0f, 0.0f, 1.0f}});

		createInverseSierpinskiTriangle(vertices, depth - 1, left, nBottom, nLeft);
		createInverseSierpinskiTriangle(vertices, depth - 1, nBottom, right, nRight);
		createInverseSierpinskiTriangle(vertices, depth - 1, nLeft, nRight, top);
	}

	void FirstApp::loadModels()
	{
		std::vector<LveModel::Vertex> vertices;
		// createInverseSierpinskiTriangle(vertices, 10, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, -1.0f });
		vertices.push_back({ { -0.5f,  0.5f }  , { 1.0f, 0.0f, 0.0f } });
		vertices.push_back({ {  0.5f,  0.5f }  , { 0.0f, 1.0f, 0.0f } });
		vertices.push_back({ {  0.0f, -0.5f }  , { 0.0f, 0.0f, 1.0f } });
		lveModel = std::make_unique<LveModel>(lveDevice, vertices);
		
	}

	void FirstApp::createPipelineLayout()
	{

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelinelayoutInfo{};
		pipelinelayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelinelayoutInfo.setLayoutCount = 0;
		pipelinelayoutInfo.pSetLayouts = nullptr;
		pipelinelayoutInfo.pushConstantRangeCount = 1;
		pipelinelayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(lveDevice.device(), &pipelinelayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed creating pipeline layout");
	}
	
	void FirstApp::createPipeline()
	{
		PipelineConfigInfo pipelineConfig{};
		LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = lveSwapChain->getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LvePipeline>(
			lveDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig
			);
	}
	
	void FirstApp::recreateSwapChain()
	{
		auto extend = lveWindow.getExtend();
		while (extend.width == 0 || extend.height == 0)
		{
			extend = lveWindow.getExtend();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(lveDevice.device());

		if (lveSwapChain == nullptr)
			lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extend);
		else
		{
			lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extend, std::move(lveSwapChain));
			if (lveSwapChain->imageCount() != commandBuffers.size())
			{
				freeCommandBuffers();
				createCommandBuffers();
			}
		}

		createPipeline();

	}

	void FirstApp::createCommandBuffers()
	{
		commandBuffers.resize(lveSwapChain->imageCount());
		
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		
		if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed allocating command buffers");

	}

	void FirstApp::freeCommandBuffers()
	{
		vkFreeCommandBuffers(
			lveDevice.device(), 
			lveDevice.getCommandPool(), 
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}
	
	void FirstApp::recordCommandBuffer(int imageIndex)
	{
		static int frame = 0;
		frame = (frame + 1) % 1000;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed beggining command buffers");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = lveSwapChain->getRenderPass();
		renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f,0.01f,0.01f,1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set up dynamic viewPort and Scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(lveSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(lveSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, lveSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

		lvePipeline->bind(commandBuffers[imageIndex]);
		lveModel->bind(commandBuffers[imageIndex]);

		for (int j = 0; j < 4; ++j)
		{
			SimplePushConstantData push;
			push.offset = { -0.5f + frame * 0.002f * (j + 1), -0.4f + j * 0.25f};
			push.color =  {  0.0f + frame * 0.001f,  0.0f + frame * 0.01f, 0.2f + 0.2f * j * frame };

			vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
			
			lveModel->draw(commandBuffers[imageIndex]);
		}


		vkCmdEndRenderPass(commandBuffers[imageIndex]);

		if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer");
	}

	void FirstApp::drawFrame()
	{
		uint32_t image_index;
		auto result = lveSwapChain->acquireNextImage(&image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain(); 
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Failed to acquire swap chain image");

		// Submits the Providing command buffer to the graphics queue when handling CPU and GPU sync
		// the command buffer will then be exe
		// And then the swap chain will present the assocated color attachment view to the display
		// at the opropriate time
		recordCommandBuffer(image_index);
		result = lveSwapChain->submitCommandBuffers(&commandBuffers[image_index], &image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lveWindow.wasWindowResized())
		{
			lveWindow.resetWindowResizedFlag();
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to present swap chain image");

	}
}
