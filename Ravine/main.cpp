#include <iostream>
#include "Ravine.h"

int main()
{
	Ravine app;

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		system("pause");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}