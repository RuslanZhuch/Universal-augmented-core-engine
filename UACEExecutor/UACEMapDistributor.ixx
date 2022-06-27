module;
#include <utility>
#include <span>

export module UACEMapDistributor;
import hfog.Core;
import UACECoreDataStructs;
import UACEMapStreamer;

export namespace UACE::Map
{

	template <hfog::CtAllocator Alloc>
	class Distributor
	{
		using Streamer = UACE::Map::Streamer<Alloc>;
	public:
		explicit constexpr Distributor(Alloc* alloc, Streamer* streamer)
			:alloc(alloc), streamer(streamer)
		{
			
		}

		constexpr auto onObjectCreated(size_t objId)
		{
			if (this->streamer == nullptr)
				return false;
			return this->streamer->sendCreateObject({ .objId = 1 });
		}

		constexpr auto onDeleteObject(size_t objId)
		{
			if (this->streamer == nullptr)
				return false;
			return this->streamer->sendDeleteObject({ .objId = objId });
		}

		constexpr auto onSetObjectTransform(size_t objId, UACE::Structs::Mat4x4 auto && mat)
		{
			if (this->streamer == nullptr)
				return false;
			UACE::Map::StreamerPkg::Transform pkg;
			pkg.objId = objId;
			std::memcpy(pkg.matBuffer.data(), &mat, sizeof(mat));
			return this->streamer->sendTransformData(pkg);
		}

		constexpr auto onSetMesh(size_t objId, std::span<const char> blob)
		{
			if (this->streamer == nullptr)
				return false;
			UACE::Map::StreamerPkg::MeshData pkg;
			pkg.objId = objId;
			pkg.blob = blob;
			return this->streamer->sendMeshData(pkg);
		}
		 
		constexpr auto onSetCameraData(size_t objId, UACE::Structs::CameraData camData)
		{
			if (this->streamer == nullptr)
				return false;
			return this->streamer->sendCameraData({ .objId = objId, .camera = camData });
		}

	private:
		Alloc* alloc{ nullptr };

		Streamer* streamer;

	};

};