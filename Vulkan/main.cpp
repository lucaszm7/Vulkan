#include "first_app.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main()
{
	// Create Window
	// Create Device
	// Create Swap Chain
	// Create Pipeline Layout
	// Create Pipeline
	// Create Command Buffers

	lve::FirstApp app{};

	try
	{
		app.run();
	} catch (const std::exception &e) 
	{
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}