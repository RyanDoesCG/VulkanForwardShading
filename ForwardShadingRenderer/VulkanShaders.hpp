//
//  VulkanShaders.hpp
//  ForwardRenderer
//
//  Created by macbook on 14/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#ifndef VulkanShaders_hpp
#define VulkanShaders_hpp

#include <vulkan/vulkan.h>
#include <vector>
#include <stack>

class VulkanShaders
    {
    public:
    
    static vk::PipelineShaderStageCreateInfo loadShader (
                vk::Device &device,
                const char* path,
                vk::ShaderStageFlagBits stage);
    
    static vk::ShaderModule createModule (
                vk::Device &device,
                const std::vector<char>& code);
    
    static void tidy (vk::Device& device);
    
    private:
    static std::vector<char> readShaderSource (const char* path);
    static std::stack<vk::ShaderModule> pool;
    };


#endif /* VulkanShaders_hpp */
