//
//  main.cpp
//  ForwardRenderer
//
//  Created by macbook on 14/05/2018.
//  Copyright © 2018 MastersProject. All rights reserved.
//
#include "VulkanApp.hpp"
#include <iostream>
#include <fstream>

int main (int argc, const char* argv[])
    { // main
	VulkanApp* app;
	app = new VulkanApp(1080, 1080, "VulkanApp", 4, 0);
	delete app;
    return 0;
    } // main
