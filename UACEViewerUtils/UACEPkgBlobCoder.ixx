module;

#include <optional>
#include <array>
#include <bit>
#include <span>

export module UACEPkgBlobCoder;
import UACECoreDataStructs;

export namespace UACE::PkgBlobCoder
{

	template<UACE::Structs::Mat4x4 M>
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