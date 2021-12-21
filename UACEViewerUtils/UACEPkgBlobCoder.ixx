module;

#include <optional>
#include <array>
#include <bit>
#include <span>

export module UACEPkgBlobCoder;

export namespace UACE::PkgBlobCoder
{

	template<typename Mat>
	concept Mat4x4 = requires(Mat && mat)
	{
			requires (std::is_same_v<decltype(mat.m), std::array<std::array<float, 4>, 4>>) || 
						(std::is_same_v<decltype(mat.m), float[4][4]>);
	};

	template<Mat4x4 M>
	[[nodiscard]] constexpr const std::optional<M> decodeMat4x4(std::span<char> blobData)
	{
		
		if ((blobData.data() == nullptr) || (blobData.size() != 64))
		{
			return std::nullopt;
		}

		M out{};
		std::memcpy(reinterpret_cast<char*>(&out), blobData.data(), blobData.size());

		return out;

	}

};