//
//  VulkanVertex.hpp
//  ForwardRenderer
//
//  Created by macbook on 08/06/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#ifndef VulkanVertex_hpp
#define VulkanVertex_hpp

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <array>

struct Vertex
    {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uvs;
    int32_t   id;

    static std::array<vk::VertexInputAttributeDescription, 5> attributeDescriptions ()
        { // Vertex :: attributeDescriptions
        
        std::array<vk::VertexInputAttributeDescription, 5> attributes = {};

        // position
        attributes[0].binding  = 0;
        attributes[0].location = 0;
        attributes[0].format   = vk::Format::eR32G32B32Sfloat;
        attributes[0].offset   = offsetof(Vertex, position);

        // normal
        attributes[1].binding  = 0;
        attributes[1].location = 1;
        attributes[1].format   = vk::Format::eR32G32B32Sfloat;
        attributes[1].offset   = offsetof(Vertex, normal);

        // colour
        attributes[2].binding  = 0;
        attributes[2].location = 2;
        attributes[2].format   = vk::Format::eR32G32B32Sfloat;
        attributes[2].offset   = offsetof(Vertex, color);

        // tcs
        attributes[3].binding  = 0;
        attributes[3].location = 3;
        attributes[3].format   = vk::Format::eR32G32Sfloat;
        attributes[3].offset   = offsetof(Vertex, uvs);

        // object id
        attributes[4].binding  = 0;
        attributes[4].location = 4;
        attributes[4].format   = vk::Format::eR32Sint;
        attributes[4].offset   = offsetof(Vertex, id);

        return attributes;
        
        } // Vertex :: attributeDescriptions
    
    };

#endif /* VulkanVertex_h */
