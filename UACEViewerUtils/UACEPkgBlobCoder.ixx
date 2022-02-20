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

	struct CameraData
	{

		float posX{ 0.f };
		float posY{ 0.f };
		float posZ{ 0.f };

		float dirX{ 1.f };
		float dirY{ 0.f };
		float dirZ{ 0.f };

		float upX{ 0.f };
		float upY{ 0.f };
		float upZ{ 1.f };

		char type{ 0 };

		float orthographicScale{ 1.f };

		float clipStart{ 1.f };
		float clipEnd{ 10.f };

		float aspectRatio{ 1.f };

		char sensorFitType{ 0 };
		float sensorWidth{ 24.f };
		float sensorHeight{ 0.f };

	};

	[[nodiscard]] const std::optional<CameraData> decodeCamera(std::span<const char> blobData)
	{

		if ((blobData.data() == nullptr) || (blobData.size() != 62))
		{
			return std::nullopt;
		}

		CameraData camData{};
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

		point = pullData(point, camData.dirX);
		point = pullData(point, camData.dirY);
		point = pullData(point, camData.dirZ);

		point = pullData(point, camData.upX);
		point = pullData(point, camData.upY);
		point = pullData(point, camData.upZ);

		point = pullData(point, camData.type);

		point = pullData(point, camData.orthographicScale);

		point = pullData(point, camData.clipStart);
		point = pullData(point, camData.clipEnd);

		point = pullData(point, camData.aspectRatio);

		point = pullData(point, camData.sensorFitType);
		point = pullData(point, camData.sensorWidth);
		point = pullData(point, camData.sensorHeight);

		return camData;
		
	}

};