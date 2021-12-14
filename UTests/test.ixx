
#pragma warning(disable: 5050)
#include "gtest/gtest.h"
//#include "testTCP.h"
#include "testBuffers.h"
//#include "testDeviation.h"

//#include "../UACEMemManger/UACEDomain.h"
import MemoryManagerCommon;
import UACEMemPool;
import UACEUnifiedBlockAllocator;
import UACEQueue;
import ScriptData;
import UACEBasicScriptFuncs;
import UACEArray;
import UACEScriptDecoder;
import UACEScriptLoader;
import UACEScript;
//import UACEUnifiedBlockAllocator;

#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <fstream>
#include <array>


using namespace UACE::MemManager::Literals;

template <typename arr0, typename arr1>
constexpr auto checkEqual(const arr0& a0, const arr1& a1, auto size)
{
	for (unsigned int i = 0; i < size; i++)
	{
		if (a0[i] != a1[i])
		{
			return false;
		}
	}
	return true;
}

TEST(MemManager, MemManagerLiterals)
{

	EXPECT_EQ(32, 32_b);
	EXPECT_EQ(32 * 1024, 32_kb);

}

TEST(MemManger, PoolAlloc) 
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;
	umem::Pool pool(64_kb);

	ump::PoolSize p64kb = pool.getSize();
	EXPECT_TRUE(p64kb == 64_kb);
	
	char buff[40];
	for (int i = 0; i < sizeof(buff); i++)
	{
		buff[i] = i + 1;
	}

	char* destBuffer0{ pool.getPtr() };
	memcpy_s(destBuffer0, sizeof(buff), buff, sizeof(buff));
	
	EXPECT_TRUE(checkEqual(pool.getPtr(), buff, sizeof(buff)));

}
 
TEST(MemManager, Domain)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;
	umem::Pool pool(64_kb);

	umem::Domain* domain{ pool.createDomain(32_kb) };
	EXPECT_NE(domain, nullptr);

	using udom = umem::Domain;

	{
		EXPECT_EQ(domain->getOffset(), 0);
		EXPECT_EQ(domain->getSize(), 32_kb);
	}

	umem::Domain* domain2{ pool.createDomain(16_kb) };
	EXPECT_NE(domain2, nullptr);
	{

		EXPECT_EQ(domain2->getOffset(), 32_kb);
		EXPECT_EQ(domain2->getSize(), 16_kb);

		umem::Domain* domainOutOfMem{ pool.createDomain(32_kb) };
		EXPECT_EQ(domainOutOfMem, nullptr);

	}

	const umem::MemSize reservedSize0{ domain->reserveMemory(64_b) };
	EXPECT_EQ(reservedSize0, 64_b);
	{
		EXPECT_EQ(domain->getOffset(), 0);
		EXPECT_EQ(domain->getSize(), 32_kb);
		EXPECT_EQ(domain->getFreeSpace(), (32_kb - 64_b));
	}

	{

		const umem::MemSize reservedSize1{ domain->reserveMemory(64_b) };
		EXPECT_EQ(reservedSize1, 64_b);

		EXPECT_EQ(domain->getOffset(), 0);
		EXPECT_EQ(domain->getSize(), 32_kb);
		EXPECT_EQ(domain->getFreeSpace(), (32_kb - (64_b * 2)));

	}

	{

		const umem::MemSize reservedSize1{ domain->reserveMemory(32_kb) };
		EXPECT_EQ(reservedSize1, 0);

		EXPECT_EQ(domain->getOffset(), 0);
		EXPECT_EQ(domain->getSize(), 32_kb);
		EXPECT_EQ(domain->getFreeSpace(), (32_kb - (64_b * 2)));

	}

	{
		char* domainMem{ domain->getMemoryPointer()};
		EXPECT_TRUE(domainMem != nullptr);
		char* poolBaseMem{ pool.getPtr() };
		EXPECT_EQ(domainMem, poolBaseMem);
	}


	{
		umem::Domain* domain3{ pool.createDomain(16_kb) };
		EXPECT_NE(domain3, nullptr);
		const umem::MemSize reservedSize3{ domain3->reserveMemory(64_b) };
		EXPECT_EQ(reservedSize3, 64_b);
		{
			EXPECT_EQ(domain3->getOffset(), 48_kb);
			EXPECT_EQ(domain3->getSize(), 16_kb);
			EXPECT_EQ(domain3->getFreeSpace(), (16_kb - 64_b));
		}

		{
			char* domainMem2{ domain3->getMemoryPointer() };
			EXPECT_TRUE(domainMem2 != nullptr);
			char* poolBaseMem2{ pool.getPtr() + 48_kb };
			EXPECT_EQ(domainMem2, poolBaseMem2);
		}

		domain3->reset();
		{
			EXPECT_EQ(domain3->getOffset(), 48_kb);
			EXPECT_EQ(domain3->getSize(), 16_kb);
			EXPECT_EQ(domain3->getFreeSpace(), (16_kb));
		}

		const umem::MemSize reservedSize3Res{ domain3->reserveMemory(64_b) };
		EXPECT_EQ(reservedSize3Res, 64_b);
		{
			EXPECT_EQ(domain3->getOffset(), 48_kb);
			EXPECT_EQ(domain3->getSize(), 16_kb);
			EXPECT_EQ(domain3->getFreeSpace(), (16_kb - 64_b));
		}
		
	}

	pool.clearDomains();
	{
		EXPECT_EQ(domain->getOffset(), 0);
		EXPECT_EQ(domain->getSize(), 0);
		EXPECT_EQ(domain->getFreeSpace(), 0);
	}
	
}

TEST(MemManager, UnifiedBlockLogic)
{

	char* buffer = new char[16_b];

	UACE::MemManager::UnifiedBlockAllocator::UnifiedBlockLogic ubLogic;
	ubLogic.init(buffer, 16_b);
	{
		char* pLeft{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pLeft, buffer);
		char* pCenter{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pCenter, buffer + 5_b);
		char* pRight{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pRight, buffer + 10_b);
		ubLogic.dealloc(pRight, 5_b);
		ubLogic.dealloc(pCenter, 5_b);
		ubLogic.dealloc(pLeft, 5_b);
	}
	{
		char* pLeft{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pLeft, buffer);
		char* pCenter{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pCenter, buffer + 5_b);
		char* pRight{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pRight, buffer + 10_b);
		ubLogic.dealloc(pLeft, 5_b);
		ubLogic.dealloc(pRight, 5_b);
		pLeft = ubLogic.requestMemory(5_b);
		EXPECT_EQ(pLeft, buffer);
		pRight = ubLogic.requestMemory(5_b);
		EXPECT_EQ(pRight, buffer + 10_b);
		ubLogic.dealloc(pRight, 5_b);
		ubLogic.dealloc(pCenter, 5_b);
		ubLogic.dealloc(pLeft, 5_b);
	}
	{
		char* pLeft{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pLeft, buffer);
		char* pCenter{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pCenter, buffer + 5_b);
		char* pRight{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pRight, buffer + 10_b);

		ubLogic.dealloc(pLeft, 5_b);
		ubLogic.dealloc(pCenter, 5_b);
		pLeft = ubLogic.requestMemory(5_b);
		EXPECT_EQ(pLeft, buffer);
		pCenter = ubLogic.requestMemory(5_b);
		EXPECT_EQ(pCenter, buffer + 5_b);
		ubLogic.dealloc(pRight, 5_b);
		ubLogic.dealloc(pCenter, 5_b);
		ubLogic.dealloc(pLeft, 5_b);
	}
	{
		char* pLeft{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pLeft, buffer);
		char* pCenter{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pCenter, buffer + 5_b);
		char* pRight{ ubLogic.requestMemory(5_b) };
		EXPECT_EQ(pRight, buffer + 10_b);

		ubLogic.dealloc(pCenter, 5_b);
		pCenter = ubLogic.requestMemory(5_b);
		EXPECT_EQ(pCenter, buffer + 5_b);
		ubLogic.dealloc(pRight, 5_b);
		ubLogic.dealloc(pCenter, 5_b);
		ubLogic.dealloc(pLeft, 5_b);
	}

}

TEST(MemManger, UnifiedBlockAlloc)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(64_kb);

	umem::Domain* domain{ pool.createDomain(64_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 32_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	EXPECT_TRUE(ubAlloc.getPtr() != nullptr);
	EXPECT_EQ(ubAlloc.getPtr(), pool.getPtr());

	upa::UnifiedBlockAllocator ubAlloc2{ upa::createAllocator(domain, 30_kb) };
	EXPECT_TRUE(ubAlloc2.getIsValid());
	EXPECT_EQ(ubAlloc2.getPtr(), pool.getPtr() + 32_kb);
	
	struct Obj0
	{
		int data0{ 10 };
		int data1{ 20 };
		int data2{ 30 };
		int data3{ 40 };
	};
	auto obj0{ ubAlloc.create_unique<Obj0>() };
	EXPECT_EQ(obj0.allocPtr, &ubAlloc);
	EXPECT_NE(obj0.ptr, nullptr);
	EXPECT_EQ(obj0->data0, 10);
	EXPECT_EQ(obj0->data1, 20);
	EXPECT_EQ(obj0->data2, 30);
	EXPECT_EQ(obj0->data3, 40);

	struct Obj1
	{
		Obj1(int in)
		{
			this->data = in;
		}
		int data{ 24 };
	}; 
	auto obj1{ ubAlloc.create_unique<Obj1>(34) };
	EXPECT_TRUE(obj1.ptr != nullptr);
	EXPECT_EQ(obj1->data, 34);

	obj0.release();
	EXPECT_EQ(obj0.ptr, nullptr);
	obj1.release();
	EXPECT_EQ(obj1.ptr, nullptr);

	upa::UnifiedBlockAllocator ubAlloc3{ upa::createAllocator(domain, 15_b) };
	EXPECT_TRUE(ubAlloc3.getIsValid());
	auto obj3Left{ ubAlloc3.create_unique<std::array<char, 5_b>>() };
	EXPECT_TRUE(obj3Left.ptr != nullptr);
	auto obj3Mid{ ubAlloc3.create_unique<std::array<char, 5_b>>() };
	EXPECT_TRUE(obj3Mid.ptr != nullptr);
	auto obj3Right{ ubAlloc3.create_unique<std::array<char, 5_b>>() };
	EXPECT_TRUE(obj3Right.ptr != nullptr);
	auto obj3Failed{ ubAlloc3.create_unique<std::array<char, 5_b>>() };
	EXPECT_TRUE(obj3Failed.ptr == nullptr);
	EXPECT_TRUE(obj3Failed.allocPtr == nullptr);

	obj3Mid.release();
	EXPECT_EQ(obj3Mid.ptr, nullptr);
	obj3Left.release();
	EXPECT_EQ(obj3Left.ptr, nullptr);
	obj3Right.release();
	EXPECT_EQ(obj3Right.ptr, nullptr);

	Obj1* objPtr{ ubAlloc.create<Obj1>(68) };
	EXPECT_NE(objPtr, nullptr);
	EXPECT_EQ(objPtr->data, 68);
	EXPECT_EQ(reinterpret_cast<char*>(objPtr), domain->getMemoryPointer());

	ubAlloc.free(objPtr);
	Obj1* objPtrNew{ ubAlloc.create<Obj1>(404) };
	EXPECT_NE(objPtrNew, nullptr);
	EXPECT_EQ(objPtrNew->data, 404);
	EXPECT_EQ(reinterpret_cast<char*>(objPtrNew), domain->getMemoryPointer());

	{
		auto unique{ ubAlloc.make_unique(objPtrNew) };
		EXPECT_EQ(unique->data, 404);
	}
	Obj1* objPtrNew2{ ubAlloc.create<Obj1>(808) };
	EXPECT_NE(objPtrNew2, nullptr);
	EXPECT_EQ(objPtrNew2->data, 808);
	EXPECT_EQ(reinterpret_cast<char*>(objPtrNew2), domain->getMemoryPointer());

	char* rawDataPtr{ nullptr };
	{
		auto blobData{ ubAlloc.create_raw(64_b) };
		rawDataPtr = blobData.ptr;
		EXPECT_NE(rawDataPtr, nullptr);
		EXPECT_EQ(blobData.size, 64_b);
	}

	Obj1* objPtrNew3{ ubAlloc.create<Obj1>(909) };
	EXPECT_NE(objPtrNew3, nullptr);
	EXPECT_EQ(objPtrNew3->data, 909);
	EXPECT_EQ(reinterpret_cast<char*>(objPtrNew3), rawDataPtr);

//	{
//		auto objProto{ ubAlloc.create_unique<Obj1>(102) };
//		EXPECT_EQ(reinterpret_cast<char*>(objProto.ptr), rawDataPtr + sizeof(Obj1));
//		EXPECT_EQ(objProto->data, 102);
//		auto objInstance{ objProto->deepCopy() };
//		EXPECT_EQ(reinterpret_cast<char*>(objInstance.ptr), rawDataPtr + sizeof(Obj1) * 2);
//		EXPECT_EQ(objProto->data, 102);
//		EXPECT_EQ(objInstance->data, 102);
//		objInstance->data = 104
//		EXPECT_EQ(objProto->data, 102);
//		EXPECT_EQ(objInstance->data, 104);
//		objProto.release();
//		EXPECT_EQ(objInstance->data, 104);
//	}

}

TEST(Utils, Queue)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(1_kb);
	umem::Domain* domain{ pool.createDomain(1_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 1_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	UACE::UTILS::Queue<int, 4> queue(&ubAlloc);
	EXPECT_EQ(queue.getMaxEls(), 4);

	{

		EXPECT_TRUE(queue.getIsEmpty());
		auto vOut{ queue.pop() };
		EXPECT_EQ(vOut.ptr, nullptr);
	}

	EXPECT_TRUE(queue.push(65));
	EXPECT_FALSE(queue.getIsEmpty());
	{
		auto vOut{ queue.pop() };
		EXPECT_NE(vOut.ptr, nullptr);
		EXPECT_EQ(*vOut, 65);
	}
	{
		EXPECT_TRUE(queue.getIsEmpty());
		auto vOut{ queue.pop() };
		EXPECT_EQ(vOut.ptr, nullptr);
	}

	EXPECT_TRUE(queue.push(122));
	EXPECT_TRUE(queue.push(244));
	{
		auto vOut{ queue.pop() };
		EXPECT_NE(vOut.ptr, nullptr);
		EXPECT_EQ(*vOut, 122);
	} 
	{
		auto vOut{ queue.pop() };
		EXPECT_NE(vOut.ptr, nullptr);
		EXPECT_EQ(*vOut, 244);
	}
	{
		auto vOut{ queue.pop() };
		EXPECT_EQ(vOut.ptr, nullptr);
	}
	EXPECT_TRUE(queue.push(301));
	EXPECT_TRUE(queue.push(302));
	EXPECT_TRUE(queue.push(303));
	EXPECT_TRUE(queue.push(304));
	EXPECT_FALSE(queue.push(305));
	EXPECT_FALSE(queue.getIsEmpty());
	{
		auto vOut{ queue.pop() };
		EXPECT_NE(vOut.ptr, nullptr);
		EXPECT_EQ(*vOut, 301);
	}
	{
		auto vOut{ queue.pop() };
		EXPECT_NE(vOut.ptr, nullptr);
		EXPECT_EQ(*vOut, 302);
	}
	{
		auto vOut{ queue.pop() };
		EXPECT_NE(vOut.ptr, nullptr);
		EXPECT_EQ(*vOut, 303);
	}
	{
		auto vOut{ queue.pop() };
		EXPECT_NE(vOut.ptr, nullptr);
		EXPECT_EQ(*vOut, 304);
	}
	{
		auto vOut{ queue.pop() };
		EXPECT_EQ(vOut.ptr, nullptr);
	}
	EXPECT_TRUE(queue.getIsEmpty());

}

TEST(Utils, QueueMultithread)
{

	using namespace std::chrono;


	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(1_kb);
	umem::Domain* domain{ pool.createDomain(1_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 1_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	UACE::UTILS::Queue<int, 4> queue(&ubAlloc);

	std::atomic_flag afNeedExit{};
	std::atomic_flag afConsReady{};

	std::jthread thCons([&queue, &afNeedExit, &afConsReady]()
	{

		int currVal{ 0 };
//		std::this_thread::sleep_for(milliseconds(100));

		while (!afNeedExit.test(std::memory_order_relaxed))
		{

			if (queue.getIsEmpty())
			{
				continue;
			}
			auto outValPtr{ queue.pop() };
			EXPECT_NE(outValPtr.ptr, nullptr);

			EXPECT_EQ(*outValPtr, currVal + 1);
			currVal++;

			if (currVal == 500)
			{
				afConsReady.notify_one();
				break;
			}

		}
		currVal = currVal;

	});

	for (int iters = 0; iters < 500; iters++)
	{
		while (!queue.push(iters + 1));
	}

	bool bConsComplete{ true };
	int iters{ 250 };
	while (afConsReady.test(std::memory_order_relaxed))
	{
		std::this_thread::sleep_for(milliseconds(10));
		iters--;
		if (iters == 0)
		{
			bConsComplete = false;
			break;
		}
	}

	EXPECT_TRUE(bConsComplete);

}

TEST(Containers, Array)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(1_kb);
	umem::Domain* domain{ pool.createDomain(1_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 1_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	{

		UACE::Containers::Array<int, 20, upa::UnifiedBlockAllocator> arr(&ubAlloc);
		EXPECT_EQ(arr.getCapacity(), 20);
		EXPECT_EQ(arr.getSize(), 0);
		const auto s1{ arr.append(10) };
		EXPECT_EQ(s1, 1);
		EXPECT_EQ(arr[0], 10);
		const auto s2{ arr.append(20) };
		EXPECT_EQ(s2, 2);
		EXPECT_EQ(arr[0], 10);
		EXPECT_EQ(arr[1], 20);
		arr.clear();
		EXPECT_EQ(arr.getSize(), 2);
		EXPECT_EQ(arr[0], 0);
		EXPECT_EQ(arr[1], 0);
		arr.reset();
		EXPECT_EQ(arr.getSize(), 0);

		arr.resize(5);
		EXPECT_EQ(arr.getSize(), 5);
		arr[0] = 10;
		arr[1] = 20;
		arr[2] = 30;
		arr[3] = 40;
		arr[4] = 50;
		arr[5] = 60;
		EXPECT_EQ(arr[0], 10);
		EXPECT_EQ(arr[1], 20);
		EXPECT_EQ(arr[2], 30);
		EXPECT_EQ(arr[3], 40);
		EXPECT_EQ(arr[4], 50);

		auto ptr{ arr.data() };
		EXPECT_NE(ptr, nullptr);

		int val = 120;
		memcpy(ptr + 1, &val, 4);
		EXPECT_EQ(arr[0], 10);
		EXPECT_EQ(arr[1], 120);
		EXPECT_EQ(arr[2], 30);
		EXPECT_EQ(arr[3], 40);
		EXPECT_EQ(arr[4], 50);

		auto copied{ arr.copy() };

		EXPECT_EQ(copied[0], 10);
		EXPECT_EQ(copied[1], 120);
		EXPECT_EQ(copied[2], 30);
		EXPECT_EQ(copied[3], 40);
		EXPECT_EQ(copied[4], 50);

		copied[0] = copied[0] * 10;
		copied[1] = copied[1] * 10;
		copied[2] = copied[2] * 10;
		copied[3] = copied[3] * 10;
		copied[4] = copied[4] * 10;


		EXPECT_EQ(copied[0], 100);
		EXPECT_EQ(copied[1], 1200);
		EXPECT_EQ(copied[2], 300);
		EXPECT_EQ(copied[3], 400);
		EXPECT_EQ(copied[4], 500);
		EXPECT_EQ(arr[0], 10);
		EXPECT_EQ(arr[1], 120);
		EXPECT_EQ(arr[2], 30);
		EXPECT_EQ(arr[3], 40);
		EXPECT_EQ(arr[4], 50);

	}

	{
		auto intPtr{ ubAlloc.create_unique<int>() };
		EXPECT_EQ((char*)intPtr.ptr, domain->getMemoryPointer());
	}



}

TEST(Scripts, ScriptFileDecoding)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(1_kb);
	umem::Domain* domain{ pool.createDomain(1_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 1_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	std::ifstream scrFile("testscript.lscr", std::ios::binary);

	scrFile.seekg(0, std::ios::end);
	const auto dataLen{ scrFile.tellg() };
	scrFile.seekg(0, std::ios::beg);

	std::array<char, 1_kb> fileData;
	EXPECT_GE(fileData.size(), dataLen);
	scrFile.read(fileData.data(), dataLen);

	EXPECT_GE(dataLen, 4);

	UACE::Script::Decoder<upa::UnifiedBlockAllocator> decoder(&ubAlloc);

	auto [funcData, structData] {decoder.decode(fileData.data())};


	EXPECT_EQ((*funcData.ptr)[0], (int)UACE::ScriptFuncs::mathPlus);
	EXPECT_EQ((*funcData.ptr)[4], (int)UACE::ScriptFuncs::mathNegate);
	EXPECT_EQ((*funcData.ptr)[7], (int)UACE::ScriptFuncs::getObj1);
	EXPECT_EQ((*funcData.ptr)[9], (int)UACE::ScriptFuncs::processObj);

	for (size_t i{ 0 }; i != structDataProto.size(); i++)
	{
		EXPECT_EQ((*structData.ptr)[i], structDataProto[i]);
	}
	for (size_t i{ 0 }; i != funcDataProto.size(); i++)
	{
		if (i == 0 || i == 4 || i == 7 || i == 9)
			continue;
		EXPECT_EQ((*funcData.ptr)[i], funcDataProto[i]);
	}

}

TEST(Scripts, ScriptFileLoader)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(1_kb);
	umem::Domain* domain{ pool.createDomain(1_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 1_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	auto [funcData, structData] {UACE::Script::loadFromFile("testscript.lscr", &ubAlloc)};

	EXPECT_EQ((*funcData.ptr)[0], (int)UACE::ScriptFuncs::mathPlus);
	EXPECT_EQ((*funcData.ptr)[4], (int)UACE::ScriptFuncs::mathNegate);
	EXPECT_EQ((*funcData.ptr)[7], (int)UACE::ScriptFuncs::getObj1);
	EXPECT_EQ((*funcData.ptr)[9], (int)UACE::ScriptFuncs::processObj);

	for (size_t i{ 0 }; i != structDataProto.size(); i++)
	{
		EXPECT_EQ((*structData.ptr)[i], structDataProto[i]);
	}
	for (size_t i{ 0 }; i != funcDataProto.size(); i++)
	{
		if (i == 0 || i == 4 || i == 7 || i == 9)
			continue;
		EXPECT_EQ((*funcData.ptr)[i], funcDataProto[i]);
	}

}

TEST(Scripts, ScriptPrototype)
{

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(2_kb);
	umem::Domain* domain{ pool.createDomain(2_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 2_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	UACE::Script::Prototype scrProto(&ubAlloc);
	EXPECT_FALSE(scrProto.getLoaded());
	const auto bLoaded{ scrProto.load("testscript.lscr") };
	EXPECT_TRUE(scrProto.getLoaded());

	UACE::ScriptFuncs::gObj1.data2 = 555;

	auto script{ scrProto.createInstance() };
	script->run();

	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data0, 10);
	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data1, 6);
	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data2, -6);
	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data3, 13);

}

TEST(Scripts, BasicLogicScript)
{

	//Const1->Math(+)-->End
	//Const2->^
	

	auto funcData{ std::to_array<int>({
		//----------------Function math plus
		//Function address 0
		(int)UACE::ScriptFuncs::mathPlus,
		//Num of params 1
		2,
		//Params:
		//First param 2
		4,
		//Second param 3
		2,
		//----------------Function math negate
		//Function negate 4
		(int)UACE::ScriptFuncs::mathNegate,
		//NumOfParams 5
		1,
		//Param 6
		0,
		//----------------Function get obj1
		//Function get obj1 7
		(int)UACE::ScriptFuncs::getObj1,
		//Num of params 8
		0,
		//----------------Function process object
		// Function process object 9
		(int)UACE::ScriptFuncs::processObj,
		//Num of params 10
		3,
		//First param (summed val) 11
		0,
		//Second param (negated val) 12
		0,
		//Third param (objects ptr) 13
		0
		}) };

	const auto structureData{ std::to_array<size_t>({
		//Sum function address 0
		0,
		//Num of return pointers 1
		2,
		//Num of links pointers 2 
		1,
		//First return pointer 3
		6,
		//Second return pointer 4
		11,
		//Ptr of next link 5
		6,
		//Link 1 Negate func ptr position 6
		4,
		//Num of return pointers 7
		1,
		//Num of links pointers 8
		1,
		//First return pointers 9
		12,
		//Ptr of next link 10
		11,
		//Link 1 Get obj1 func ptr position 11
		7,
		//Num of return pointers 12
		1,
		//Num of link pointers 13
		1,
		//First return pointer 14
		13,
		//Ptr of next link 15
		16,
		//Link 1 Process object func ptr position 16
		9,
		//Num of return pointers 17
		0,
		//Num of link pointers 18
		0,
		//First ret (null) 19
		0
	}) };

	std::array<int, 32> retData{};

	size_t point{ 0 };
	while (true)
	{

		const auto funcPosition{ structureData[point++] };
		const auto numOfReturnPointers{ structureData[point++] };
		const auto numOfLinkPointers{ structureData[point++] };
		const auto returnPtrsIds{ structureData.data() + point };
		point += numOfReturnPointers;

		const auto returnsPtr{ retData.data() };
		const auto numOfParams{ funcData[funcPosition + 1] };
		const auto firstParamOffset{ funcPosition + 2 };
		const auto paramsPtr{ funcData.data() + firstParamOffset };

		void (*funcPtr)(char*, char*) = (void(*)(char*, char*))funcData[funcPosition];
		funcPtr((char*)(paramsPtr), (char*)(returnsPtr));

		auto currRetPtrId{ returnPtrsIds };
		for (size_t retId{ 0 }; retId < numOfReturnPointers; retId++)
		{

			funcData[*currRetPtrId] = retData.front();
			currRetPtrId++;

		}

		if ((point == structureData.size()) || (point == structureData.size() - 1))
		{
			break;
		}

		const auto linkPos{ structureData[point] };
		point = linkPos;

	}

	//		sum for negate		left		right
	EXPECT_EQ(funcData[6], funcData[2] + funcData[3]);
	//		sum for process		left		right
	EXPECT_EQ(funcData[11], funcData[6]);
	//			negated			source
	EXPECT_EQ(funcData[12], -funcData[6]);

	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data0, 10);
	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data1, 6);
	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data2, -6);
	EXPECT_EQ(UACE::ScriptFuncs::gObj1.data3, 13);

}