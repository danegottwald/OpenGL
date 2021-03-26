
#include <iostream>

#include "Engine/Core/Application.h"

int main(int argc, char *argv[]) {
	std::cout << argv[0] << std::endl;
	
	std::unique_ptr<Application> app(new Application);

	app->Run();
	
	return 0;
	
}

