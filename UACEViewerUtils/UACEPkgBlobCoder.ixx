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

	[[nodiscard]] const std::optional<UACE::Structs::CameraData> decodeCamera(std::span<const char> blobData)
	{

		if ((blobData.data() == nullptr) || (blobData.size() != 49))
		{
			return std::nullopt;
		}

		UACE::Structs::CameraData camData{};
		auto point{ blobData.data() };
		
		const auto pullData = [](auto point, auto&& data)
		{
			std::memcpy(&data, point, sizeof(data));
			point += sizeof(data);
			return point;
		};

		point = pullData(point, camData.posX);
		point = pullData(point, camData.posY);
		point = pullData(point, camData.posZ);

		point = pullData(point, camData.rotAngle);
		point = pullData(point, camData.rotX);
		point = pullData(point, camData.rotY);
		point = pullData(point, camData.rotZ);

		point = pullData(point, camData.type);

		point = pullData(point, camData.orthographicScale);

		point = pullData(point, camData.clipStart);
		point = pullData(point, camData.clipEnd);

		point = pullData(point, camData.aspectRatio);
		point = pullData(point, camData.fov);

		return camData;
		
	}

};