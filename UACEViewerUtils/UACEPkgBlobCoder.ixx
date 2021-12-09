module;

#include <optional>
#include <array>

export module UACEPkgBlobCoder;

export namespace UACE::PkgBlobCoder
{

	template<typename _Mat>
	concept _Mat4x4 = requires(_Mat && mat)
	{
			requires (std::is_same_v<decltype(mat.m), std::array<std::array<float, 4>, 4>>) || 
						(std::is_same_v<decltype(mat.m), float[4][4]>);
	};

	template<_Mat4x4 _M>
	const std::optional<_M> decodeMat4x4(char* blobData, size_t blobSize)
	{
		
		if ((blobData == nullptr) || (blobSize != 64))
		{
			return std::nullopt;
		}

		_M out{};
		std::memcpy(reinterpret_cast<char*>(&out), blobData, blobSize);

		return out;

	}

};