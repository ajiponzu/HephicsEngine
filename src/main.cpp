#include "app/SampleApp.hpp"

#include <Windows.h>

int main()
{
#ifndef _DEBUG
	::FreeConsole();
#endif

	try
	{
		hephics::Engine::Invoke(std::move(std::make_unique<SampleApp>()));
		hephics::Engine::Shutdown();
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}