//
//  VulkanApp.cpp
//  ForwardRenderer
//
//  Created by macbook on 14/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//
#include <glm/gtc/matrix_transform.hpp>

#include "ErrorHandler.hpp"
#include "VulkanHelpers.hpp"
#include "VulkanShaders.hpp"
#include "VulkanDebug.hpp"
#include "VulkanApp.hpp"

#include "MeshIO.hpp"

#include <sstream>
#include <stdlib.h>
#include <random>

glm::vec3 lightPosition = { 2.0f, -3.0f, 1.0f };
glm::vec3 eyePosition   = { 0.0f, 16.0f, 0.0f };

//
//  The vulkan API allows us to opt in or out of
//  it's validation suite and offers good granularity
//  for which (if any) validation layers we want to
//  activate.
//
//  DEBUG is a constant available in most IDE's that
//  is defined when an executable is built as debug.
//  We use it to set a global boolean to avoid using
//  the macro definition too much and overworking the
//  preprocessor
//
#ifdef DEBUG
    const bool DEBUG_MODE = true;
    const std::vector<const char*> requestedValidation =
        { "VK_LAYER_LUNARG_standard_validation" };
#else
    const bool DEBUG_MODE = false;
    const std::vector<const char*> requestedValidation =
        {  };
#endif

uint8_t forwards   = 0;
uint8_t backwards  = 0;

uint8_t left       = 0;
uint8_t right      = 0;

uint8_t up         = 0;
uint8_t down       = 0;

bool animateLights = true;

uint32_t reset = 0;

bool regenerateMaterials = false;

void keyCallback (GLFWwindow* window, int key, int scancode, int action, int mods)
    {
    if (key == GLFW_KEY_W)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) forwards = 1;
        if (action == GLFW_RELEASE) forwards = 0;
        }

    if (key == GLFW_KEY_S)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) backwards = 1;
        if (action == GLFW_RELEASE) backwards = 0;
        }
        
    if (key == GLFW_KEY_A)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) left = 1;
        if (action == GLFW_RELEASE) left = 0;
        }

    if (key == GLFW_KEY_D)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) right = 1;
        if (action == GLFW_RELEASE) right = 0;
        }

    if (key == GLFW_KEY_Q)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) up = 1;
        if (action == GLFW_RELEASE)  up = 0;
        }

    if (key == GLFW_KEY_E)
        {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) down = 1;
        if (action == GLFW_RELEASE) down = 0;
        }

	if (key == GLFW_KEY_F)
		{
		if (action == GLFW_PRESS) regenerateMaterials = true;
		if (action == GLFW_RELEASE) regenerateMaterials = false;
		}
        
    if (key == GLFW_KEY_SPACE)
        {
		if (action == GLFW_PRESS) animateLights = !animateLights;
        }

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		reset = (reset + 1) % 3;
    }

//
//  constructor
//
//      width   - horizontal size of the window
//      height  - vertical size of the window
//      title   - string to display in menu bar
//      clear   - the colour to clear the screen with each frame
//
//  creates a window with a vulkan context configured for a forward
//  shading architecture
//
VulkanApp::VulkanApp (uint32_t width, uint32_t height, std::string title, uint32_t objects, uint32_t id, glm::vec3 clear):
        WINDOW_WIDTH  (width),
        WINDOW_HEIGHT (height),
        WINDOW_TITLE  (title),
        WINDOW_CLEAR  ({ clear.x, clear.y, clear.z, 1.0f }),
        window        (nullptr),
		timing        (MAX_FPS),
		nObjects      (objects)
    { // VulkanApp :: VulkanApp

	runID = id;
	timing.id = runID;
    
    if (createWindow           ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("GLFW Window Creation failure");
    if (createSceneMesh        ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Failed to prepare a mesh");
    if (createInstance         ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Vulkan instance creation failure");
    if (createDebugCallback    ()  != vk::Result::eSuccess) ErrorHandler::nonfatal ("Validation disabled");
    if (createSurface          ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Surface KHR creation failed");
    if (createDevice           ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Device creation failure");
    if (createSwapChain        ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Swapchain Creation failure");
    if (createDepthBuffer      ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Depth Buffer Creation failure");
    if (createUniformBuffer    ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Uniform Buffer Creationn failure");
    if (createPipelineLayout   ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Pipeline Layout Creation failure");
    if (createDescriptorSet    ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Descriptor Set Creation failure");
    if (createSemaphores       ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Semaphore creation failure");
    if (createRenderPass       ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Render Pass Creation failure");
    if (createFrameBuffers     ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Frame Buffer Creation failure");
    if (createVertexBuffer     ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Vertex Buffer Creation failure");
    if (createIndexBuffer      ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Index Buffer Creation failure");
    if (createGraphicsPipeline ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Graphics Pipeline Creation failure");
    if (createCommandBuffers   ()  != vk::Result::eSuccess) ErrorHandler::fatal    ("Command Pool/Buffer creation failure");


	createPhysicsState();

	arrangeObjects();

    loop ();
    
    } // VulkanApp :: VulkanApp


//
//  desctructor
//
//  destroys the vulkan instance and deallocates it's state
//
VulkanApp::~VulkanApp ()
    { // VulkanApp :: ~VulkanApp
    
    // destroy graphics pipeline
    core.logicalDevice.destroyPipeline(graphics.pipeline);
    
    // destroy semaphores
    core.logicalDevice.destroySemaphore(semaphores.imageAvailable);
    core.logicalDevice.destroySemaphore(semaphores.renderFinished);
    
    // destroy vertex buffer
    core.logicalDevice.destroyBuffer(buffers.vertex.buffer);
    core.logicalDevice.freeMemory(buffers.vertex.memory);

    // destroy index buffer
    core.logicalDevice.destroyBuffer(buffers.index.buffer);
    core.logicalDevice.freeMemory(buffers.index.memory);

    // destroy framebuffers
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        core.logicalDevice.destroyFramebuffer(swapchain.framebuffers[i]);
    
    // destroy render pass
    core.logicalDevice.destroyRenderPass(graphics.renderPass);
    
    // destroy descriptor pool
    core.logicalDevice.destroyDescriptorPool(graphics.descriptorPool);
    
    // destroy pipeline layout
    for (uint32_t i = 0; i < graphics.layouts.size(); ++i)
        core.logicalDevice.destroyDescriptorSetLayout(graphics.layouts[i]);
    core.logicalDevice.destroyPipelineLayout(graphics.layout);
    
    // destroy uniform buffer
    core.logicalDevice.destroyBuffer(buffers.uniform.buffer);
   // core.logicalDevice.freeMemory(buffers.uniform.memory);
    
    // destroy depth buffer
    core.logicalDevice.destroyImageView(depth.view);
    core.logicalDevice.destroyImage(depth.image);
    core.logicalDevice.freeMemory(depth.memory);
    
    // destroy swap chain
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        core.logicalDevice.destroyImageView(swapchain.views[i]);
    core.logicalDevice.destroySwapchainKHR(swapchain.swapchain);
    
    // destroy command buffers
 //   core.logicalDevice.freeCommandBuffers(command.pool, 1, &command.buffer);
    core.logicalDevice.destroyCommandPool(command.pool, nullptr);
    
  //  VulkanDebug::DestroyDebugReportCallbackEXT(core.instance, callback, nullptr);
    
  //  vk::ObjectDestroy<vk::Device>   (device);
  //  vk::ObjectDestroy<vk::Instance> (instance);
    
    // destroy glfw stuff
    glfwDestroyWindow(window);
    glfwTerminate();
    
    } // VulkanApp :: ~VulkanApp


//
//  createWindow
//
//  uses the constants defined at creation to
//  initialize a window using glfw
//
vk::Result VulkanApp::createWindow ()
    { // VulkanApp :: createWindow
    
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT,
        WINDOW_TITLE.c_str(),
        nullptr, nullptr);
        
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    
    if (window == nullptr)
        return vk::Result::eIncomplete;
    else
        return vk::Result::eSuccess;
    
    } // VulkanApp :: createWindow


//
//  createSceneMesh
//
//  initializes a mesh to render
//
vk::Result VulkanApp::createSceneMesh ()
    { // VulkanApp :: createSceneMesh

	std::uniform_int_distribution<int> dist(0, 3);

	// the four meshes are pre-loaded and selected from
	// at random for each object, the selected mesh is then
	// batched into the render mesh
	std::vector<std::vector<Vertex>>   vBuffers(1);
	std::vector<std::vector<uint32_t>> iBuffers(1);

	for (uint32_t i = 0; i < 1; ++i)
		{ // for each mesh

		std::string path = "models/bust_";
		path += std::to_string(i);
		path += ".mesh";

		MeshIO::readMeshFile(path.c_str(), vBuffers[i], iBuffers[i]);

		} // for each mesh

	for (uint32_t i = 0; i < nObjects; ++i)
		{ // for each objectssss

		uint32_t model = 0;
		MeshIO::assign(vBuffers[model], i);
		MeshIO::merge(meshes.vertices, meshes.indices, vBuffers[model], iBuffers[model]);

		} // for each object

	MeshIO::atlas(meshes.vertices, nObjects, 1080);

	return vk::Result::eSuccess;
    
    } // VulkanApp :: createSceneMesh


//
//  createInstance
//
//  constructs an instance of the Vulkan API with
//  the appropriate extensions, validation layers
//  and the versions of both the API and the app
//  itself
//
vk::Result VulkanApp::createInstance ()
    { // VulkanApp :: createInstance
    vk::Result result = vk::Result::eSuccess;
    
    // our windowing framework will/may want some extentions
    // to be enabled and lets us query it for these C style
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);
    
    // we move the extentions requested by the windowing
    // library to a dynamic array and append our own
    // desired extensions
    std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back (VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    // vulkan will want to know what version of the API it
    // should, and some semantic information about the app
    vk::ApplicationInfo appInfo          = { };
        appInfo.pApplicationName             = WINDOW_TITLE.c_str();
        appInfo.applicationVersion           = 1;
        appInfo.pEngineName                  = "No Engine";
        appInfo.engineVersion                = 1;
        appInfo.apiVersion                   = VK_API_VERSION_1_0;
    
    // we package everything the instance will want to know
    // into it's creation information structure and call
    // the initialization function
    vk::InstanceCreateInfo instanceInfo  = { };
        instanceInfo.pApplicationInfo        = &appInfo;
        instanceInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        instanceInfo.ppEnabledExtensionNames = extensions.data();
        instanceInfo.enabledLayerCount       = static_cast<uint32_t>(requestedValidation.size());
        instanceInfo.ppEnabledLayerNames     = requestedValidation.data();

    result = vk::createInstance(&instanceInfo, nullptr, &core.instance);
    
    if (result != vk::Result::eSuccess)
        ErrorHandler::nonfatal("error code: " + to_string(result));

    return result;

    } // VulkanApp :: createInstance


//
//  createDebugCallback
//
//  sets up a user-defined error reporting function
//
vk::Result VulkanApp::createDebugCallback ()
    { // VulkanApp :: createDebugCallback
    if (!DEBUG_MODE) return vk::Result::eNotReady;
    
    vk::Result result = vk::Result::eSuccess;

    VkDebugReportCallbackCreateInfoEXT createInfo = { };
        createInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = VulkanDebug::debugCallback;

    result = static_cast<vk::Result>(VulkanDebug::CreateDebugReportCallbackEXT((VkInstance)core.instance, &createInfo, nullptr, &callback));

    return result;
        
    } // VulkanApp :: createDebugCallback


//
//  createSurface
//
//  we have to break the c++ bindings to get glfw
//  to create a surface for us. It's ugly, but it's
//  concise compared to manually sorting the surface
//
vk::Result VulkanApp::createSurface ()
    { // VulkanApp :: createSurface
    vk::Result result = vk::Result::eSuccess;

    VkSurfaceKHR surf = VkSurfaceKHR(swapchain.surface);
    result = static_cast<vk::Result>(glfwCreateWindowSurface(
        static_cast<VkInstance>(core.instance),
        window,
        nullptr,
        &surf));
    swapchain.surface = (vk::SurfaceKHR)surf;
    
    return result;
    
    } // VulkanApp :: createSurface


//
//  createDevice
//
//  sets up the physical and logical vulkan devices, as
//  well as populating the queue indices object with the
//  index of relevant queues on the physical device
//
vk::Result VulkanApp::createDevice ()
    { // VulkanApp :: createDevice
    vk::Result result = vk::Result::eSuccess;
    
    // Get and check the physical devices on the system
    // but we'll just use the first device we find (for now)
    std::vector<vk::PhysicalDevice> candidateDevices =
        core.instance.enumeratePhysicalDevices();
        
    if (candidateDevices.size() < 1)
        return vk::Result::eIncomplete;

    core.physicalDevice = candidateDevices[0];
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        core.physicalDevice.getQueueFamilyProperties();
    
    // we populate a struct with data about the queues
    // we'll want to use for our application
    vk::DeviceQueueCreateInfo graphicsCreateInfo = { };
    vk::DeviceQueueCreateInfo computeCreateInfo  = { };
    vk::DeviceQueueCreateInfo transferCreateInfo = { };
    vk::DeviceQueueCreateInfo presentCreateInfo  = { };
    
    vk::Bool32 graphicsFound = VK_FALSE;
    vk::Bool32 computeFound  = VK_FALSE;
    vk::Bool32 transferFound = VK_FALSE;
    vk::Bool32 presentFound  = VK_FALSE;
    
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
        { // for each queue family
        
        // check if the queue supports graphics
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && !graphicsFound)
            { // found graphics queue family
            graphicsCreateInfo.queueFamilyIndex = i;
            queues.graphicsIndex                = i;
            graphicsFound                       = VK_TRUE;
            } // found graphics queue family
            
        // check if the queue supports compute
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) && !computeFound)
            { // found compute queue family
            computeCreateInfo.queueFamilyIndex = i;
            queues.computeIndex                = i;
            computeFound                       = VK_TRUE;
            } // found compute queue family
            
        // check if the queue supports transfer
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) && !transferFound)
            { // found transfer queue family
            transferCreateInfo.queueFamilyIndex = i;
            queues.transferIndex                = i;
            transferFound                       = VK_TRUE;
            } // found transfer queue family
         
        // check if the queue supports present
        core.physicalDevice.getSurfaceSupportKHR(i, swapchain.surface, &presentFound);
        if (queueFamilyProperties[i].queueCount >= 0 && presentFound)
            { // found a present queue family
            presentCreateInfo.queueFamilyIndex = i;
            queues.presentIndex                = i;
            } // found a present queue family
        
        if (graphicsFound && computeFound && transferFound && presentFound)
            break;
         
        } // for each queue family
    
    // report which queues are missing from the physical device
    if (DEBUG_MODE)
        {
        if (queues.graphicsIndex == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find graphics queue");
        if (queues.computeIndex  == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find compute queue");
        if (queues.transferIndex == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find transfer queue");
        if (queues.presentIndex  == UINT32_MAX) ErrorHandler::nonfatal ("Failed to find present queue");
        }
        
    // by default we'll always want the graphics queue to be
    // created. However, the other queues should only be created
    // if a queue with that family index has not already been created
    std::vector<vk::DeviceQueueCreateInfo> queueCreationInfos = { graphicsCreateInfo };
    //if (queues.computeIndex  != queues.graphicsIndex) queueCreationInfos.push_back(computeCreateInfo);
    //if (queues.transferIndex != queues.graphicsIndex) queueCreationInfos.push_back(transferCreateInfo);
    //if (queues.presentIndex  != queues.graphicsIndex) queueCreationInfos.push_back(presentCreateInfo);
    
    // it's sensible to only create one queue per family based on
    // the face current vulkan implementations that may not support
    // multiple queues in a single family
    for (vk::DeviceQueueCreateInfo& info : queueCreationInfos)
        { // for each queue creation struct
        info.queueCount = 1;
        info.pQueuePriorities = std::array<float, 1>{ 0.0f }.data();
        } // for each queue creation struct
    
    // the only extension we need to worry about at present is the ability
    // to use the basic Khronos (KHR) swapchain implementation
    const std::vector<const char*> extensions =
        { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vk::DeviceCreateInfo deviceCreateInfo = { };
        deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreationInfos.size());
        deviceCreateInfo.pQueueCreateInfos       = queueCreationInfos.data();
        deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
        deviceCreateInfo.enabledLayerCount       = 0;
        deviceCreateInfo.ppEnabledLayerNames     = nullptr;
        deviceCreateInfo.pEnabledFeatures        = nullptr;

    result = candidateDevices[0].createDevice(&deviceCreateInfo, nullptr, &core.logicalDevice);

    core.logicalDevice.getQueue(queues.graphicsIndex, 0, &queues.graphics);
    core.logicalDevice.getQueue(queues.presentIndex, 0, &queues.present);

    return result;
        
    } // VulkanApp :: createDevice


//
//  createSwapChain
//
//  sets up a basic khronos swap chain, as well as images and
//  views for the various buffers (however many there should be)
//
vk::Result VulkanApp::createSwapChain ()
    { // VulkanApp :: createSwapChain
    vk::Result result = vk::Result::eSuccess;

    // query the physical device
    core.physicalDevice.getSurfaceCapabilitiesKHR (swapchain.surface, &swapchain.supported.capabilities);
    swapchain.supported.formats      = core.physicalDevice.getSurfaceFormatsKHR      (swapchain.surface);
    swapchain.supported.presentModes = core.physicalDevice.getSurfacePresentModesKHR (swapchain.surface);

    // Choose a swap surface present format
    vk::SurfaceFormatKHR format      = VulkanHelpers::querySwapChainSurfaceFormat(swapchain.supported.formats);
    vk::PresentModeKHR   presentMode = VulkanHelpers::querySwapChainPresentMode(swapchain.supported.presentModes);
    vk::Extent2D         extent      = VulkanHelpers::querySwapChainExtents(swapchain.supported.capabilities, window);

    // Set an image count
    swapchain.nImages = swapchain.supported.capabilities.minImageCount + 1;
    if (swapchain.supported.capabilities.maxImageCount > 0 && swapchain.nImages > swapchain.supported.capabilities.maxImageCount)
        swapchain.nImages = swapchain.supported.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo = { };
        createInfo.surface          = swapchain.surface;
        createInfo.minImageCount    = swapchain.nImages;
        createInfo.imageFormat      = format.format;
        createInfo.imageColorSpace  = format.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
    
    // this should be handled better now that the queue family indexing
    // code has been renovated to assemble all the family types available
    // in the API
    uint32_t queueFamilyIndices[] = { queues.graphicsIndex, queues.presentIndex };
    if (queues.graphicsIndex != queues.presentIndex)
        {
        createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
    else
        {
        createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices   = nullptr; // Optional
        }

        createInfo.preTransform   = swapchain.supported.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode    = presentMode;
        createInfo.clipped        = VK_TRUE;

    result = core.logicalDevice.createSwapchainKHR(&createInfo, nullptr, &swapchain.swapchain);
    
    core.logicalDevice.getSwapchainImagesKHR(swapchain.swapchain, &swapchain.nImages, nullptr);
    swapchain.images.resize(swapchain.nImages);
    core.logicalDevice.getSwapchainImagesKHR(swapchain.swapchain, &swapchain.nImages, swapchain.images.data());
    
    swapchain.format = format.format;
    swapchain.extent = extent;
    
    graphics.pixelFormat = swapchain.format;
    
    swapchain.views.resize(swapchain.nImages);
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        { // for each swapchain image view
        
        vk::ImageViewCreateInfo createInfo = { };
            createInfo.image = swapchain.images[i];
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.format = graphics.pixelFormat;
            createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            
        result = core.logicalDevice.createImageView(&createInfo, nullptr, &swapchain.views[i]);
        
        if (result != vk::Result::eSuccess)
            return result;
        
        } // for each swapchain image view
    
    return result;
    
    } // VulkanApp :: createSwapChain


//
//
//
vk::Result VulkanApp::createDepthBuffer ()
    { // VulkanApp :: createDepthBuffer
    vk::Result result = vk::Result::eSuccess;
    
    graphics.depthFormat = vk::Format::eD16Unorm;
    
    vk::FormatProperties depthProperties;
    core.physicalDevice.getFormatProperties (graphics.depthFormat, &depthProperties);

    vk::ImageCreateInfo createInfo = { };
    if (depthProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        createInfo.tiling = vk::ImageTiling::eLinear;
    else if (depthProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        createInfo.tiling = vk::ImageTiling::eOptimal;
    else ErrorHandler::fatal("depth format unsupported");
        createInfo.imageType             = vk::ImageType::e2D;
        createInfo.format                = graphics.depthFormat;
        createInfo.extent.width          = swapchain.extent.width;
        createInfo.extent.height         = swapchain.extent.height;
        createInfo.extent.depth          = 1;
        createInfo.mipLevels             = 1;
        createInfo.arrayLayers           = 1;
        createInfo.samples               = vk::SampleCountFlagBits::e1;
        createInfo.initialLayout         = vk::ImageLayout::eUndefined;
        createInfo.usage                 = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
        createInfo.sharingMode           = vk::SharingMode::eExclusive;
    
    vk::ImageViewCreateInfo viewCreateInfo = { };
        viewCreateInfo.format                          = graphics.depthFormat;
        viewCreateInfo.components.r                    = vk::ComponentSwizzle::eR;
        viewCreateInfo.components.g                    = vk::ComponentSwizzle::eG;
        viewCreateInfo.components.b                    = vk::ComponentSwizzle::eB;
        viewCreateInfo.components.a                    = vk::ComponentSwizzle::eA;
        viewCreateInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
        viewCreateInfo.subresourceRange.baseMipLevel   = 0;
        viewCreateInfo.subresourceRange.levelCount     = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount     = 1;
        viewCreateInfo.viewType                        = vk::ImageViewType::e2D;
        
    result = core.logicalDevice.createImage(&createInfo, nullptr, &depth.image);

    if (result != vk::Result::eSuccess)
        return result;
    
    vk::MemoryRequirements memoryRequirements = { };
        core.logicalDevice.getImageMemoryRequirements(depth.image, &memoryRequirements);
        
    vk::MemoryAllocateInfo allocationInfo = { };
        allocationInfo.allocationSize  = memoryRequirements.size;
        allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(core.physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &depth.memory);
    
    if (result != vk::Result::eSuccess)
        return result;

    core.logicalDevice.bindImageMemory(depth.image, depth.memory, 0);
    
    viewCreateInfo.image = depth.image;
    core.logicalDevice.createImageView(&viewCreateInfo, nullptr, &depth.view);
    
    return result;
    
    } // VulkanApp :: createDepthBuffer


//
//
//
vk::Result VulkanApp::createUniformBuffer ()
    { // Vulkan :: createUniformBuffer
    vk::Result result = vk::Result::eSuccess;
    
    ubo.proj = glm::perspective((float)(WINDOW_WIDTH / WINDOW_HEIGHT), 1.0f, 0.01f, 100.0f);
    ubo.proj[1][1] *= -1;

	eyePosition.y = sqrt(nObjects) * 2.0f;

    ubo.view = glm::lookAt(
        eyePosition,  // position
        glm::vec3 { 0.0f, 0.0f, 1.0f },  // center
        glm::vec3 { 0.0f, 0.0f, 1.00f }); // world up
    

	std::uniform_real_distribution<float> dist(0.0, 1.0);
	rng.seed(time(0));
	for (uint32_t i = 0; i < nObjects; ++i)
		ubo.materials[i] = { dist(rng), dist(rng), dist(rng), dist(rng) };

    ubo.lightPosition = lightPosition;
    ubo.eyePosition = eyePosition;
    
    // first we create a buffer for the object so
    // we can get it onto VRAM / device memory
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    vk::BufferCreateInfo createInfo = { };
        createInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
        createInfo.size  = bufferSize;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.sharingMode = vk::SharingMode::eExclusive;
    
    result = core.logicalDevice.createBuffer(
        &createInfo,
        nullptr,
        &buffers.uniform.buffer);
        
    if (result != vk::Result::eSuccess)
        return result;
    
    // we query the device for the requirements of
    // our buffer's memory including size and type
    vk::MemoryRequirements requirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffers.uniform.buffer, &requirements);

    // once we have a buffer and know about the memory
    // we're going to allocate we can perform the VRAM
    // allocation and crash on failure
    vk::MemoryAllocateInfo allocationInfo = { };
    allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
        core.physicalDevice,
        requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);
    allocationInfo.allocationSize = requirements.size;
        
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &buffers.uniform.memory);

    if (result != vk::Result::eSuccess)
        return result;

    // now the VRAM on the graphics card is sitting
    // allocated and empty we can copy our vertex
    // data across, once again crashing on failure
    void* data;
    result = core.logicalDevice.mapMemory(buffers.uniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &data);
   
    if (result != vk::Result::eSuccess)
        return result;
        
    memcpy(data, &ubo, (size_t)bufferSize);
    core.logicalDevice.unmapMemory(buffers.uniform.memory);
    
    core.logicalDevice.bindBufferMemory(
        buffers.uniform.buffer,
        buffers.uniform.memory,
        0);

    return result;
    
    } // Vulkan :: createUniformBuffer


//
//
//
vk::Result VulkanApp::createPipelineLayout ()
    { // VulkanApp :: createPipelineLayout
    vk::Result result = vk::Result::eSuccess;
    
    graphics.layouts.push_back(vk::DescriptorSetLayout());
    
    vk::DescriptorSetLayoutBinding layoutBinding = { };
        layoutBinding.binding             = 0;
        layoutBinding.descriptorType      = vk::DescriptorType::eUniformBuffer;
        layoutBinding.descriptorCount     = 1;
        layoutBinding.stageFlags          = vk::ShaderStageFlagBits::eVertex;
        layoutBinding.pImmutableSamplers  = nullptr;
    vk::DescriptorSetLayoutCreateInfo descriptorCreateInfo = { };
        descriptorCreateInfo.bindingCount = 1;
        descriptorCreateInfo.pBindings    = &layoutBinding;
        
    result = core.logicalDevice.createDescriptorSetLayout(
        &descriptorCreateInfo,
        nullptr,
        &graphics.layouts[0]);
        
    if (result != vk::Result::eSuccess)
        { // failed to create set layout
        return result;
        } // failed to create set layout
        
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;
        pipelineLayoutCreateInfo.setLayoutCount         = static_cast<uint32_t>(graphics.layouts.size());
        pipelineLayoutCreateInfo.pSetLayouts            = graphics.layouts.data();
        
    result = core.logicalDevice.createPipelineLayout (
        &pipelineLayoutCreateInfo,
        nullptr,
        &graphics.layout);

    if (result != vk::Result::eSuccess)
        { // failed to create pool
        return result;
        } // failed to create pool
    
    return result;
    
    } // VulkanApp :: createPipelineLayout


//
//
//
vk::Result VulkanApp::createDescriptorSet ()
    { // VulkanApp :: createDescriptorSet
    vk::Result result = vk::Result::eSuccess;
    
    // first we'll need a descriptor pool from
    // which to allocate our descriptor sets
    std::array<vk::DescriptorPoolSize, 1> poolSizes;
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = 1;
    vk::DescriptorPoolCreateInfo poolCreateInfo = { };
        poolCreateInfo.maxSets       = 1;
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
        poolCreateInfo.pPoolSizes    = poolSizes.data();
        
    result = core.logicalDevice.createDescriptorPool (
        &poolCreateInfo,
        nullptr,
        &graphics.descriptorPool);
        
    if (result != vk::Result::eSuccess)
        { // failed to create pool
        std::cout << "Failed to create descriptor pool" << std::endl;
        return result;
        } // failed to create pool
    
    // then we can start putting together our
    // actual descriptor sets and allocating
    // memory on the device for them
    vk::DescriptorSetAllocateInfo allocationInfo = { };
        allocationInfo.descriptorPool     = graphics.descriptorPool;
        allocationInfo.descriptorSetCount = static_cast<uint32_t>(graphics.layouts.size());
        allocationInfo.pSetLayouts        = graphics.layouts.data();
    result = core.logicalDevice.allocateDescriptorSets(
        &allocationInfo,
        &graphics.descriptorSet);
        
    if (result != vk::Result::eSuccess)
        { // failed to create descriptor set
        return result;
        } // failed to create descriptor set
        
    // then we prepare to write the descriptor
    // set to the device. initially this will
    // only contain the uniform buffer object
    vk::DescriptorBufferInfo bufferInfo = { };
        bufferInfo.buffer = buffers.uniform.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObject);
    
    vk::WriteDescriptorSet writes = { };
        writes.dstSet          = graphics.descriptorSet;
        writes.descriptorCount = 1;
        writes.descriptorType  = vk::DescriptorType::eUniformBuffer;
        writes.pBufferInfo     = &bufferInfo;
        writes.dstArrayElement = 0;
        writes.dstBinding      = 0;
        
    core.logicalDevice.updateDescriptorSets(1, &writes, 0, nullptr);
    
    return result;
    
    } // VulkanApp :: createDescriptorSet


//
//
//
vk::Result VulkanApp::createSemaphores ()
    { // VulkanApp :: createSemaphores
    vk::Result result = vk::Result::eSuccess;
    
    vk::SemaphoreCreateInfo semaphoreCreateInfo = { };

    result = core.logicalDevice.createSemaphore (
        &semaphoreCreateInfo,
        nullptr,
        &semaphores.imageAvailable);
        
    if (result != vk::Result::eSuccess)
        return result;
        
    result = core.logicalDevice.createSemaphore (
        &semaphoreCreateInfo,
        nullptr,
        &semaphores.renderFinished);
    
    return result;
    
    } // VulkanApp :: createSemaphores


//
//
//
vk::Result VulkanApp::createRenderPass ()
    { // VulkanApp :: createRenderPass
    vk::Result result = vk::Result::eSuccess;

    // grab a swapchain image so we can inspect it's layout
    // when creating our render pass and attachments
    //result = core.logicalDevice.acquireNextImageKHR (
    //    swapchain.swapchain,
    //    UINT64_MAX,
    //    semaphores.imageAvailable,
    //    nullptr,
    //    &swapchain.currentImage);
    
    // now we create the attachments for our renderpass
    vk::AttachmentDescription attachmentDescriptions[2];
    
        // pixel buffer attachment
        attachmentDescriptions[0].format         = graphics.pixelFormat;
        attachmentDescriptions[0].samples        = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[0].loadOp         = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[0].storeOp        = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[0].initialLayout  = vk::ImageLayout::eUndefined;
        attachmentDescriptions[0].finalLayout    = vk::ImageLayout::ePresentSrcKHR;
        
        vk::AttachmentReference pixelReference = { };
            pixelReference.attachment = 0;
            pixelReference.layout = vk::ImageLayout::eColorAttachmentOptimal;
        
        // depth buffer attachment
        attachmentDescriptions[1].format         = graphics.depthFormat;
        attachmentDescriptions[1].samples        = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[1].loadOp         = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[1].storeOp        = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[1].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[1].initialLayout  = vk::ImageLayout::eUndefined;
        attachmentDescriptions[1].finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference depthReference = { };
            depthReference.attachment = 1;
            depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        
    // we can now define the subpass dependency graph. In this
    // renderer this is trivial but will become more important
    // when we expand into more advance pipeline configurations
    vk::SubpassDependency dependencies = { };
        dependencies.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependencies.dstSubpass    = 0;
        dependencies.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies.srcAccessMask = vk::AccessFlagBits { };
        dependencies.dstAccessMask =
            vk::AccessFlagBits::eColorAttachmentRead |
            vk::AccessFlagBits::eColorAttachmentWrite;
        
    // followed closely by our subpass
    vk::SubpassDescription subpass = { };
        subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
        subpass.inputAttachmentCount    = 0;
        subpass.pInputAttachments       = nullptr;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &pixelReference;
        subpass.pResolveAttachments     = nullptr;
        subpass.pDepthStencilAttachment = &depthReference;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments    = nullptr;
        
    // then finally by the big daddy render pass
    vk::RenderPassCreateInfo createInfo = { };
        createInfo.attachmentCount = 2;
        createInfo.pAttachments    = attachmentDescriptions;
        createInfo.subpassCount    = 1;
        createInfo.pSubpasses      = &subpass;
        createInfo.dependencyCount = 1;
        createInfo.pDependencies   = &dependencies;
        
    result = core.logicalDevice.createRenderPass(&createInfo, nullptr, &graphics.renderPass);
    
    if (result != vk::Result::eSuccess)
        std::cout << "ERROR: " << vk::to_string(result);

    return result;
    
    } // VulkanApp :: createRenderPass


//
//
//
vk::Result VulkanApp::createFrameBuffers ()
    { // VulkanApp :: createFrameBuffers
    vk::Result result = vk::Result::eSuccess;
    
    // we create a framebuffer in memory for every
    // swap chain image. this will represent the
    // image in VRAM and allow us to render to it
    swapchain.framebuffers.resize(swapchain.nImages);
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        { // for each swapchain image
        std::array<vk::ImageView, 2> attachmentViews;
            attachmentViews[0] = swapchain.views[i];
            attachmentViews[1] = depth.view;
        vk::FramebufferCreateInfo framebufferCreateInfo = { };
            framebufferCreateInfo.renderPass      = graphics.renderPass;
            framebufferCreateInfo.attachmentCount = attachmentViews.size();
            framebufferCreateInfo.pAttachments    = attachmentViews.data();
            framebufferCreateInfo.width           = swapchain.extent.width;
            framebufferCreateInfo.height          = swapchain.extent.height;
            framebufferCreateInfo.layers          = 1;
        result = core.logicalDevice.createFramebuffer (
            &framebufferCreateInfo,
            nullptr,
            &swapchain.framebuffers[i]);
            
        if (result != vk::Result::eSuccess)
            return result;

        } // for each swapchain image

    return result;
    
    } // VulkanApp :: createFrameBuffers


//
//
//
vk::Result VulkanApp::createVertexBuffer ()
    { // VulkanApp :: createVertexBuffer
    vk::Result result = vk::Result::eSuccess;
    
    // first we create a buffer for the vertices so
    // we can get them onto VRAM / device memory
    vk::DeviceSize bufferSize = sizeof(Vertex) * meshes.vertices.size ();
    vk::BufferCreateInfo createInfo = { };
        createInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        createInfo.size  = bufferSize;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.sharingMode = vk::SharingMode::eExclusive;
    
    result = core.logicalDevice.createBuffer(
        &createInfo,
        nullptr,
        &buffers.vertex.buffer);
        
    if (result != vk::Result::eSuccess)
        return result;
    
    // we query the device for the requirements of
    // our buffer's memory including size and type
    vk::MemoryRequirements requirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffers.vertex.buffer, &requirements);

    // once we have a buffer and know about the memory
    // we're going to allocate we can perform the VRAM
    // allocation and crash on failure
    vk::MemoryAllocateInfo allocationInfo = { };
    allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
        core.physicalDevice,
        requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);
    allocationInfo.allocationSize = requirements.size;
        
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &buffers.vertex.memory);

    if (result != vk::Result::eSuccess)
        return result;

    // now the VRAM on the graphics card is sitting
    // allocated and empty we can copy our vertex
    // data across, once again crashing on failure
    void* data;
    result = core.logicalDevice.mapMemory(buffers.vertex.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &data);
   
    if (result != vk::Result::eSuccess)
        return result;
        
    memcpy(data, meshes.vertices.data(), (size_t)bufferSize);
    core.logicalDevice.unmapMemory(buffers.vertex.memory);
    
    core.logicalDevice.bindBufferMemory(
        buffers.vertex.buffer,
        buffers.vertex.memory,
        0);

    return result;
    
    } // VulkanApp :: createVertexBuffer


//
//
//
vk::Result VulkanApp::createIndexBuffer ()
    { // VulkanApp :: createIndexBuffer
    vk::Result result = vk::Result::eSuccess;
    
    // first we create a buffer for the vertices so
    // we can get them onto VRAM / device memory
    vk::DeviceSize bufferSize = sizeof(uint32_t) * meshes.indices.size ();
    vk::BufferCreateInfo createInfo = { };
        createInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
        createInfo.size  = bufferSize;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.sharingMode = vk::SharingMode::eExclusive;
        
    result = core.logicalDevice.createBuffer(
        &createInfo,
        nullptr,
        &buffers.index.buffer);
        
    if (result != vk::Result::eSuccess)
        return result;
        
    // we query the device for the requirements of
    // our buffer's memory including size and type
    vk::MemoryRequirements requirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffers.index.buffer, &requirements);

    // once we have a buffer and know about the memory
    // we're going to allocate we can perform the VRAM
    // allocation and crash on failure
    vk::MemoryAllocateInfo allocationInfo = { };
    allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(
        core.physicalDevice,
        requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);
    allocationInfo.allocationSize = requirements.size;
        
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &buffers.index.memory);

    if (result != vk::Result::eSuccess)
        return result;

    // now the VRAM on the graphics card is sitting
    // allocated and empty we can copy our index
    // data across, once again crashing on failure
    void* data;
    result = core.logicalDevice.mapMemory(buffers.index.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags {}, &data);
   
    if (result != vk::Result::eSuccess)
        return result;
        
    memcpy(data, meshes.indices.data(), (size_t)bufferSize);
    core.logicalDevice.unmapMemory(buffers.index.memory);
    
    core.logicalDevice.bindBufferMemory(
        buffers.index.buffer,
        buffers.index.memory,
        0);
        
    return result;
    
    } // VulkanApp :: createIndexBuffer


//
//
//
vk::Result VulkanApp::createGraphicsPipeline ()
    { // VulkanApp :: createGraphicsPipeline
    vk::Result result = vk::Result::eSuccess;
    
    vk::PipelineShaderStageCreateInfo shaderStages[] =
        {
        VulkanShaders::loadShader(core.logicalDevice, "shaders/vert.spv", vk::ShaderStageFlagBits::eVertex),
        VulkanShaders::loadShader(core.logicalDevice, "shaders/frag.spv", vk::ShaderStageFlagBits::eFragment)
        };
        
     vk::VertexInputBindingDescription inputBinding = { };
        inputBinding.binding    = 0;
        inputBinding.stride     = sizeof(Vertex);
        inputBinding.inputRate  = vk::VertexInputRate::eVertex;
        
    std::array<vk::VertexInputAttributeDescription, 5> attributes = Vertex::attributeDescriptions();
        
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = { };
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        
        vertexInputInfo.pVertexBindingDescriptions      = &inputBinding;
        vertexInputInfo.pVertexAttributeDescriptions    = attributes.data();
        
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { };
        inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = { };
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (float)swapchain.extent.width;
        viewport.height   = (float)swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = { };
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        scissor.extent.width  = swapchain.extent.width;
        scissor.extent.height = swapchain.extent.height;
        
    vk::PipelineViewportStateCreateInfo viewportCreateInfo = { };
        viewportCreateInfo.viewportCount = 1;
        viewportCreateInfo.pViewports    = &viewport;
        viewportCreateInfo.scissorCount  = 1;
        viewportCreateInfo.pScissors     = &scissor;
        
    vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo = { };
        rasterizationCreateInfo.depthClampEnable        = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode             = vk::PolygonMode::eFill;
        rasterizationCreateInfo.lineWidth               = 1.0f;
        rasterizationCreateInfo.cullMode                = vk::CullModeFlagBits::eBack;
        rasterizationCreateInfo.frontFace               = vk::FrontFace::eCounterClockwise;
        rasterizationCreateInfo.depthBiasEnable         = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationCreateInfo.depthBiasClamp          = 0.0f;
        rasterizationCreateInfo.depthBiasSlopeFactor    = 0.0f;
        
    vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo = { };
        multisampleCreateInfo.sampleShadingEnable   = VK_FALSE;
        multisampleCreateInfo.rasterizationSamples  = vk::SampleCountFlagBits::e1;
        multisampleCreateInfo.minSampleShading      = 1.0f;
        multisampleCreateInfo.pSampleMask           = nullptr;
        multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleCreateInfo.alphaToOneEnable      = VK_FALSE;
        
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = { };
        colorBlendAttachmentState.colorWriteMask       =
                  vk::ColorComponentFlagBits::eR
                | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA;
        colorBlendAttachmentState.blendEnable          = VK_TRUE;
        colorBlendAttachmentState.srcColorBlendFactor  = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachmentState.dstColorBlendFactor  = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachmentState.colorBlendOp         = vk::BlendOp::eAdd;
        colorBlendAttachmentState.srcAlphaBlendFactor  = vk::BlendFactor::eOne;
        colorBlendAttachmentState.dstAlphaBlendFactor  = vk::BlendFactor::eZero;
        colorBlendAttachmentState.alphaBlendOp         = vk::BlendOp::eAdd;
        
    vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = { };
        colorBlendCreateInfo.logicOpEnable     = VK_FALSE;
        colorBlendCreateInfo.logicOp           = vk::LogicOp::eCopy;
        colorBlendCreateInfo.attachmentCount   = 1;
        colorBlendCreateInfo.pAttachments      = &colorBlendAttachmentState;
        colorBlendCreateInfo.blendConstants[0] = 0.0f;
        colorBlendCreateInfo.blendConstants[1] = 0.0f;
        colorBlendCreateInfo.blendConstants[2] = 0.0f;
        colorBlendCreateInfo.blendConstants[3] = 0.0f;
        
    vk::DynamicState dynamicStates[] =
        {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
        };
        
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = { };
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates    = dynamicStates;
        
    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = { };
        depthStencilCreateInfo.depthTestEnable       = VK_TRUE;
        depthStencilCreateInfo.depthWriteEnable      = VK_TRUE;
        depthStencilCreateInfo.depthCompareOp        = vk::CompareOp::eLess;
        depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilCreateInfo.minDepthBounds        = 0.0f;
        depthStencilCreateInfo.maxDepthBounds        = 1.0f;
        depthStencilCreateInfo.stencilTestEnable     = VK_FALSE;
        
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
        pipelineLayoutCreateInfo.setLayoutCount         = static_cast<uint32_t>(graphics.layouts.size());
        pipelineLayoutCreateInfo.pSetLayouts            = graphics.layouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;
        
    result = core.logicalDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &graphics.layout);
    
    if (result != vk::Result::eSuccess)
        return result;
        
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo = { };
        pipelineCreateInfo.stageCount           = 2;
        pipelineCreateInfo.pStages              = shaderStages;
        pipelineCreateInfo.pVertexInputState    = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState  = &inputAssemblyInfo;
        pipelineCreateInfo.pViewportState       = &viewportCreateInfo;
        pipelineCreateInfo.pRasterizationState  = &rasterizationCreateInfo;
        pipelineCreateInfo.pMultisampleState    = &multisampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState   = &depthStencilCreateInfo;
        pipelineCreateInfo.pColorBlendState     = &colorBlendCreateInfo;
        pipelineCreateInfo.pDynamicState        = nullptr; // Optional
        pipelineCreateInfo.layout               = graphics.layout;
        pipelineCreateInfo.renderPass           = graphics.renderPass;
        pipelineCreateInfo.subpass              = 0;

    result = core.logicalDevice.createGraphicsPipelines(nullptr, 1, &pipelineCreateInfo, nullptr, &graphics.pipeline);

    if (result != vk::Result::eSuccess)
        return result;
        
    VulkanShaders::tidy(core.logicalDevice);

    return result;
    } // VulkanApp :: createGraphicsPipeline


//
//  createCommandBuffers
//
//  sets up a graphics command buffer for fulfilling
//  our rendering needs, if we require compute we'll
//  need (ideally) another command buffer
//
vk::Result VulkanApp::createCommandBuffers ()
    { // VulkanApp :: createCommandBuffers
    vk::Result result = vk::Result::eSuccess;
    
    // first we'll need to init a command pool to allocate
    // this buffer and future buffers from
    vk::CommandPoolCreateInfo poolCreateInfo = { };
        poolCreateInfo.queueFamilyIndex = queues.graphicsIndex;
        
    result = core.logicalDevice.createCommandPool(&poolCreateInfo, nullptr, &command.pool);
    
    if (result != vk::Result::eSuccess)
        return result;
        
    // now we can create our command buffers from the
    // previously created pool
    swapchain.commandBuffers.resize(swapchain.nImages);
    
    vk::CommandBufferAllocateInfo allocationInfo = { };
        allocationInfo.commandPool        = command.pool;
        allocationInfo.level              = vk::CommandBufferLevel::ePrimary;
        allocationInfo.commandBufferCount = swapchain.nImages;
  
    result = core.logicalDevice.allocateCommandBuffers(&allocationInfo, swapchain.commandBuffers.data());
  
    if (result != vk::Result::eSuccess)
        return result;
        
    // once we allocate our memory we can initialize a command
    // buffer for each swapchain image
    for (uint32_t i = 0; i < swapchain.nImages; ++i)
        { // for each swapchain image
        
        vk::CommandBufferBeginInfo beginInfo = { };
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
            beginInfo.pInheritanceInfo = nullptr;
        swapchain.commandBuffers[i].begin(&beginInfo);

        // we define a clear value for our colour buffer and our
        // stencil buffer so they can be reset at the start of render
        vk::ClearColorValue color = { WINDOW_CLEAR };
        vk::ClearDepthStencilValue depth = { 1.0f, 0 };
        
        std::array<vk::ClearValue, 2> clearValues = {};
            clearValues[0].color = color;
            clearValues[1].depthStencil = depth;

        vk::RenderPassBeginInfo renderPassBeginInfo = { };
            renderPassBeginInfo.renderPass = graphics.renderPass;
            renderPassBeginInfo.framebuffer = swapchain.framebuffers[i];
            renderPassBeginInfo.renderArea.offset = vk::Offset2D { 0, 0 };
            renderPassBeginInfo.renderArea.extent = swapchain.extent;
            renderPassBeginInfo.clearValueCount = 2;
            renderPassBeginInfo.pClearValues = clearValues.data();
        
        vk::DeviceSize offsets[] = { 0 };
        
        swapchain.commandBuffers[i].beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
        
            swapchain.commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphics.pipeline);
            swapchain.commandBuffers[i].bindVertexBuffers(0, 1, &buffers.vertex.buffer, offsets);
            swapchain.commandBuffers[i].bindIndexBuffer(buffers.index.buffer, 0, vk::IndexType::eUint32);
            swapchain.commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphics.layout, 0, 1, &graphics.descriptorSet, 0, nullptr);
            
            swapchain.commandBuffers[i].drawIndexed(static_cast<uint32_t>(meshes.indices.size()), 1, 0, 0, 0);
    
        swapchain.commandBuffers[i].endRenderPass();
        swapchain.commandBuffers[i].end();

        } // for each swapchain image
        
    return result;
        
    } // VulkanApp :: createCommandBuffers

//
//
//
void VulkanApp::arrangeObjects ()
	{ // VulkanApp :: arrangeObjects

	if (!arrangement.arranged)
		{
		int m = sqrt(nObjects);

		arrangement.translations.resize(nObjects);
		arrangement.centre = { 0.0f, 0.0f, 0.0f };

		for (Vertex& v : meshes.vertices)
			{ // for each vertex

			arrangement.translations[v.id] = {
				(v.id % m) * offset,
				0.0f, 
				(v.id / m) * offset};

			} // for each vertex

		for (uint32_t i = 0; i < nObjects; ++i)
			arrangement.centre += arrangement.translations[i];

		arrangement.centre /= nObjects;

		arrangement.arranged = true;

		}


	for (uint32_t i = 0; i < nObjects; ++i)
		{ // for each object
		
		ubo.model[i] = glm::mat4(1.0f);
		ubo.model[i] = glm::translate(ubo.model[i], arrangement.translations[i] - arrangement.centre);
		ubo.model[i] = glm::scale(ubo.model[i], glm::vec3(0.5f, 0.5f, 0.5f));
		//ubo.model[i] = glm::rotate(ubo.model[i], glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		simulation.positions[i] = arrangement.translations[i] - arrangement.centre;

		} // for each object

	} // VulkanApp :: arrangeObjects


//
//
//
void VulkanApp::createPhysicsState()
	{ // VulkanApp :: createPhysicsState
	
	std::uniform_real_distribution<float> posDist(-0.01, 0.01);
	std::uniform_real_distribution<float> angDist(-10.0, 10.0);

	simulation.positions.resize(nObjects);
	simulation.velocities.resize(nObjects);
	simulation.orientations.resize(nObjects);
	simulation.rotations.resize(nObjects);

	for (uint32_t i = 0; i < nObjects; ++i)
	{ // for each object

		simulation.velocities[i] = glm::vec3(posDist(rng), posDist(rng), posDist(rng));
		simulation.rotations[i] = glm::vec3(angDist(rng), angDist(rng), angDist(rng));
		simulation.orientations[i] = glm::vec3(0.0f, 0.0f, 0.0f);
		simulation.bounds = 2.0f;

	} // for each object



	} // VulkanApp :: createPhysicsState


//
//
//
void VulkanApp::updatePhysicsState ()
	{ // VulkanApp :: updatePhysicsState

	// advance the simulation
	for (uint32_t i = 0; i < nObjects; ++i)
		{
		simulation.positions[i] += simulation.velocities[i] * (float)timing.delta * 0.05f;
		simulation.orientations[i] += simulation.rotations[i] * (float)timing.delta * 0.005f;
		}

	// collide with boundary
	for (uint32_t i = 0; i < nObjects; ++i)
		if (glm::length(simulation.positions[i]) > 12.0f)
			{ 
			//simulation.positions[i] = glm::normalize(simulation.positions[i]) * (nObjects * 0.9f);
			simulation.velocities[i] = glm::normalize(simulation.positions[i]) * -0.01f;
			}


	// collide with eachother
	for (uint32_t i = 0; i < nObjects; ++i)
		for (uint32_t j = 0; j < nObjects; ++j)
			{
			if (i == j) continue;

			float minDist = simulation.bounds;
			float actDist = glm::length(simulation.positions[i] - simulation.positions[j]);
			if (actDist < minDist)
				{ 
				simulation.velocities[i] = glm::normalize(simulation.positions[i] - simulation.positions[j]) * 0.01f;
				simulation.velocities[j] = glm::normalize(simulation.positions[j] - simulation.positions[i]) * 0.01f;
				}
			}
	} // VulkanApp :: updatePhysicsState


//
//
//
void VulkanApp::updateUniforms ()
    { // VulkanApp :: updateUniforms
    
    if (animateLights)
        {
        ubo.lightPosition.x = sin (timing.timer * 0.1f) * 4.0f;
        ubo.lightPosition.y = cos (timing.timer * 0.1f) * 4.0f;
        }

    ubo.proj = glm::perspective((float)(WINDOW_WIDTH / WINDOW_HEIGHT), 1.0f, 0.01f, 100.0f);
    ubo.proj[1][1] *= -1;
    ubo.view = glm::lookAt(
        eyePosition,                      // position
        eyePosition + glm::vec3 { 0.0f, -1.0f, 0.0f },  // center
        glm::vec3 { 0.0f, 0.0f, 1.00f }); // world up
        
    for (uint32_t i = 0; i < nObjects; ++i)
        {

		ubo.model[i] = glm::mat4(1.0f);
		ubo.model[i] = glm::translate(ubo.model[i], simulation.positions[i]);
		ubo.model[i] = glm::scale(ubo.model[i], glm::vec3(0.5f, 0.5f, 0.5f));
		ubo.model[i] = glm::rotate(ubo.model[i], glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		ubo.model[i] = glm::rotate(ubo.model[i], glm::radians(simulation.orientations[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.model[i] = glm::rotate(ubo.model[i], glm::radians(simulation.orientations[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.model[i] = glm::rotate(ubo.model[i], glm::radians(simulation.orientations[i].x), glm::vec3(1.0f, 0.0f, 0.0f));

        }

	if (regenerateMaterials)
		{
		std::uniform_real_distribution<float> colDist(0.2f, 1.0f);
		for (uint32_t i = 0; i < nObjects; ++i)
			{
			ubo.materials[i] =
				{ colDist(rng), colDist(rng), colDist(rng), colDist(rng) };
			}
		regenerateMaterials = false;
		}

    void* data;
    core.logicalDevice.mapMemory(buffers.uniform.memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags { }, &data);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    core.logicalDevice.unmapMemory(buffers.uniform.memory);
    
    } // VulkanApp :: updateUniforms

#include <windows.h>

//
//
//
void VulkanApp::report ()
	{ // VulkanApp :: report
	
	if ((timing.frame % 30) != 0) return;

	HANDLE                     hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD                      count;
	DWORD                      cellCount;
	COORD                      homeCoords = { 0, 0 };

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return;

	/* Get the number of cells in the current buffer */
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return;
	cellCount = csbi.dwSize.X *csbi.dwSize.Y;

	/* Fill the entire buffer with spaces */
	if (!FillConsoleOutputCharacter(
		hStdOut,
		(TCHAR) ' ',
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Fill the entire buffer with the current colors and attributes */
	if (!FillConsoleOutputAttribute(
		hStdOut,
		csbi.wAttributes,
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Move the cursor home */
	SetConsoleCursorPosition(hStdOut, homeCoords);

	uint32_t meshMemoryOccupation    = ((meshes.vertices.size() * sizeof(Vertex)) / 1000) / 1000;
	meshMemoryOccupation += ((meshes.indices.size() * sizeof(uint32_t)) / 1000) / 1000;

	uint32_t depthBufferMemorySize   = ((WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint32_t) / 1000) / 1000);
	uint32_t frameBufferMemorySize   = ((WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(glm::vec3) / 1000) / 1000);
	uint32_t textureMemoryOccupation = (depthBufferMemorySize + frameBufferMemorySize) * swapchain.nImages;

	std::cout << std::endl;
	std::cout << "  average fps    : " << timing.fps << std::endl;
	std::cout << "  mesh memory    : " << meshMemoryOccupation << "mb" << std::endl;
	std::cout << "  texture memory : " << textureMemoryOccupation << "mb" << std::endl;
	std::cout << "  object count   : " << nObjects << std::endl;

	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << meshes.vertices.size();

	std::locale::global(std::locale(""));
	std::cout << "  vertex count   : " << ss.str() << std::endl;
	std::cout << std::endl;

	} // VulkanApp :: report


//
//
//
void VulkanApp::render ()
    { // VulkanApp :: render
    
    vk::Result result = vk::Result::eSuccess;
    
    // before we begin rendering we'll want to know
    // which framebuffer in the swapchain we're going
    // to be writing to. We can query the device for
    // the index of this framebuffer
    uint32_t framebufferIndex = UINT32_MAX;
    result = core.logicalDevice.acquireNextImageKHR(
                swapchain.swapchain,
                UINT64_MAX,
                semaphores.imageAvailable,
                nullptr,
                &framebufferIndex);
        
    if (result != vk::Result::eSuccess)
        std::cout << std::endl << "framebuffer index query: " << vk::to_string(result) << std::endl;
    
    // now we can start setting up our submission data
    // to hand to the API for this frame's render
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signalSemaphores[]    = { semaphores.renderFinished };
    vk::Semaphore waitSemaphores[]      = { semaphores.imageAvailable };
    
    vk::SubmitInfo submitInfo = { };
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;
        
        submitInfo.pWaitDstStageMask  = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &swapchain.commandBuffers[framebufferIndex];
    
    result = queues.graphics.submit(1, &submitInfo, nullptr);
    
    if (result != vk::Result::eSuccess)
        std::cout << std::endl << "queue submission: " << vk::to_string(result) << std::endl;

    // after submitting our queue we can present the
    // render on the screen using the KHR functions
    vk::SwapchainKHR swapchains[] = { swapchain.swapchain };
    
    vk::PresentInfoKHR presentInfo = { };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = swapchains;
        presentInfo.pImageIndices      = &framebufferIndex;
        presentInfo.pResults           = nullptr;
        
    queues.present.presentKHR(&presentInfo);
    queues.present.waitIdle();
    
    } // VulkanApp :: render


//
//
//
void VulkanApp::loop ()
    { // VulkanApp :: loop

    while (!glfwWindowShouldClose(window))
        { // while the window is open
        glfwPollEvents();
		//report();
       
        if (forwards)  eyePosition.y -= parameters.movementSpeed;
        if (backwards) eyePosition.y += parameters.movementSpeed;
        if (up)        eyePosition.z += parameters.movementSpeed;
        if (down)      eyePosition.z -= parameters.movementSpeed;
        if (left)      eyePosition.x += parameters.movementSpeed;
        if (right)     eyePosition.x -= parameters.movementSpeed;
    
		timing.update ();

		if (animateLights) timing.advance ();
    
		if (reset == 2)
			{
			arrangeObjects();
			createPhysicsState();

			timing.timer = 0.0f;
			reset = 0;
			}

		if (reset == 1)
			updatePhysicsState ();
        
		updateUniforms ();
        render ();

		if (timing.shouldClose)
			glfwSetWindowShouldClose(window, 1);
    
        } // while the window is open
    
    } // VulkanApp :: loop


//
//
//
void VulkanApp::createBuffer (
        vk::DeviceSize          size,
        vk::BufferUsageFlags    usage,
        vk::MemoryPropertyFlags properties,
        vk::Buffer&             buffer,
        vk::DeviceMemory        memory)
    { // VulkanApp :: createBuffer
    
    vk::Result result = vk::Result::eSuccess;
    
    vk::BufferCreateInfo createInfo = { };
        createInfo.size        = size;
        createInfo.usage       = usage;
        createInfo.sharingMode = vk::SharingMode::eExclusive;
    
    result = core.logicalDevice.createBuffer(&createInfo, nullptr, &buffer);
    
    if (result != vk::Result::eSuccess)
        ErrorHandler::fatal("Failed to create buffer");
        
    vk::MemoryRequirements requirements = { };
    core.logicalDevice.getBufferMemoryRequirements(buffer, &requirements);
    
    vk::MemoryAllocateInfo allocationInfo = { };
        allocationInfo.allocationSize  = requirements.size;
        allocationInfo.memoryTypeIndex = VulkanHelpers::findMemoryType (core.physicalDevice, requirements.memoryTypeBits, properties);
        
    // allocate buffer memory
    result = core.logicalDevice.allocateMemory(&allocationInfo, nullptr, &memory);
    if (result != vk::Result::eSuccess)
        ErrorHandler::fatal("Failed to allocate buffer memory");
        
    // bind buffer memory
    core.logicalDevice.bindBufferMemory(buffer, memory, 0);

    } // VulkanApp :: createBuffer
