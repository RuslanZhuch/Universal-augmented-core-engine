#pragma warning(disable: 5050)
#include "gtest/gtest.h"

import UACEDirectoryHeader;
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
	EXPECT_EQ(directory.find(keyId0), std::numeric_limits<uint32_t>::max());
	EXPECT_EQ(directory.find(keyId1), std::numeric_limits<uint32_t>::max());
	EXPECT_EQ(directory.find(keyId2), std::numeric_limits<uint32_t>::max());
	EXPECT_EQ(directory.find(keyId3), std::numeric_limits<uint32_t>::max());

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

//TEST(map, staticMeshMetadata)
//{
//
//	UACE::Map::StaticMeshHeader smesh;
//	EXPECT_FALSE(smesh.getValid());
//
//	EXPECT_FALSE(smesh.create(0, 0));
//	EXPECT_FALSE(smesh.create(10, 0));
//
//	EXPECT_TRUE(smesh.create(0, 10));
//
//}
