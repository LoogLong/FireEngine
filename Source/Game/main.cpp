


#include <filesystem>
#include <string>
#include "EngineCore/engine.h"
#include "Level/level_manager.h"

int main(int argc, char** argv)
{
	std::filesystem::path executable_path(argv[0]);

	std::filesystem::path resource_path = executable_path.parent_path();//exe所在路径
	resource_path = resource_path.parent_path();//debug所在路径
	resource_path = resource_path.parent_path();//binary所在路径
	// resource_path = resource_path / "Resource";

	FireEngine::InitEngine(resource_path.generic_string());
	FireEngine::TickEngine();

	FireEngine::ShutDownEngine();
	return 0;
}

