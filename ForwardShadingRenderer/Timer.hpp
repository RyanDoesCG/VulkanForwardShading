#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <thread>

#include <iostream>
#include <fstream>

struct Timer
{

	uint32_t id;
	
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point stop;

	double minDelta = 0;
	double delta = 0;

	double timer = 0;
	double step = 0.01;

	uint32_t frame = 0;

	uint32_t fps = 0;

	std::vector<double> deltas;
	bool shouldClose = false;

	Timer(uint32_t maxFps) :
		minDelta((1.0f / (float)maxFps) * 1000),
		start(std::chrono::high_resolution_clock::now())
	{ // Timer :: Timer

	} // Timer :: Timer

	void advance()
	{ // Timer :: advance

		timer += step * delta;

	} // Timer :: advanceC:\Users\Ryan\Desktop\PreferredShadingRenderer\PreferredShadingRenderer\shaders\lighting.frag

	void update()
	{ // Timer :: update

	  /*

	  stop  = std::chrono::high_resolution_clock::now();
	  delta = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

	  if (delta < minDelta)
	  {

	  double nextFrame = minDelta - delta;
	  std::chrono::milliseconds waitTime ((int)nextFrame);
	  std::this_thread::sleep_for (waitTime);

	  }

	  */

		stop = std::chrono::high_resolution_clock::now();
		delta = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
		frame = frame + 1;
		fps = (uint32_t)(1000.0f / delta);
		start = std::chrono::high_resolution_clock::now();

		deltas.push_back(delta);

		if (frame > 10000)
			{ // kill program

			double avg = 0.0;
			for (double d : deltas)
				avg += d;
			avg /= (double)deltas.size();

			std::ofstream file("log_" + std::to_string(id) + ".txt", std::ios_base::app);
			file << (uint32_t)(1000.0f / avg) << "\n";

			shouldClose = true;
			} // kill program

	} // Timer :: update

};

#endif