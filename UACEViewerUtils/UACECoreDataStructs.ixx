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

};