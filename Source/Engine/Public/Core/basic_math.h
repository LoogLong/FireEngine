#pragma once

#include <Eigen>

namespace FireEngine
{
	typedef Eigen::Vector3f    FVector;
	typedef Eigen::Quaternionf FQuat;
	typedef Eigen::Matrix4f    FMatrix;

	class FTransform
	{
		FVector translate;
		FQuat   rotation;
		FVector scaling;
	};

	namespace Math
	{
		template <typename T>
		T Upper(T a, T b)
		{
			return a + (b - 1) & ~(b - 1);
		}

		template<typename T>
		T Max(T a, T b)
		{
			return a > b ? a : b;
		}
	}
}
