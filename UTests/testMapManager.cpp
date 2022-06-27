#pragma warning(disable: 5050)
#include "gtest/gtest.h"
#include <vector>
#include <span>
#include <variant>
#include <algorithm>
#include "cqueue.h"

#include <thread>
#include <atomic>
#include <barrier>

//import UACEDirectoryHeader;
//import UACEStaticMeshHeader;

import UACEMemPool;
//import UACEUnifiedBlockAllocator;

//import UACEMeshDataCache;
import UACEMapDistributor;

import UACEMapStreamer;

import Structures;

import hfog.Core;
import hfog.Alloc;

using namespace hfog::MemoryUtils::Literals;

//using namespace UACE::MemManager::Literals;

/*TEST(map, directoryHeader)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	constexpr mem_t MEM_BYTES{ 128_B };
	umem::Pool pool(MEM_BYTES);
	umem::Domain* domain{ pool.createDomain(MEM_BYTES) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, MEM_BYTES) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	UACE::Map::DirectoryHeader directory(&ubAlloc, 4);
	EXPECT_TRUE(directory.getReady());
	EXPECT_EQ(directory.getMaxElements(), 4);

	EXPECT_EQ(directory.find(0), UACE::Map::DirectoryHeaderUtils::NotFound);

	constexpr uint32_t keyId0{ 0 };
	constexpr uint32_t point0{ 4 };
	EXPECT_TRUE(directory.put(keyId0, point0));
	EXPECT_EQ(directory.find(keyId0), point0);

	constexpr uint32_t keyId1{ 1 };
	constexpr uint32_t point1{ 2 };
	EXPECT_TRUE(directory.put(keyId1, point1));
	EXPECT_EQ(directory.find(keyId1), point1);

	constexpr uint32_t keyId2{ 2 };
	constexpr uint32_t point2{ 32 };
	EXPECT_TRUE(directory.put(keyId2, point2));

	constexpr uint32_t keyId3{ 3 };
	constexpr uint32_t point3{ 34 };
	EXPECT_TRUE(directory.put(keyId3, point3));

	constexpr uint32_t keyId4{ 4 };
	constexpr uint32_t point4{ 543 };
	EXPECT_FALSE(directory.put(keyId4, point4));

	EXPECT_EQ(directory.find(keyId0), point0);
	EXPECT_EQ(directory.find(keyId1), point1);
	EXPECT_EQ(directory.find(keyId2), point2);
	EXPECT_EQ(directory.find(keyId3), point3);

	constexpr uint32_t point01{ 555 };
	EXPECT_TRUE(directory.modify(keyId0, point01));
	EXPECT_EQ(directory.find(keyId0), point01);

	directory.clear();
	EXPECT_EQ(directory.find(keyId0), UACE::Map::DirectoryHeaderUtils::NotFound);
	EXPECT_EQ(directory.find(keyId1), UACE::Map::DirectoryHeaderUtils::NotFound);
	EXPECT_EQ(directory.find(keyId2), UACE::Map::DirectoryHeaderUtils::NotFound);
	EXPECT_EQ(directory.find(keyId3), UACE::Map::DirectoryHeaderUtils::NotFound);

	EXPECT_TRUE(directory.put(keyId0, point0));
	EXPECT_TRUE(directory.put(keyId1, point1));
	EXPECT_TRUE(directory.put(keyId2, point2));

	EXPECT_EQ(directory.find(keyId0), point0);
	EXPECT_EQ(directory.find(keyId1), point1);
	EXPECT_EQ(directory.find(keyId2), point2);

	EXPECT_TRUE(directory.remove(keyId0));
	EXPECT_TRUE(directory.remove(keyId2));
	EXPECT_TRUE(directory.remove(keyId1));

}

TEST(map, staticMeshMetadata)
{

	constexpr mem_t MEM_BYTES{ 1_kB };

	hfog::Alloc::Unified<256_B, MEM_BYTES> ubAlloc;

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, MEM_BYTES) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	UACE::Map::StaticMeshHeader smesh(&ubAlloc, 4);
	EXPECT_TRUE(smesh.getIsValid());

	const auto csmesh = [&](uint32_t objId, uint32_t address, uint32_t size) -> bool
	{
		return smesh.create(objId, address, size);
	};

	EXPECT_FALSE(csmesh(0, 1, 0));
	EXPECT_FALSE(csmesh(0, 10, 0));

	EXPECT_TRUE(csmesh(0, 1, 10));
	EXPECT_FALSE(csmesh(0, 1, 10));

	{
		constexpr auto objId{ 0 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 1);
		EXPECT_EQ(el.size, 10);
	}
	{
		constexpr auto objId{ 2 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 0);
		EXPECT_EQ(el.size, 0);
	}

	EXPECT_TRUE(csmesh(1, 11, 20));
	EXPECT_TRUE(csmesh(2, 21, 30));

	{
		constexpr auto objId{ 1 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 11);
		EXPECT_EQ(el.size, 20);
	}
	{
		constexpr auto objId{ 2 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 21);
		EXPECT_EQ(el.size, 30);
	}

	EXPECT_TRUE(smesh.remove(2));
	EXPECT_TRUE(smesh.remove(0));
	EXPECT_TRUE(smesh.remove(1));
	EXPECT_FALSE(smesh.remove(1));

	{
		constexpr auto objId{ 0 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 0);
		EXPECT_EQ(el.size, 0);
	}
	{
		constexpr auto objId{ 1 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 0);
		EXPECT_EQ(el.size, 0);
	}
	{
		constexpr auto objId{ 2 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 0);
		EXPECT_EQ(el.size, 0);
	}

	EXPECT_TRUE(csmesh(0, 1, 10));
	EXPECT_TRUE(csmesh(1, 11, 20));

	{
		constexpr auto objId{ 0 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 1);
		EXPECT_EQ(el.size, 10);
	}
	{
		constexpr auto objId{ 1 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 11);
		EXPECT_EQ(el.size, 20);
	}

	smesh.clear();

	{
		constexpr auto objId{ 0 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 0);
		EXPECT_EQ(el.size, 0);
	}
	{
		constexpr auto objId{ 1 };
		const UACE::Map::StaticMeshHeaderUtils::Element el{ smesh.get(objId) };
		EXPECT_EQ(el.address, 0);
		EXPECT_EQ(el.size, 0);
	}

}

TEST(map, tsMeshDataCache)
{

	constexpr mem_t MEM_BYTES{ 1_kB };

	hfog::Alloc::Unified<256_B, MEM_BYTES> ubAlloc;

	UACE::Map::StaticMeshCache mcache(&ubAlloc);

	EXPECT_FALSE(mcache.prepare(MEM_BYTES * 2));
	EXPECT_TRUE(mcache.prepare(MEM_BYTES));

	struct TVertex
	{
		struct Vec2
		{
			float x{};
			float y{};
		};
		struct Vec3
		{
			float x{};
			float y{};
			float z{};
		};
		Vec3 position{};
		Vec3 normal{};
		Vec2 texCoord{};
	};

	constexpr size_t NUM_OF_VERTICES_1{ 50 };
	std::vector<char> vStaticMesh(NUM_OF_VERTICES_1 * sizeof(TVertex));
	for (char val{ 0 }; auto v : vStaticMesh)
	{
		v = val;
		val++;
	}
	static_assert(NUM_OF_VERTICES_1 * sizeof(TVertex) < MEM_BYTES);

	EXPECT_TRUE(mcache.set({ vStaticMesh }));
	{
		const auto meshData{ mcache.getData() };
		EXPECT_EQ(meshData.size(), vStaticMesh.size());
		
		auto cPtr{ meshData.begin() };
		auto vPtr{ vStaticMesh.begin() };
		while (cPtr != meshData.end() &&
			vPtr != vStaticMesh.end())
		{
			EXPECT_EQ(*cPtr, *vPtr);
			cPtr++;
			vPtr++;
		}
	}

	vStaticMesh.resize(MEM_BYTES * 2);
	EXPECT_FALSE(mcache.set(vStaticMesh));
	{
		const auto meshData{ mcache.getData() };
		EXPECT_EQ(meshData.size(), 0);
		EXPECT_EQ(meshData.data(), nullptr);
	}

}*/

constinit const Mat testStreamMat({ { {1.f, 2.f, 3.f, 4.f}, {11.f, 21.f, 31.f, 41.f}, {12.f, 22.f, 33.f, 44.f}, {13.f, 23.f, 33.f, 43.f} } });

constinit const auto testStreamMesh = std::invoke([]()
{

	std::array<char, 100> mesh;
	for (char el{ 0 }; auto & m : mesh)
	{
		m = el++;
	}
	return mesh;

});

constinit const UACE::Structs::CameraData testStreamCamera{
	.posX = 10.f,
	.posY = 20.f,
	.posZ = 30.f,
	.rotAngle = 45.f,
	.rotX = 2.f,
	.rotY = 3.f,
	.rotZ = 4.f,
	.type = 1,
	.orthographicScale = 5.f,
	.fov = 32.f,
	.aspectRatio = 1.5f,
	.clipStart = 0.02f,
	.clipEnd = 200.f
};

void streamerTest(auto& streamer, 
	auto& syncPoint)
{

	syncPoint.arrive_and_wait();
	const auto vStreamPkgCameraCreated{ streamer.getStreamPkg() };
	const auto pkgCameraCreated{ std::get<UACE::Map::StreamerPkg::ObjectCreated>(vStreamPkgCameraCreated) };
	EXPECT_EQ(pkgCameraCreated.objId, 1);

	syncPoint.arrive_and_wait();
	const auto vStreamPkgCameraData{ streamer.getStreamPkg() };
	const auto pkgCameraData{ std::get<UACE::Map::StreamerPkg::CameraData>(vStreamPkgCameraData) };
	EXPECT_EQ(pkgCameraData.objId, 1);
	EXPECT_EQ(pkgCameraData.camera.posX, 10.f);
	EXPECT_EQ(pkgCameraData.camera.posY, 20.f);
	EXPECT_EQ(pkgCameraData.camera.posZ, 30.f);
	EXPECT_EQ(pkgCameraData.camera.rotAngle, 45.f);
	EXPECT_EQ(pkgCameraData.camera.rotX, 2.f);
	EXPECT_EQ(pkgCameraData.camera.rotY, 3.f);
	EXPECT_EQ(pkgCameraData.camera.rotZ, 4.f);
	EXPECT_EQ(pkgCameraData.camera.type, 1);
	EXPECT_EQ(pkgCameraData.camera.orthographicScale, 5.f);
	EXPECT_EQ(pkgCameraData.camera.fov, 32.f);
	EXPECT_EQ(pkgCameraData.camera.aspectRatio, 1.5f);
	EXPECT_EQ(pkgCameraData.camera.clipStart, 0.02f);
	EXPECT_EQ(pkgCameraData.camera.clipEnd, 200.f);

	syncPoint.arrive_and_wait();
	const auto vStreamPkgTransfromData{ streamer.getStreamPkg() };
	const auto pkgTransfromData{ std::get<UACE::Map::StreamerPkg::Transform>(vStreamPkgTransfromData) };

	EXPECT_EQ(pkgTransfromData.objId, 1);
	EXPECT_EQ(std::strncmp(pkgTransfromData.matBuffer.data(), reinterpret_cast<const char*>(testStreamMat.m.begin()->data()), sizeof(Mat)), 0);

	syncPoint.arrive_and_wait();
	const auto vStreamPkgMeshData{ streamer.getStreamPkg() };
	const auto pkgMeshData{ std::get<UACE::Map::StreamerPkg::MeshData>(vStreamPkgMeshData) };
	EXPECT_EQ(pkgMeshData.objId, 1);
	EXPECT_EQ(std::strncmp(pkgMeshData.blob.data(), testStreamMesh.data(), testStreamMesh.size()), 0);

	const auto vStreamPkgTransfromData2{ streamer.getStreamPkg() };
	const auto pkgTransfromData2{ std::get<UACE::Map::StreamerPkg::Transform>(vStreamPkgTransfromData2) };
	EXPECT_EQ(pkgTransfromData2.objId, 1);
	EXPECT_EQ(std::strncmp(pkgTransfromData2.matBuffer.data(), reinterpret_cast<const char*>(testStreamMat.m.begin()->data()), sizeof(Mat)), 0);

	syncPoint.arrive_and_wait();
	const auto vStreamPkgDelete{ streamer.getStreamPkg() };
	const auto pkgDelete{ std::get<UACE::Map::StreamerPkg::ObjectDeleted>(vStreamPkgDelete) };
	EXPECT_EQ(pkgDelete.objId, 1);

}

TEST(Map, tsMapStreamer)
{

	constexpr mem_t MEM_BYTES{ 2_kB };

	hfog::Alloc::Unified<256_B, MEM_BYTES> ubAlloc;

	UACE::Map::Streamer streamer(&ubAlloc, 4);

	std::barrier syncPoint(2);

	const auto threadBody = [&streamer, &syncPoint](/*std::stop_token stoken*/)
	{

		constexpr size_t objId{ 1 };
		UACE::Map::StreamerPkg::ObjectCreated pkgObjCreated{ objId };
		EXPECT_TRUE(streamer.sendCreateObject(pkgObjCreated));
		syncPoint.arrive_and_wait();

		UACE::Map::StreamerPkg::CameraData pkgCameraData{
			.objId = 1,
			.camera = testStreamCamera
		};
		EXPECT_TRUE(streamer.sendCameraData(pkgCameraData));
		syncPoint.arrive_and_wait();

		{
			UACE::Map::StreamerPkg::Transform pkg;
			pkg.objId = objId;
			std::memcpy(pkg.matBuffer.data(), &testStreamMat, sizeof(testStreamMat));
			EXPECT_TRUE(streamer.sendTransformData(pkg));
		}
		syncPoint.arrive_and_wait();

		{
			UACE::Map::StreamerPkg::MeshData pkg;
			pkg.objId = objId;
			pkg.blob = testStreamMesh;
			EXPECT_TRUE(streamer.sendMeshData(pkg));
		}
		{
			UACE::Map::StreamerPkg::Transform pkg;
			pkg.objId = objId;
			std::memcpy(pkg.matBuffer.data(), &testStreamMat, sizeof(testStreamMat));
			EXPECT_TRUE(streamer.sendTransformData(pkg));
		}
		syncPoint.arrive_and_wait();

		EXPECT_TRUE(streamer.sendDeleteObject({ objId }));
		syncPoint.arrive_and_wait();

	};

	std::jthread mapManagerThread(threadBody);

	streamerTest(streamer, syncPoint);

}

template<typename TDist>
class TestProvider
{
//	using Alloc_t = UACE::MemManager::UnifiedBlockAllocator::UnifiedBlockAllocator;
	using Distributor_t = TDist;

public:
	explicit constexpr TestProvider(Distributor_t* target)
	{
		this->target = target;
	}

	constexpr void triggerCreateNewObject(size_t objId)
	{
		EXPECT_TRUE(this->target->onObjectCreated(objId));
	}

	constexpr void triggerSetTransform(size_t objId, const Mat& mat)
	{
		EXPECT_TRUE(this->target->onSetObjectTransform(objId, mat));
	}

	constexpr void triggerSetMesh(size_t objId, std::span<const char> blob)
	{
		EXPECT_TRUE(this->target->onSetMesh(objId, blob));
	}

	constexpr void triggerDeleteObject(size_t objId)
	{
		EXPECT_TRUE(this->target->onDeleteObject(objId));
	}

	constexpr void triggerCameraData(size_t objId, UACE::Structs::CameraData cameraData)
	{
		EXPECT_TRUE(this->target->onSetCameraData(objId, cameraData));
	}

private:
	Distributor_t* target{ nullptr };

};

TEST(Map, tsMapDistributor)
{

	constexpr mem_t MEM_BYTES{ 2_kB };
	using alloc_t = hfog::Alloc::Unified<128_B, MEM_BYTES>;

	alloc_t ubAlloc;

	UACE::Map::Streamer streamer(&ubAlloc, 4);

	std::barrier syncPoint(2);

	const auto threadBody = [&streamer, &syncPoint, &ubAlloc](/*std::stop_token stoken*/)
	{

		UACE::Map::Distributor<alloc_t> distributor(&ubAlloc, &streamer);
		TestProvider<UACE::Map::Distributor<alloc_t>> provider(&distributor);

		size_t objId{ 1 };
		provider.triggerCreateNewObject(objId);

		syncPoint.arrive_and_wait();

		UACE::Structs::CameraData cameraData{ testStreamCamera };

		provider.triggerCameraData(objId, cameraData);

		syncPoint.arrive_and_wait();

		provider.triggerSetTransform(objId, testStreamMat);

		syncPoint.arrive_and_wait();

		provider.triggerSetMesh(objId, testStreamMesh);
		provider.triggerSetTransform(objId, testStreamMat);

		syncPoint.arrive_and_wait();

		provider.triggerDeleteObject(objId);

		syncPoint.arrive_and_wait();

	};

	std::jthread mapManagerThread(threadBody);

	streamerTest(streamer, syncPoint);

}