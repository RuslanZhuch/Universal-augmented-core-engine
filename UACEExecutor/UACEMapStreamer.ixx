module;
#include <variant>
#include <array>
#include <span>

export module UACEMapStreamer;

export import UACECoreDataStructs;

import UACEAllocator;
import UACEQueue;

export namespace UACE::Map
{

	namespace StreamerPkg
	{

		struct Empty
		{

		};
		struct ObjectCreated
		{
			size_t objId{};
		};
		struct ObjectDeleted
		{
			size_t objId{};
		};
		struct Transform
		{
			size_t objId{};
			std::array<char, 64> matBuffer;
		};
		struct MeshData
		{
			size_t objId{};
			std::span<const char> blob;
		};
		struct CameraData
		{
			size_t objId{};
			UACE::Structs::CameraData camera{};
		};

	};

	using streamerPkg_t = std::variant<
		StreamerPkg::Empty,
		StreamerPkg::ObjectCreated,
		StreamerPkg::ObjectDeleted,
		StreamerPkg::Transform,
		StreamerPkg::MeshData,
		StreamerPkg::CameraData
	>;

	template <UACE::MemManager::Allocator Alloc>
	class Streamer
	{

	public:
		Streamer() = delete;
		Streamer(Alloc* alloc)
			:
			alloc(alloc),
			queue(alloc)
		{}

		[[nodiscard]] constexpr auto sendCreateObject(StreamerPkg::ObjectCreated pkg)
		{
			return this->queue.push(pkg);
		}

		[[nodiscard]] constexpr auto sendDeleteObject(StreamerPkg::ObjectDeleted pkg)
		{
			return this->queue.push(pkg);
		}

		[[nodiscard]] constexpr auto sendTransformData(StreamerPkg::Transform pkg)
		{
			return this->queue.push(pkg);
		}

		[[nodiscard]] constexpr auto sendMeshData(StreamerPkg::MeshData pkg)
		{
			return this->queue.push(pkg);
		}

		[[nodiscard]] constexpr auto sendCameraData(StreamerPkg::CameraData pkg)
		{
			return this->queue.push(pkg);
		}

		[[nodiscard]] constexpr auto getStreamPkg()
		{
			streamerPkg_t outPkg;
			const auto bSuccess{ this->queue.pop(&outPkg) };
			if (!bSuccess)
				outPkg = StreamerPkg::Empty();
			return outPkg;
		}

	private:
		Alloc* alloc{ nullptr };
		UACE::UTILS::Queue<streamerPkg_t, 10, Alloc> queue;

	};

}