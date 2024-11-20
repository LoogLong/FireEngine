#pragma once
#include <string>

namespace FireEngine
{
	void InitEngine(const std::string& resource_path);
	void TickEngine();

	void ShutDownEngine();
}
