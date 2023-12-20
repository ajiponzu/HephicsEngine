#include "app/SampleApp.hpp"

#include <Windows.h>

int main()
{
#ifndef _DEBUG
	::FreeConsole();
#endif

	hephics::Engine::Invoke(std::move(std::make_unique<SampleApp>()));
	hephics::Engine::Shutdown();

	return 0;
}