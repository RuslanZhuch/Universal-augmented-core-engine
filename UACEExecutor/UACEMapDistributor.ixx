module;
#include <utility>
#include <span>

export module UACEMapDistributor;
import UACEAllocator;
import UACECoreDataStructs;

export namespace UACE::Map
{

	template <UACE::MemManager::Allocator Alloc, typename Streamer>
	class Distributor
	{
	public:
		explicit constexpr Distributor(Alloc* alloc, Streamer* streamer)
			:alloc(alloc), streamer(streamer)
		{
			
		}

		constexpr void onObjectCreated(size_t objId)
		{
			if (this->streamer == nullptr)
				return;
			this->streamer->createStaticObject(objId);
		}

		constexpr void onDeleteObject(size_t objId)
		{
			if (this->streamer == nullptr)
				return;
			this->streamer->deleteStaticObject(objId);
		}

		constexpr void onSetObjectTransform(size_t objId, UACE::Structs::Mat4x4 auto && mat)
		{
			if (this->streamer == nullptr)
				return;
			this->streamer->sendTransformData(objId, std::forward<decltype(mat)>(mat));
		}

		constexpr void onSetMesh(size_t objId, std::span<const char> blob)
		{
			if (this->streamer == nullptr)
				return;
			this->streamer->sendMeshData(objId, blob);
		}

	private:
		Alloc* alloc{ nullptr };

		Streamer* streamer;

	};

};