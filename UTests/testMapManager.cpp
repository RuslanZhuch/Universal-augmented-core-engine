#pragma warning(disable: 5050)
#include "gtest/gtest.h"

import UACEDirectoryHeader;
import UACEStaticMeshHeader;

import UACEMemPool;
import UACEUnifiedBlockAllocator;

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
