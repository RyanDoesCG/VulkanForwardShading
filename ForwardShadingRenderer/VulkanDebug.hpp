//
//  VulkanDebug.hpp
//  ForwardRenderer
//
//  Created by macbook on 08/06/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//

#ifndef VulkanDebug_hpp
#define VulkanDebug_hpp

struct VulkanDebug
    {
    
    //
    //  debugCallback
    //
    //  recieves information whenever something bad happens
    //  within the API itself and informs us what we did wrong
    //
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback (
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT objType,
            uint64_t obj,
            size_t location,
            int32_t code,
            const char* layerPrefix,
            const char* msg,
            void* userData)
        { // ErrorHandler :: debugCallback
        
        uint32_t lineMax = 40;
        uint32_t line    = 0;
        uint32_t i       = 0;
        while (msg[i] != '\0')
            {
            std::cout << msg[i];
            if (line > lineMax && msg[i] == ' ')
                { std::cout << std::endl; line = 0; }
            ++line;
            ++i;
            }
        std::cout << std::endl << std::endl;

        return VK_FALSE;
        } // ErrorHandler :: debugCallback

    static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
        { // ErrorHandler :: CreateDebugReportCallbackEXT
        auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        if (func != nullptr)
            return func(instance, pCreateInfo, pAllocator, pCallback);
        else
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        } // ErrorHandler :: CreateDebugReportCallbackEXT

    static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
        { // ErrorHandler :: DestroyDebugReportCallbackEXT
        auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr)
            func(instance, callback, pAllocator);
        } // ErrorHandler :: DestroyDebugReportCallbackEXT
    
    };

#endif /* VulkanDebug_hpp */
