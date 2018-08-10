//
//  ErrorHandler.hpp
//  VukanApplication
//
//  Created by macbook on 12/05/2018.
//  Copyright © 2018 HPG. All rights reserved.
//

#ifndef ErrorHandler_hpp
#define ErrorHandler_hpp

#include <vulkan/vulkan.hpp>
#include <iostream>
#include <string>

namespace ErrorHandler
    {
    
    inline void fatal (std::string m)
        { // ErrorHandler :: fatal
        std::cout << m << std::endl << "exiting..." << std::endl;
        exit(1);
        } // ErrorHandler :: fatal
    
    inline void nonfatal (std::string m)
        { // ErrorHandler :: nonfatal
        std::cout << m << std::endl;
        } // ErrorHandler :: nonfatal
        
    }

#endif /* ErrorHandler_hpp */
