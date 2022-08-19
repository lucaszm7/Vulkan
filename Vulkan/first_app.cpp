#include "first_app.hpp"

#include <stdexcept>
#include <array>

namespace lve
{
	FirstApp::FirstApp()
	{
		loadModels();
		createPipelineLayout();
		createPipeline();
		createCommandBuffers();
	}
	
	FirstApp::~FirstApp()
	{
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}
	
	void FirstApp::run()
	{
		while (!lveWindow.shouldClose())
		{
			glfwPollEvents();
			drawFrame();
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

		vertices.push_back({ nLeft });
		vertices.push_back({ nRight });
		vertices.push_back({ nBottom });

		createInverseSierpinskiTriangle(vertices, depth - 1, left, nBottom, nLeft);
		createInverseSierpinskiTriangle(vertices, depth - 1, nBottom, right, nRight);
		createInverseSierpinskiTriangle(vertices, depth - 1, nLeft, nRight, top);
	}

	void FirstApp::loadModels()
	{
		std::vector<LveModel::Vertex> vertices;
		createInverseSierpinskiTriangle(vertices, 10, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, -1.0f });
		lveModel = std::make_unique<LveModel>(lveDevice, vertices);
		
	}

	void FirstApp::createPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipelinelayoutInfo{};
		pipelinelayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelinelayoutInfo.setLayoutCount = 0;
		pipelinelayoutInfo.pSetLayouts = nullptr;
		pipelinelayoutInfo.pushConstantRangeCount = 0;
		pipelinelayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(lveDevice.device(), &pipelinelayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed creating pipeline layout");
	}
	
	void FirstApp::createPipeline()
	{
		PipelineConfigInfo pipelineConfig = LvePipeline::defaultPipelineConfigInfo(lveSwapChain.width(), lveSwapChain.height());
		pipelineConfig.renderPass = lveSwapChain.getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LvePipeline>(
			lveDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig
			);
	}
	
	void FirstApp::createCommandBuffers()
	{
		commandBuffers.resize(lveSwapChain.imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		
		if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed allocating command buffers");

		for (int i = 0; i < commandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			
			if(vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("Failed beggining command buffers");
			
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = lveSwapChain.getRenderPass();
			renderPassInfo.framebuffer = lveSwapChain.getFrameBuffer(i);
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = lveSwapChain.getSwapChainExtent();

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { 0.1f,0.1f,0.1f,1.0f };
			clearValues[1].depthStencil = {1.0f, 0};

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			
			lvePipeline->bind(commandBuffers[i]);
			lveModel->bind(commandBuffers[i]);
			lveModel->draw(commandBuffers[i]);
			
			vkCmdEndRenderPass(commandBuffers[i]);
			
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to record command buffer");
		}
	}
	
	void FirstApp::drawFrame()
	{
		uint32_t image_index;
		auto result = lveSwapChain.acquireNextImage(&image_index);

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Failed to acquire swap chain image");

		// Submits the Providing command buffer to the graphics queue when handling CPU and GPU sync
		// the command buffer will then be exe
		// And then the swap chain will present the assocated color attachment view to the display
		// at the opropriate time
		result = lveSwapChain.submitCommandBuffers(&commandBuffers[image_index], &image_index);

		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to present swap chain image");

	}
}
