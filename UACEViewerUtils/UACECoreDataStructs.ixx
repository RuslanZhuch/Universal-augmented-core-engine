module;
#include <array>
#include <type_traits>

export module UACECoreDataStructs;

export namespace UACE::Structs
{

	template<typename Mat>
	concept Mat4x4 = requires(Mat && mat)
	{
		requires (std::is_same_v<decltype(mat.m), std::array<std::array<float, 4>, 4>>) ||
			(std::is_same_v<decltype(mat.m), float[4][4]>);
	};

	struct CameraData
	{

		float posX{ 0.f };
		float posY{ 0.f };
		float posZ{ 0.f };

		float rotAngle{ 0.f };
		float rotX{ 0.f };
		float rotY{ 0.f };
		float rotZ{ 0.f };

		char type{ 0 };

		float orthographicScale{ 1.f };

		float fov{ 36.9f };

		float aspectRatio{ 1.f };

		float clipStart{ 0.01f };
		float clipEnd{ 100.f };

	};

};