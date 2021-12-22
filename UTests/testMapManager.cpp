#pragma warning(disable: 5050)
#include "gtest/gtest.h"
#include <vector>
#include <span>
#include <variant>
#include <algorithm>
#include "cqueue.h"

import UACEDirectoryHeader;
import UACEStaticMeshHeader;

import UACEMemPool;
import UACEUnifiedBlockAllocator;

import UACEMeshDataCache;
import UACEMapDistributor;

import Structures;

using namespace UACE::MemManager::Literals;

TEST(map, directoryHeader)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	constexpr umem::MemSize MEM_BYTES{ 128_b };
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

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	constexpr umem::MemSize MEM_BYTES{ 512_b };
	umem::Pool pool(MEM_BYTES);
	umem::Domain* domain{ pool.createDomain(MEM_BYTES) };
	EXPECT_NE(domain, nullptr);

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

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	constexpr umem::MemSize MEM_BYTES{ 2_kb };
	umem::Pool pool(MEM_BYTES);
	umem::Domain* domain{ pool.createDomain(MEM_BYTES) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, MEM_BYTES) };
	EXPECT_TRUE(ubAlloc.getIsValid());

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

}

class TestStreamer
{
public:
	struct DataStaticObjectCreated
	{
		size_t objId{};
	};

	struct DataStaticObjectDeleted
	{
		size_t objId{};
	};

	struct DataStaticSetTransform
	{
		Mat mat4x4{};
		size_t objId{};
	};

	struct DataStaticSetMesh
	{
		size_t objId{};
		std::span<const char> blob;
	};

	using Data_t = std::variant<
		DataStaticObjectCreated,
		DataStaticObjectDeleted,
		DataStaticSetTransform,
		DataStaticSetMesh>;

public:
	constexpr void createStaticObject(size_t objId)
	{
		qTriggered.push(DataStaticObjectCreated(objId));
	}

	constexpr void deleteStaticObject(size_t objId)
	{
		qTriggered.push(DataStaticObjectDeleted(objId));
	}

	constexpr void sendTransformData(size_t objId, const Mat& mat)
	{
		qTriggered.push(DataStaticSetTransform(mat, objId));
	}

	constexpr void sendMeshData(size_t objId, std::span<const char> blob)
	{
		qTriggered.push(DataStaticSetMesh(objId, blob));
	}

public:

	cexpr::queue<Data_t> qTriggered{};

};

class TestProvider
{
	using Alloc_t = UACE::MemManager::UnifiedBlockAllocator::UnifiedBlockAllocator;
	using Distributor_t = UACE::Map::Distributor<Alloc_t, TestStreamer>;

public:
	explicit constexpr TestProvider(Distributor_t* target)
	{
		this->target = target;
	}

	constexpr void triggerCreateNewObject(size_t objId)
	{
		this->target->onObjectCreated(objId);
	}

	constexpr void triggerSetTransform(size_t objId, const Mat& mat)
	{
		this->target->onSetObjectTransform(objId, mat);
	}

	constexpr void triggerSetMesh(size_t objId, std::span<const char> blob)
	{
		this->target->onSetMesh(objId, blob);
	}

	constexpr void triggerDeleteObject(size_t objId)
	{
		this->target->onDeleteObject(objId);
	}

private:
	Distributor_t* target{ nullptr };

};


TEST(Map, tsMapDistributor)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	constexpr umem::MemSize MEM_BYTES{ 2_kb };
	umem::Pool pool(MEM_BYTES);
	umem::Domain* domain{ pool.createDomain(MEM_BYTES) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, MEM_BYTES) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	TestStreamer streamer;

	UACE::Map::Distributor distributor(&ubAlloc, &streamer);
	TestProvider provider(&distributor);

	size_t obj1Id{ 1 };
	provider.triggerCreateNewObject(obj1Id);

	EXPECT_EQ(streamer.qTriggered.size(), 1);
	{
		const auto varData{ streamer.qTriggered.front() };
		streamer.qTriggered.pop();
		const auto data{ std::get<TestStreamer::DataStaticObjectCreated>(varData) };
		EXPECT_EQ(data.objId, obj1Id);
	}

	provider.triggerDeleteObject(obj1Id);
	{
		const auto varData{ streamer.qTriggered.front() };
		streamer.qTriggered.pop();
		const auto data{ std::get<TestStreamer::DataStaticObjectDeleted>(varData) };
		EXPECT_EQ(data.objId, obj1Id);
	}

	Mat mat1({ { {1.f, 2.f, 3.f, 4.f}, {11.f, 21.f, 31.f, 41.f}, {12.f, 22.f, 33.f, 44.f}, {13.f, 23.f, 33.f, 43.f} } });
	provider.triggerSetTransform(obj1Id, mat1);
	{
		const auto varData{ streamer.qTriggered.front() };
		streamer.qTriggered.pop();
		const auto data{ std::get<TestStreamer::DataStaticSetTransform>(varData) };
		EXPECT_EQ(data.objId, obj1Id);
		EXPECT_EQ(data.mat4x4, mat1);
	}

	std::vector<char> vMesh1(100);
	for (char el{ 0 }; auto & m : vMesh1)
	{
		m = el++;
	}
	provider.triggerSetMesh(obj1Id, vMesh1);
	provider.triggerSetTransform(obj1Id, mat1);
	{
		const auto varData{ streamer.qTriggered.front() };
		streamer.qTriggered.pop();
		const auto data{ std::get<TestStreamer::DataStaticSetMesh>(varData) };
		EXPECT_EQ(data.objId, obj1Id);
		EXPECT_EQ(data.blob.size(), vMesh1.size());
		EXPECT_TRUE(std::ranges::equal(data.blob, vMesh1));
	}

}