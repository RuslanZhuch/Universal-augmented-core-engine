#pragma warning(disable: 5050)
#include "gtest/gtest.h"

#include <string_view>
#include <array>
#include <thread>

import UACEUnifiedBlockAllocator;
import UACEMemPool;

import UACERingBuffer;

using namespace UACE::MemManager::Literals;

TEST(buffers, UACERingBuffer)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(128_b);
	umem::Domain* domain{ pool.createDomain(128_b) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 128_b) };
	EXPECT_TRUE(ubAlloc.getIsValid());
	if (!ubAlloc.getIsValid())
	{
		return;
	}

	UACE::RingBuffer<120_b, decltype(ubAlloc)> rbuffer(&ubAlloc);

	std::array<char, 16 * 5> aTestData{};
	memset(aTestData.data(), 1, 16);
	memset(aTestData.data() + 16, 2, 16);
	memset(aTestData.data() + 32, 3, 16);
	memset(aTestData.data() + 48, 4, 16);
	memset(aTestData.data() + 64, 5, 16);


	std::array<char, 100_b> aOut{};

	for (int numOfIters = 0; numOfIters < 2; numOfIters++)
	{

		const auto appended0Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended0Ptr, nullptr);
		memcpy(appended0Ptr, aTestData.data(), 16);

		auto ptr{ aOut.data() };
		const auto copiedSize{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data(), 16), 0);

		const auto copiedSizeNone{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSizeNone, 0);
		EXPECT_EQ(strncmp(ptr, aTestData.data(), 16), 0);

	}

	for (int numOfIters = 0; numOfIters < 2; numOfIters++)
	{

		const auto appended0Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended0Ptr, nullptr);
		memcpy(appended0Ptr, aTestData.data(), 16);

		const auto appended1Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended1Ptr, nullptr);
		memcpy(appended1Ptr, aTestData.data() + 16, 16);

		const auto appended2Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended2Ptr, nullptr);
		memcpy(appended2Ptr, aTestData.data() + 32, 16);

		const auto appended3Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended3Ptr, nullptr);
		memcpy(appended3Ptr, aTestData.data() + 48, 16);

		const auto appended4Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended4Ptr, nullptr);
		memcpy(appended4Ptr, aTestData.data() + 64, 16);

		const auto appended5Ptr{ rbuffer.append(16) };
		EXPECT_EQ(appended5Ptr, nullptr);

		auto ptr{ aOut.data() };
		const auto copiedSize0{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize0, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data(), 16), 0);

		const auto copiedSize1{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize1, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 16, 16), 0);

		const auto copiedSize2{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize2, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 32, 16), 0);

		const auto copiedSize3{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize3, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 48, 16), 0);

		const auto copiedSize4{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize4, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 64, 16), 0);

		const auto copiedSize5{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize5, 0);

	}


	for (int numOfIters = 0; numOfIters < 2; numOfIters++)
	{

		auto ptr{ aOut.data() };

		// Frist half write
		const auto appended0Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended0Ptr, nullptr);
		memcpy(appended0Ptr, aTestData.data(), 16);

		const auto appended1Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended1Ptr, nullptr);
		memcpy(appended1Ptr, aTestData.data() + 16, 16);

		const auto appended2Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended2Ptr, nullptr);
		memcpy(appended2Ptr, aTestData.data() + 32, 16);

		// First half read
		const auto copiedSize0{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize0, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data(), 16), 0);

		const auto copiedSize1{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize1, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 16, 16), 0);

		// Second part write
		const auto appended3Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended3Ptr, nullptr);
		memcpy(appended3Ptr, aTestData.data() + 48, 16);

		const auto appended4Ptr{ rbuffer.append(16) };
		EXPECT_NE(appended4Ptr, nullptr);
		memcpy(appended4Ptr, aTestData.data() + 64, 16);

		// Second path read
		const auto copiedSize2{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize2, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 32, 16), 0);

		const auto copiedSize3{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize3, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 48, 16), 0);

		const auto copiedSize4{ rbuffer.copyAndPop(ptr, aOut.size()) };
		EXPECT_EQ(copiedSize4, 16);
		EXPECT_EQ(strncmp(ptr, aTestData.data() + 64, 16), 0);

	}

}