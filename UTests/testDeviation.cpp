#pragma once

#pragma warning(disable: 5050)
#include "gtest/gtest.h"
#include <variant>

#include <array>
#include <span>

#include "BasicTCPServer.h"

import UACEJsonCoder;
import UACEPkgBlobCoder;
import UACEDeviationLogger;
import UACEViewerUtils;

import UACEUnifiedBlockAllocator;
import UACEMemPool;

import UACEClient;
import UACEDeviationDecoder;

using namespace UACE::MemManager::Literals;

struct Mat
{
	using mat_t = std::array<std::array<float, 4>, 4>;
	Mat() = default;
	Mat(const mat_t& m) : m(m) {}

	mat_t m{};
};

TEST(deviationCoder, JsonCoder)
{

	constexpr char jsonString[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"TestObj1\",\
		\"DeviationType\" : \"Mesh\",\
		\"DeviationId\" : 14,\
		\"ObjectType\" : \"ObjType\"}\
" };

	const auto decodedVariant{ UACE::JsonCoder::decode(jsonString) };
	EXPECT_EQ(decodedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::DEVIATION));
	const auto devPkg{ std::get_if<static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::DEVIATION)>(&decodedVariant) };

	EXPECT_STRCASEEQ(devPkg->objectName.data(), "TestObj1");
	EXPECT_STRCASEEQ(devPkg->deviationType.data(), "Mesh");
	EXPECT_EQ(devPkg->deviationId, 14);
	EXPECT_STRCASEEQ(devPkg->objectType.data(), "ObjType");

	{
		const auto failedVariant{ UACE::JsonCoder::decode("{\"sometype\": 31}") };
		EXPECT_EQ(failedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::NONE));
	}

	{
		const auto failedVariant{ UACE::JsonCoder::decode("ABRAKADABRA") };
		EXPECT_EQ(failedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::NONE));
	}

}

TEST(deviationCoder, JsonEncoder)
{

	const auto outStr{ UACE::JsonCoder::encodeLastDeviationId(15) };
	const auto decodedVariant{ UACE::JsonCoder::decode(outStr.data()) };

	EXPECT_EQ(decodedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::LAST_DEVIATION_ID));
	const auto devPkg{ std::get_if<static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::LAST_DEVIATION_ID)>(&decodedVariant) };

	EXPECT_EQ(devPkg->lastDeviationId, 15);

}

TEST(deviationCoder, DecodeTransformation)
{

	const Mat trMat({ { {1.f, 2.f, 3.f, 4.f}, {11.f, 21.f, 31.f, 41.f}, {12.f, 22.f, 33.f, 44.f}, {13.f, 23.f, 33.f, 43.f} } });

	std::array<char, sizeof(trMat)> blob0{};
	std::memcpy(blob0.data(), trMat.m.data(), sizeof(trMat));
	
	const auto oMat{ UACE::PkgBlobCoder::decodeMat4x4<Mat>(blob0.data(), blob0.size())};
	EXPECT_TRUE(oMat.has_value());

	EXPECT_EQ(std::memcmp(oMat->m.data(), trMat.m.data(), 64), 0);

}

TEST(deviationUtils, nameHash)
{

	const char str1[]{"First string data"};
	const char str2[]{"Second string data"};
	const char str3[]{"Second string daat"};
	const char str4[]{""};

	const auto h1{ UACE::ViewerUtils::hashString(str1) };
	const auto h2{ UACE::ViewerUtils::hashString(str2) };
	const auto h3{ UACE::ViewerUtils::hashString(str3) };
	const auto h4{ UACE::ViewerUtils::hashString(str4) };

	EXPECT_NE(h1, h2);
	EXPECT_NE(h1, h3);
	EXPECT_NE(h1, h4);

	EXPECT_NE(h2, h3);
	EXPECT_NE(h2, h4);

	EXPECT_NE(h3, h4);

	EXPECT_EQ(h4, 0);

}

TEST(deviationLogger, databaseClear)
{

	UACE::DeviationLogger devLogger("loggerDatabase.sqlite");
	EXPECT_TRUE(devLogger.getIsReady());

	devLogger.clear();

	const auto objectHashName{ UACE::ViewerUtils::hashString("ClearObj") };
	const auto objId{ devLogger.logNewObject(objectHashName, 0)};

	const auto bExist{ devLogger.getObjectExist(objId) };
	EXPECT_TRUE(bExist);

	devLogger.clear();

	{
		const auto bNotFound{ devLogger.getObjectExist(objId) };
		EXPECT_FALSE(bNotFound);
	}

}

void prepareDatabase()
{
	UACE::DeviationLogger devLogger("loggerDatabase.sqlite");
	devLogger.clear();
}

TEST(deviationLogger, cacheInDatabase)
{

	prepareDatabase();


	UACE::DeviationLogger devLogger("loggerDatabase.sqlite");
	EXPECT_TRUE(devLogger.getIsReady());
	if (!devLogger.getIsReady())
	{
		return;
	}

	const auto objectHashName{ UACE::ViewerUtils::hashString("ObjectName") };

	{
		const auto bNotFound{ devLogger.getObjectExistByHash(objectHashName) };
		EXPECT_FALSE(bNotFound);
	}

	const auto objId{ devLogger.logNewObject(objectHashName, 0) };
	const auto objIdByHash{ devLogger.getObjectId(objectHashName) };
	EXPECT_EQ(objIdByHash, objId);

	const auto objHashById{ devLogger.getObjectHash(objId) };
	EXPECT_EQ(objHashById, objectHashName);

	{
		const auto bExist{ devLogger.getObjectExist(objId) };
		EXPECT_TRUE(bExist);
	}
	{
		const auto bExist{ devLogger.getObjectExistByHash(objectHashName)};
		EXPECT_TRUE(bExist);
	}

	const auto objectHashName2{ UACE::ViewerUtils::hashString("ObjectName2") };
	const auto objId2{ devLogger.logNewObject(objectHashName2, 0) };
	{
		const auto bExist{ devLogger.getObjectExist(objId2) };
		EXPECT_TRUE(bExist);
	}

	devLogger.removeObjectByHash(objectHashName);
	EXPECT_FALSE(devLogger.getObjectExistByHash(objectHashName));
	EXPECT_TRUE(devLogger.getObjectExist(objId2));

	const auto newObject2HashName{ UACE::ViewerUtils::hashString("ObjectName3") };
	devLogger.rename(objId2, newObject2HashName);
	EXPECT_EQ(devLogger.getObjectId(newObject2HashName), objId2);

	devLogger.removeObject(objId2);
	EXPECT_FALSE(devLogger.getObjectExist(objId2));

	{
		const auto bNotFound{ devLogger.getObjectExistByHash(UACE::ViewerUtils::hashString("NoSuchObjectName")) };
		EXPECT_FALSE(bNotFound);
	}


}

TEST(deviationLogger, logTransform)
{

	prepareDatabase();


	UACE::DeviationLogger devLogger("loggerDatabase.sqlite");
	EXPECT_TRUE(devLogger.getIsReady());
	if (!devLogger.getIsReady())
	{
		return;
	}

	const Mat trMat({ { {1.f, 2.f, 3.f, 4.f}, {11.f, 21.f, 31.f, 41.f}, {12.f, 22.f, 32.f, 42.f}, {13.f, 23.f, 33.f, 43.f} } });
	std::array<char, sizeof(Mat)> matData{};
	std::memcpy(matData.data(), &trMat, matData.size());

	{
		const auto bLoaded{ devLogger.setObjectTransform(1, matData) };
		EXPECT_FALSE(bLoaded);
	}

	const auto objectHashName{ UACE::ViewerUtils::hashString("ObjectName") };
	const auto objId{ devLogger.logNewObject(objectHashName, 0) };

	const auto bLoaded{ devLogger.setObjectTransform(objId, matData) };
	EXPECT_TRUE(bLoaded);

	{
		std::array<char, 2> loadedTransform{};
		const auto bTransformLoaded{ devLogger.getObjectTransform(objId, loadedTransform) };
		EXPECT_FALSE(bTransformLoaded);
	}

	{
		std::array<char, sizeof(Mat)> loadedTransform{};
		const auto bTransformLoaded{ devLogger.getObjectTransform(objId, loadedTransform) };
		EXPECT_TRUE(bTransformLoaded);
		EXPECT_EQ(loadedTransform, matData);
	}

}

struct TestVec3
{

	friend auto operator<=>(const TestVec3&, const TestVec3&) = default;

	float x{ 0.f };
	float y{ 1.f };
	float z{ 2.f };

};

TEST(deviationLogger, logMesh)
{

	prepareDatabase();

	UACE::DeviationLogger devLogger("loggerDatabase.sqlite");
	EXPECT_TRUE(devLogger.getIsReady());
	if (!devLogger.getIsReady())
	{
		return;
	}

	constexpr auto aMeshData{ std::to_array({
		TestVec3(1.f, 2.f, 3.f), TestVec3(4.f, 5.f, 6.f),
		TestVec3(7.f, 8.f, 9.f), TestVec3(10.f, 11.f, 12.f),
		TestVec3(13.f, 14.f, 15.f), TestVec3(16.f, 17.f, 18.f)}) };

	std::array<char, sizeof(aMeshData)> rawData{};
	std::memcpy(rawData.data(), aMeshData.data(), rawData.size());

	{
		const auto bLoaded{ devLogger.setObjectMesh(1, rawData) };
		EXPECT_FALSE(bLoaded);
	}

	const auto objectHashName{ UACE::ViewerUtils::hashString("ObjectName") };
	const auto objId{ devLogger.logNewObject(objectHashName, 0) };

	const auto bLoaded{ devLogger.setObjectMesh(objId, rawData) };
	EXPECT_TRUE(bLoaded);

	{
		std::array<char, 2> loadedMesh{};
		const auto bMeshLoaded{ devLogger.getObjectMesh(objId, loadedMesh) };
		EXPECT_FALSE(bMeshLoaded);
	}

	{
		std::array<char, sizeof(aMeshData)> loadedMesh{};
		const auto bMeshLoaded{ devLogger.getObjectMesh(objId, loadedMesh) };
		EXPECT_TRUE(bMeshLoaded);
		EXPECT_EQ(loadedMesh, rawData);
	}

}

TEST(deviationDecoder, decodeFromClient)
{

	prepareDatabase();
	UACE::DeviationLogger logger("loggerDatabase.sqlite");

	namespace umem = UACE::MemManager;
	using ump = umem::Pool;

	umem::Pool pool(1_kb);
	umem::Domain* domain{ pool.createDomain(1_kb) };
	EXPECT_NE(domain, nullptr);

	namespace upa = umem::UnifiedBlockAllocator;
	upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 1_kb) };
	EXPECT_TRUE(ubAlloc.getIsValid());

	BasicTCPServer server(6000);

	const auto cbOnCreation = [&](size_t objId, size_t baseObjId, bool isInstance) mutable
	{

//		const auto hashFromName{ UACE::ViewerUtils::hashString("ObjCreation") };
		

		if (isInstance)
		{
			EXPECT_EQ(2, objId);
			EXPECT_EQ(baseObjId, 1);
			EXPECT_TRUE(logger.getObjectExist(baseObjId));
		}
		else
		{
			EXPECT_EQ(1, objId);
			EXPECT_EQ(baseObjId, 0);
			EXPECT_FALSE(logger.getObjectExist(baseObjId));
		}
		EXPECT_TRUE(logger.getObjectExist(objId));

	};

	const auto cbOnRename = [&](size_t objId1, size_t objId2, size_t oldName, size_t newName, bool bSwapping)
	{

		const auto hashFromName{ UACE::ViewerUtils::hashString("ObjInstance") };
		EXPECT_EQ(hashFromName, oldName);
		const auto hashFromNewName{ UACE::ViewerUtils::hashString("ObjInstanceNewName") };
		EXPECT_EQ(hashFromNewName, newName);

		EXPECT_EQ(objId1, 2);
		EXPECT_EQ(objId1, objId2);

		EXPECT_FALSE(logger.getObjectExistByHash(oldName));
		EXPECT_TRUE(logger.getObjectExistByHash(newName));


	};

	bool bComplete0{ false };
	const auto onDeletion = [&](size_t objId)
	{
//		const auto hashFromName{ UACE::ViewerUtils::hashString("ObjDeletion") };
		EXPECT_EQ(2, objId);
		EXPECT_FALSE(logger.getObjectExist(objId));
		bComplete0 = true;
	};


	const auto cbOnTransform = [&](size_t objId, char* transformData, size_t dataLen)
	{

		EXPECT_EQ(objId, 1);

		Mat m;
		EXPECT_EQ(dataLen, sizeof(m));
		if (dataLen == sizeof(m))
		{
			std::memcpy(m.m.data(), transformData, sizeof(m));
			EXPECT_EQ(m.m[0][0], 1.f);
			EXPECT_EQ(m.m[0][1], 2.f);
			EXPECT_EQ(m.m[0][2], 3.f);
			EXPECT_EQ(m.m[0][3], 4.f);	

			EXPECT_EQ(m.m[1][0], 11.f);
			EXPECT_EQ(m.m[1][1], 21.f);
			EXPECT_EQ(m.m[1][2], 31.f);
			EXPECT_EQ(m.m[1][3], 41.f);

			EXPECT_EQ(m.m[2][0], 12.f);
			EXPECT_EQ(m.m[2][1], 22.f);
			EXPECT_EQ(m.m[2][2], 32.f);
			EXPECT_EQ(m.m[2][3], 42.f);

			EXPECT_EQ(m.m[3][0], 13.f);
			EXPECT_EQ(m.m[3][1], 23.f);
			EXPECT_EQ(m.m[3][2], 33.f);
			EXPECT_EQ(m.m[3][3], 43.f);

			std::array<char, sizeof(Mat)> aLoadedTransform{};

			const auto bLoaded{ logger.getObjectTransform(objId, aLoadedTransform) };
			EXPECT_TRUE(bLoaded);
			EXPECT_EQ(std::strncmp(aLoadedTransform.data(), transformData, sizeof(Mat)), 0);

		}

	};

	bool bComplete1{ false };
	const auto cbOnMesh = [&](size_t objId, char* meshData, size_t dataLen)
	{

		EXPECT_EQ(objId, 1);

		constexpr auto needNumOfBytes{ 6 * sizeof(TestVec3) };
		EXPECT_EQ(dataLen, needNumOfBytes);

		const auto getVec = [&](char* data)
		{
			TestVec3 vec{};
			memcpy(&vec, data, sizeof(vec));
			return vec;
		};

		const auto v0{ getVec(meshData) };
		EXPECT_EQ(v0, TestVec3(1.f, 2.f, 3.f));

		const auto v1{ getVec(meshData + sizeof(TestVec3) * 1) };
		EXPECT_EQ(v1, TestVec3(4.f, 5.f, 6.f));

		const auto v2{ getVec(meshData + sizeof(TestVec3) * 2) };
		EXPECT_EQ(v2, TestVec3(7.f, 8.f, 9.f));

		const auto v3{ getVec(meshData + sizeof(TestVec3) * 3) };
		EXPECT_EQ(v3, TestVec3(10.f, 11.f, 12.f));

		const auto v4{ getVec(meshData + sizeof(TestVec3) * 4) };
		EXPECT_EQ(v4, TestVec3(13.f, 14.f, 15.f));

		const auto v5{ getVec(meshData + sizeof(TestVec3) * 5) };
		EXPECT_EQ(v5, TestVec3(16.f, 17.f, 18.f));


		std::array<char, needNumOfBytes> aLoadedMesh{};

		const auto bLoaded{ logger.getObjectMesh(objId, aLoadedMesh) };
		EXPECT_TRUE(bLoaded);
		EXPECT_EQ(std::strncmp(aLoadedMesh.data(), meshData, needNumOfBytes), 0);

		bComplete1 = true;

	};

	UACE::Deviation::Desc desc(cbOnCreation, onDeletion, cbOnRename, cbOnTransform, cbOnMesh);

	UACE::DeviationDecoder devDecoder(&ubAlloc, desc, "loggerDatabase.sqlite", "127.0.0.1", 6000);


	const auto createBuffer = []<size_t _size>(const char (&charArray)[_size])
	{
		std::array<char, _size - 1> sendData{};
		memcpy(sendData.data(), charArray, sendData.size());
		return sendData;
	};

	constexpr char jsonCreationBase[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjBase\",\
		\"DeviationType\" : \"Creation\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"Mesh\"}\
" };

	//Is not an instance 
	server.sendData(std::to_array(jsonCreationBase));
	server.sendData(createBuffer(""));

	constexpr char jsonCreationInst[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjInstance\",\
		\"DeviationType\" : \"Creation\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"Mesh\"}\
" };

	//Is an instance
	server.sendData(std::to_array(jsonCreationInst));
	server.sendData(createBuffer("ObjBase"));



	constexpr char jsonRename[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjInstance\",\
		\"DeviationType\" : \"Rename\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"Mesh\"}\
" };
	server.sendData(std::to_array(jsonRename));
	server.sendData(createBuffer("ObjInstanceNewName"));

	constexpr char jsonDeletion[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjInstanceNewName\",\
		\"DeviationType\" : \"Deletion\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"Mesh\"}\
" };

	server.sendData(std::to_array(jsonDeletion));
	server.sendData(createBuffer(""));

	while (!bComplete0) { devDecoder.tick(); }


	constexpr char jsonTransform[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjBase\",\
		\"DeviationType\" : \"Transform\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"Mesh\"}\
" };

	const Mat trMat({ { {1.f, 2.f, 3.f, 4.f}, {11.f, 21.f, 31.f, 41.f}, {12.f, 22.f, 32.f, 42.f}, {13.f, 23.f, 33.f, 43.f} } });
	std::array<char, sizeof(Mat)> matData{};
	std::memcpy(matData.data(), &trMat, matData.size());

	server.sendData(std::to_array(jsonTransform));
	server.sendData(matData);

	constexpr char jsonMesh[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjBase\",\
		\"DeviationType\" : \"Mesh\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"Mesh\"}\
" };

	server.sendData(std::to_array(jsonMesh));
	
	const auto aMeshData{ std::to_array({
		TestVec3(1.f, 2.f, 3.f), TestVec3(4.f, 5.f, 6.f), 
		TestVec3(7.f, 8.f, 9.f), TestVec3(10.f, 11.f, 12.f),
		TestVec3(13.f, 14.f, 15.f), TestVec3(16.f, 17.f, 18.f)}) };

	server.sendData(aMeshData);

	while (!bComplete1) { devDecoder.tick(); }

//	static_assert(false, "Test database part 2");

}