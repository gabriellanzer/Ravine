#include <fmt/printf.h>
#include "Ravine.h"

int main()
{
	Ravine app;

	try {
		app.Run();
	}
	catch (const std::runtime_error& e) {
		fmt::print(stderr, e.what());
		system("pause");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}