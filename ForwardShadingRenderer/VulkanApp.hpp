//
//  VulkanApp.hpp
//  ForwardRenderer
//
//  Created by macbook on 14/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#ifndef VulkanApp_hpp
#define VulkanApp_hpp

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <array>
#include <vector>
#include <cstring>
#include <random>
#include <string>
#include <chrono>

#include "VulkanVertex.hpp"
#include "Timer.hpp"

class VulkanApp
	{  // VulkanApp
public:
	VulkanApp(uint32_t width, uint32_t height, std::string title, uint32_t objects, uint32_t id, glm::vec3 clear = { 0.12f, 0.12f, 0.12f });
	~VulkanApp();

protected:
	const uint32_t             WINDOW_WIDTH;
	const uint32_t             WINDOW_HEIGHT;
	const std::string          WINDOW_TITLE;
	const std::array<float, 4> WINDOW_CLEAR;
	const uint32_t             MAX_FPS = 120;
	GLFWwindow*                window;

	Timer timing;

	uint32_t runID;

private:

	vk::Result createWindow();
	vk::Result createSceneMesh();
	vk::Result createInstance();
	vk::Result createDebugCallback();
	vk::Result createSurface();
	vk::Result createDevice();
	vk::Result createSwapChain();
	vk::Result createDepthBuffer();
	vk::Result createUniformBuffer();
	vk::Result createPipelineLayout();
	vk::Result createDescriptorSet();
	vk::Result createSemaphores();
	vk::Result createRenderPass();
	vk::Result createFrameBuffers();
	vk::Result createVertexBuffer();
	vk::Result createIndexBuffer();
	vk::Result createGraphicsPipeline();
	vk::Result createCommandBuffers();

	void arrangeObjects();

	void createPhysicsState();
	void updatePhysicsState();

	void updateUniforms();

	void report();

	void render();
	void loop();

	struct VulkanCore {
		vk::Instance       instance;
		vk::PhysicalDevice physicalDevice;
		vk::Device         logicalDevice;
	} core;

	struct VulkanCommandState {
		vk::CommandPool   pool;
	} command;

	struct VulkanQueues {
		vk::Queue graphics; uint32_t graphicsIndex = UINT32_MAX;
		vk::Queue compute;  uint32_t computeIndex = UINT32_MAX;
		vk::Queue transfer; uint32_t transferIndex = UINT32_MAX;
		vk::Queue present;  uint32_t presentIndex = UINT32_MAX;
	} queues;

	struct VulkanSwapChain {
		vk::SurfaceKHR   surface;
		vk::SwapchainKHR swapchain;
		vk::Extent2D     extent;
		vk::Format       format;

		std::vector<vk::CommandBuffer> commandBuffers;
		std::vector<vk::Framebuffer>   framebuffers;
		std::vector<vk::ImageView>     views;
		std::vector<vk::Image>         images;

		uint32_t currentImage = 0;
		uint32_t nImages = 0;

		struct Support {
			vk::SurfaceCapabilitiesKHR      capabilities;
			std::vector<vk::SurfaceFormatKHR>    formats;
			std::vector<vk::PresentModeKHR> presentModes;
		} supported;
	} swapchain;

	struct VulkanGraphicsPipeline {
		vk::RenderPass renderPass;

		vk::Format pixelFormat;
		vk::Format depthFormat;

		std::vector<vk::DescriptorSetLayout> layouts;
		vk::DescriptorPool      descriptorPool;
		vk::DescriptorSet       descriptorSet;
		vk::PipelineLayout      layout;
		vk::Pipeline            pipeline;
	} graphics;

	struct VulkanShaderModules {
		vk::ShaderModule vertex;
		vk::ShaderModule fragment;
	} shaders;

	struct VulkanDepthBuffer {
		vk::Image          image;
		vk::DeviceMemory   memory;
		vk::ImageView      view;
	} depth;

	struct VulkanSemaphores {
		vk::Semaphore imageAvailable;
		vk::Semaphore renderFinished;
	} semaphores;

	struct VulkanBuffers {
		struct VulkanBuffer {
			vk::Buffer       buffer;
			vk::DeviceMemory memory;
		};
		VulkanBuffer uniform;
		VulkanBuffer vertex;
		VulkanBuffer index;
	} buffers;

	VkDebugReportCallbackEXT callback;

	static constexpr uint32_t maxObjects = 64;
	const uint32_t nObjects;
	static constexpr float offset = 2.5f;

	struct UniformBufferObject {
		glm::mat4 model[maxObjects];
		glm::mat4 view;
		glm::mat4 proj;

		glm::vec3 lightPosition;
		glm::vec3 eyePosition;

		glm::vec4 materials[maxObjects];

	} ubo;

	struct VulkanMeshes {
		std::vector<Vertex>   vertices;
		std::vector<uint32_t>  indices;
	} meshes;

	struct PhysicsData {
		std::vector<glm::vec3> positions;  // bounding sphere centroids
		std::vector<glm::vec3> velocities; // derivatives of the motion

		std::vector<glm::vec3> orientations; //
		std::vector<glm::vec3> rotations;    // angular velocities
		float bounds = 0.0f;                          // size of the bounding spheres
	} simulation;

	struct InputParameters {
		float movementSpeed = 0.1f;
	} parameters;

	struct ArrangementData {
		std::vector<glm::vec3> translations;
		glm::vec3 centre;
		bool arranged = false;
	} arrangement;
    
    void createBuffer (
        vk::DeviceSize          size,
        vk::BufferUsageFlags    usage,
        vk::MemoryPropertyFlags properties,
        vk::Buffer&             buffer,
        vk::DeviceMemory        memory);
    
    std::default_random_engine rng;
    
    }; // VulkanApp

#endif /* VulkanApp_hpp */
