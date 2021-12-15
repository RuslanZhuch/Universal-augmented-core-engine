#pragma warning(disable: 5050)
#include "gtest/gtest.h"
#include <string_view>
#include <array>
#include <thread>

#include <chrono>

#include "BasicTCPServer.h"

import UACEClient;
import UACEUnifiedBlockAllocator;
import UACEMemPool;


using namespace UACE::MemManager::Literals;


bool getEquals(const auto& temp, const auto& other)
{

	if (temp.size() > other.size())
		return false;

	for (size_t elId{ 0 }; elId < temp.size(); elId++)
	{
		if (temp[elId] != other[elId])
		{
			return false;
		}
	}

	return true;

};

TEST(tcp, UACEConnect)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(1_kb);
	umem::Domain* domain{ pool.createDomain(1_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 1_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	BasicTCPServer server(6000);

	constexpr auto pkg0 = std::invoke([]()
		{
			std::array<char, 32> data{};
			int i = 1;
			for (auto& d : data)
			{
				d = i++;
			}
			return data;
		});
	server.sendData(pkg0);

	UACE::Client<umem::UnifiedBlockAllocator::UnifiedBlockAllocator> client(&ubAlloc, "127.0.0.1", 6000);
	wait(client);

	EXPECT_GT(client.getNumOfPkgs(), 0);
	if (client.getNumOfPkgs() == 0)
	{
		return;
	}

	std::array<char, 1_kb> recPkg{};

	{
		const auto recSize{ client.popPkg(recPkg.data(), recPkg.size()) };
		EXPECT_EQ(recSize, 32);
		EXPECT_EQ(client.getNumOfPkgs(), 0);
		EXPECT_TRUE(getEquals(pkg0, recPkg));
	}

	constexpr auto pkg1 = std::invoke([]()
		{
			std::array<char, 50> data{};
			int i = 100;
			for (auto& d : data)
			{
				d = i++;
			}
			return data;
		});

	server.sendData(pkg1);
	wait(client);
	EXPECT_EQ(client.getNumOfPkgs(), 1);
	{
		const auto recSize{ client.popPkg(recPkg.data(), recPkg.size()) };
		EXPECT_EQ(recSize, pkg1.size());
		EXPECT_EQ(client.getNumOfPkgs(), 0);
		EXPECT_TRUE(getEquals(pkg1, recPkg));
		const auto recSizeEmpty{ client.popPkg(recPkg.data(), recPkg.size()) };
		EXPECT_EQ(recSizeEmpty, 0);
	}

	for (int numOfIters{0}; numOfIters < 5; numOfIters++)
	{
		server.sendData(pkg1);
		server.sendData(pkg0);
		wait(client, 2);
		EXPECT_EQ(client.getNumOfPkgs(), 2);
		{
			const auto recSize{ client.popPkg(recPkg.data(), recPkg.size()) };
			EXPECT_EQ(recSize, pkg1.size());
			EXPECT_EQ(client.getNumOfPkgs(), 1);
			EXPECT_TRUE(getEquals(pkg1, recPkg));
		}
		{
			const auto recSize{ client.popPkg(recPkg.data(), recPkg.size()) };
			EXPECT_EQ(recSize, pkg0.size());
			EXPECT_EQ(client.getNumOfPkgs(), 0);
			EXPECT_TRUE(getEquals(pkg0, recPkg));

			const auto recSizeEmpty{ client.popPkg(recPkg.data(), recPkg.size()) };
			EXPECT_EQ(recSizeEmpty, 0);
		}
	}

}