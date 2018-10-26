#include <iostream>
#include "Ravine.h"

#include "ImGUITest.hpp"

int main()
{
	Ravine app;

	try {
		//app.run();
		RunImGUITest();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		system("pause");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}