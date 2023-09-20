#include "fmt/printf.h"
#include "Ravine.h"

int main()
{
	Ravine app;

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		fmt::print(stderr, e.what());
		system("pause");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
