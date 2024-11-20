#pragma once
#include <vector>

namespace FireEngine {

	void LoadDefaultLevel();

	class CLevel
	{
	public:
		CLevel();
		~CLevel() = default;

		void TickLevel(float dt);


	private:
		// std::vector<>
	};
}
