//
//  VulkanShaders.cpp
//  ForwardRenderer
//
//  Created by macbook on 14/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//
#include "ErrorHandler.hpp"
#include "VulkanShaders.hpp"
#include <fstream>

std::stack<vk::ShaderModule> VulkanShaders::pool =
    {
    
    };

std::vector<char> VulkanShaders::readShaderSource (const char* path)
    { // VulkanShaders :: readShaderSource
    
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    
    if (!file.is_open())
        ErrorHandler::nonfatal(std::string("failed to read file from " + std::string(path)));
        
    std::size_t fileSize = (std::size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
    } // VulkanShaders :: readShaderSource

vk::PipelineShaderStageCreateInfo VulkanShaders::loadShader (vk::Device &device, const char* path, vk::ShaderStageFlagBits stage)
    { // VulkanShaders :: loadShader
    std::vector<char> source = readShaderSource (path);

    vk::PipelineShaderStageCreateInfo shaderStageInfo = {};
        shaderStageInfo.stage  = stage;
        shaderStageInfo.module = createModule (device, source);
        shaderStageInfo.pName  = "main";

    return shaderStageInfo;
    } // VulkanShaders :: loadShader

vk::ShaderModule VulkanShaders::createModule (vk::Device &device, const std::vector<char> &source)
    { // VulkanShaders :: createModule
    vk::ShaderModuleCreateInfo createInfo = {};
        createInfo.codeSize = source.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(source.data());

    vk::ShaderModule shaderModule;
    if (device.createShaderModule(&createInfo, nullptr, &shaderModule) != vk::Result::eSuccess)
        ErrorHandler::fatal("failed to create shader module");
    pool.push(shaderModule);

    return shaderModule;
    } // VulkanShaders :: createModule

void VulkanShaders::tidy(vk::Device& device)
    { // VulkanShaders :: tidy
    while (pool.size())
        { // for each shader module
        device.destroyShaderModule(pool.top(), nullptr);
        pool.pop();
        } // for each shader module
    } // VulkanShaders :: tidy
