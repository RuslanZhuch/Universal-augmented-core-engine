#pragma warning(disable: 5050)
#include "gtest/gtest.h"
#include <variant>

#include <array>
#include <span>
#include <bit>
#include <atomic>

#include <cassert>

import BasicTCPServer;
import UACEJsonCoder;
import UACEPkgBlobCoder;
import UACEDeviationLogger;
import UACEViewerUtils;

//import UACEUnifiedBlockAllocator;
import UACEMemPool;

import UACEClient;
import UACEDeviationDecoder;

import hfog.Core;
import hfog.Alloc;

import Structures;

using namespace hfog::MemoryUtils::Literals;

TEST(deviationCoder, JsonCoderMesh)
{

	constexpr char jsonString[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"TestObj1\",\
		\"DeviationType\" : \"Mesh\",\
		\"DeviationId\" : 14,\
		\"ObjectType\" : \"MESH\"}\
" };

	const auto decodedVariant{ UACE::JsonCoder::decode(jsonString) };
	EXPECT_EQ(decodedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::DEVIATION));
	const auto devPkg{ std::get_if<static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::DEVIATION)>(&decodedVariant) };

	EXPECT_STRCASEEQ(devPkg->objectName.data(), "TestObj1");
	EXPECT_STRCASEEQ(devPkg->deviationType.data(), "Mesh");
	EXPECT_EQ(devPkg->deviationId, 14);
	EXPECT_EQ(devPkg->objectType, UACE::JsonCoder::ObjectType::MESH);

	{
		const auto failedVariant{ UACE::JsonCoder::decode("{\"sometype\": 31}") };
		EXPECT_EQ(failedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::NONE));
	}

	{
		const auto failedVariant{ UACE::JsonCoder::decode("ABRAKADABRA") };
		EXPECT_EQ(failedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::NONE));
	}

}

TEST(deviationCoder, JsonCoderCamera)
{

	constexpr char jsonString[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"TestObj1\",\
		\"DeviationType\" : \"Camera\",\
		\"DeviationId\" : 15,\
		\"ObjectType\" : \"CAMERA\"}\
" };

	const auto decodedVariant{ UACE::JsonCoder::decode(jsonString) };
	EXPECT_EQ(decodedVariant.index(), static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::DEVIATION));
	const auto devPkg{ std::get_if<static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::DEVIATION)>(&decodedVariant) };

	EXPECT_STRCASEEQ(devPkg->objectName.data(), "TestObj1");
	EXPECT_STRCASEEQ(devPkg->deviationType.data(), "Camera");
	EXPECT_EQ(devPkg->deviationId, 15);
	EXPECT_EQ(devPkg->objectType, UACE::JsonCoder::ObjectType::CAMERA);

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
	
	const auto oMat{ UACE::PkgBlobCoder::decodeMat4x4<Mat>(blob0)};
	EXPECT_TRUE(oMat.has_value());

	EXPECT_EQ(std::memcmp(oMat->m.data(), trMat.m.data(), 64), 0);

}

const auto createCameraData()
{

	std::array<char, 49> cRaw;
	auto point{ cRaw.data() };
	const auto pushData = [&cRaw](auto point, const auto data)
	{
		assert(point + sizeof(data) <= cRaw.data() + cRaw.size());
		std::memcpy(point, &data, sizeof(data));
		point += sizeof(data);
		return point;
	};

	const float posX{ 5.f };
	const float posY{ 6.f };
	const float posZ{ -7.f };
	point = pushData(point, posX);
	point = pushData(point, posY);
	point = pushData(point, posZ);

	const float rotAngle{ 1.f };
	const float rotX{ 2.f };
	const float rotY{ 3.f };
	const float rotZ{ 4.f };
	point = pushData(point, rotAngle);
	point = pushData(point, rotX);
	point = pushData(point, rotY);
	point = pushData(point, rotZ);

	const char type{ 1 };
	point = pushData(point, type);

	const float orthographicScale{ 6.f };
	point = pushData(point, orthographicScale);

	const float clipStart{ 0.001f };
	const float clipEnd{ 100.f };
	point = pushData(point, clipStart);
	point = pushData(point, clipEnd);

	const float aspectRatio{ 1.5f };
	point = pushData(point, aspectRatio);

	const float fov{ 23.f };
	point = pushData(point, fov);
	
	assert(point == cRaw.data() + cRaw.size());

	return cRaw;

}

TEST(deviationCoder, DecodeCamera)
{

	const auto rawData{ createCameraData() };
	const auto oDecodedData{ UACE::PkgBlobCoder::decodeCamera(rawData) };

	EXPECT_TRUE(oDecodedData.has_value());
	const auto decodedData{ oDecodedData.value() };

	EXPECT_EQ(decodedData.posX, 5.f);
	EXPECT_EQ(decodedData.posY, 6.f);
	EXPECT_EQ(decodedData.posZ, -7.f);

	EXPECT_EQ(decodedData.rotAngle, 1.f);
	EXPECT_EQ(decodedData.rotX, 2.f);
	EXPECT_EQ(decodedData.rotY, 3.f);
	EXPECT_EQ(decodedData.rotZ, 4.f);

	EXPECT_EQ(decodedData.type, 1);

	EXPECT_EQ(decodedData.orthographicScale, 6.f);

	EXPECT_EQ(decodedData.clipStart, 0.001f);
	EXPECT_EQ(decodedData.clipEnd, 100.f);

	EXPECT_EQ(decodedData.aspectRatio, 1.5f);

	EXPECT_EQ(decodedData.fov, 23.f);

}

TEST(deviationUtils, nameHash)
{

	constexpr char str1[]{"First string data"};
	constexpr char str2[]{"Second string data"};
	constexpr char str3[]{"Second string daat"};
	constexpr char str4[]{""};

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
	const auto objId{ devLogger.logNewObject(objectHashName, {}) };

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

	const auto objId{ devLogger.logNewObject(objectHashName, {}) };
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
	const auto objId2{ devLogger.logNewObject(objectHashName2, {}) };
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
	const auto objId{ devLogger.logNewObject(objectHashName, {}) };

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
	const auto objId{ devLogger.logNewObject(objectHashName, {}) };

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

TEST(deviationLogger, logCameraNew)
{

	prepareDatabase();

	UACE::DeviationLogger devLogger("loggerDatabase.sqlite");
	EXPECT_TRUE(devLogger.getIsReady());
	if (!devLogger.getIsReady())
	{
		return;
	}

	const auto rawData{ createCameraData() };
	{
		const auto bLoaded{ devLogger.setCamera(1, rawData) };
		EXPECT_FALSE(bLoaded);
	}

	const auto objectHashName{ UACE::ViewerUtils::hashString("CameraName") };
	const auto objId{ devLogger.logNewObject(objectHashName, rawData) };

	{
		std::array<char, sizeof(rawData)> loadedCamera{};
		const auto bCameraLoaded{ devLogger.getCamera(objId, loadedCamera) };
		EXPECT_TRUE(bCameraLoaded);
		EXPECT_EQ(loadedCamera, rawData);
	}

}

TEST(deviationLogger, logCamera)
{

	prepareDatabase();

	UACE::DeviationLogger devLogger("loggerDatabase.sqlite");
	EXPECT_TRUE(devLogger.getIsReady());
	if (!devLogger.getIsReady())
	{
		return;
	}

	const auto rawData{ createCameraData() };
	{
		const auto bLoaded{ devLogger.setCamera(1, rawData) };
		EXPECT_FALSE(bLoaded);
	}

	const auto objectHashName{ UACE::ViewerUtils::hashString("CameraName") };
	const auto objId{ devLogger.logNewObject(objectHashName, {}) };

	const auto bLoaded{ devLogger.setCamera(objId, rawData) };
	EXPECT_TRUE(bLoaded);

	{
		std::array<char, 2> loadedCamera{};
		const auto bCameraLoaded{ devLogger.getCamera(objId, loadedCamera) };
		EXPECT_FALSE(bCameraLoaded);
	}

	{
		std::array<char, sizeof(rawData)> loadedCamera{};
		const auto bCameraLoaded{ devLogger.getCamera(objId, loadedCamera) };
		EXPECT_TRUE(bCameraLoaded);
		EXPECT_EQ(loadedCamera, rawData);
	}

}

constexpr auto genTestvertexData()
{
	using vert_t = float;
	constexpr auto vertices{ std::to_array<vert_t>({1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f}) };
	return std::bit_cast<std::array<char, vertices.size() * sizeof(vert_t)>>(vertices);
};

constexpr auto compareContainers(const auto& left, const auto& right)
{
	if (left.size() != right.size())
		return false;
	for (size_t idx{ 0 }; idx < left.size(); idx++)
	{
		if (left[idx] != right[idx])
			return false;
	}
	return true;
}

TEST(deviationDecoder, decodeFromClient)
{

	prepareDatabase();
	UACE::DeviationLogger logger("loggerDatabase.sqlite");

	hfog::Alloc::Unified<256_B, 10_kB> ubAlloc;

	BasicTCPServer server(6000);

	const auto cbOnCameraCreation = [](size_t, std::span<const char>)
	{

	};

	const auto cbOnStaticMeshCreation = [&](size_t objId, std::span<const char> meshData) mutable
	{

//		const auto hashFromName{ UACE::ViewerUtils::hashString("ObjCreation") };

		static int expectedObjId{ 1 };

		EXPECT_EQ(expectedObjId++, objId);

		constexpr auto expectedMeshData{ genTestvertexData() };
		EXPECT_TRUE(compareContainers(meshData, expectedMeshData));

//		if (isInstance)
//		{
//			EXPECT_EQ(2, objId);
//			EXPECT_EQ(baseObjId, 1);
//			EXPECT_TRUE(logger.getObjectExist(baseObjId));
//		}
//		else
//		{
//			EXPECT_EQ(1, objId);
//			EXPECT_EQ(baseObjId, 0);
//			EXPECT_FALSE(logger.getObjectExist(baseObjId));
//		}
		EXPECT_TRUE(logger.getObjectExist(objId));

	};

	const auto cbOnRename = [&](size_t objId1, size_t objId2, size_t oldName, size_t newName, bool bSwapping)
	{

		const auto hashFromName{ UACE::ViewerUtils::hashString("Obj2") };
		EXPECT_EQ(hashFromName, oldName);
		const auto hashFromNewName{ UACE::ViewerUtils::hashString("Obj2NewName") };
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


	const auto cbOnTransform = [&](size_t objId, std::span<const char> transformData)
	{

		EXPECT_EQ(objId, 1);

		Mat m;
		EXPECT_EQ(transformData.size(), sizeof(m));
		if (transformData.size() == sizeof(m))
		{
			std::memcpy(m.m.data(), transformData.data(), sizeof(m));
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
			EXPECT_EQ(std::strncmp(aLoadedTransform.data(), transformData.data(), sizeof(Mat)), 0);

		}

	};

	bool bComplete1{ false };
	const auto cbOnMesh = [&](size_t objId, std::span<const char> meshData)
	{

		EXPECT_EQ(objId, 1);

		constexpr auto expectedMeshData{ genTestvertexData() };
		EXPECT_TRUE(compareContainers(meshData, expectedMeshData));


//		constexpr auto needNumOfBytes{ 6 * sizeof(TestVec3) };
//		EXPECT_EQ(dataLen, needNumOfBytes);
//
//		const auto getVec = [&](char* data)
//		{
//			TestVec3 vec{};
//			memcpy(&vec, data, sizeof(vec));
//			return vec;
//		};
//
//		const auto v0{ getVec(meshData) };
//		EXPECT_EQ(v0, TestVec3(1.f, 2.f, 3.f));
//
//		const auto v1{ getVec(meshData + sizeof(TestVec3) * 1) };
//		EXPECT_EQ(v1, TestVec3(4.f, 5.f, 6.f));
//
//		const auto v2{ getVec(meshData + sizeof(TestVec3) * 2) };
//		EXPECT_EQ(v2, TestVec3(7.f, 8.f, 9.f));
//
//		const auto v3{ getVec(meshData + sizeof(TestVec3) * 3) };
//		EXPECT_EQ(v3, TestVec3(10.f, 11.f, 12.f));
//
//		const auto v4{ getVec(meshData + sizeof(TestVec3) * 4) };
//		EXPECT_EQ(v4, TestVec3(13.f, 14.f, 15.f));
//
//		const auto v5{ getVec(meshData + sizeof(TestVec3) * 5) };
//		EXPECT_EQ(v5, TestVec3(16.f, 17.f, 18.f));
//
//
//		std::array<char, needNumOfBytes> aLoadedMesh{};
//
//		const auto bLoaded{ logger.getObjectMesh(objId, aLoadedMesh) };
//		EXPECT_TRUE(bLoaded);
//		EXPECT_EQ(std::strncmp(aLoadedMesh.data(), meshData, needNumOfBytes), 0);

		bComplete1 = true;

	};

	const auto onCamera = [](size_t, std::span<const char>)
	{	};

	UACE::Deviation::Desc desc{ 
		cbOnCameraCreation,
		cbOnStaticMeshCreation, 
		onDeletion, 
		cbOnRename, 
		cbOnTransform, 
		cbOnMesh, 
		onCamera };

	UACE::DeviationDecoder devDecoder(&ubAlloc, desc, "loggerDatabase.sqlite", "127.0.0.1", 6000);


	const auto createBuffer = []<size_t size>(const char (&charArray)[size])
	{
		std::array<char, size - 1> sendData{};
		memcpy(sendData.data(), charArray, sendData.size());
		return sendData;
	};

	constexpr char jsonMeshCreation[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjBase\",\
		\"DeviationType\" : \"Creation\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"MESH\"}\
" };

	//Creating mesh with fake metadata 
	server.sendData(std::to_array(jsonMeshCreation));
	server.sendData(genTestvertexData());

	constexpr char jsonCreation2[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"Obj2\",\
		\"DeviationType\" : \"Creation\",\
		\"DeviationId\" : 2,\
		\"ObjectType\" : \"MESH\"}\
" };

	//Is an instance
	server.sendData(std::to_array(jsonCreation2));
	server.sendData(genTestvertexData());


	constexpr char jsonRename[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"Obj2\",\
		\"DeviationType\" : \"Rename\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"MESH\"}\
" };
	server.sendData(std::to_array(jsonRename));
	server.sendData(createBuffer("Obj2NewName"));

	constexpr char jsonDeletion[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"Obj2NewName\",\
		\"DeviationType\" : \"Deletion\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"MESH\"}\
" };

	server.sendData(std::to_array(jsonDeletion));
	server.sendData(createBuffer(""));

	while (!bComplete0) { devDecoder.tick(); }

	constexpr char jsonTransform[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"ObjBase\",\
		\"DeviationType\" : \"Transform\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"MESH\"}\
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
		\"ObjectType\" : \"MESH\"}\
" };

	server.sendData(std::to_array(jsonMesh));
	
//	const auto aMeshData{ std::to_array({
//		TestVec3(1.f, 2.f, 3.f), TestVec3(4.f, 5.f, 6.f), 
//		TestVec3(7.f, 8.f, 9.f), TestVec3(10.f, 11.f, 12.f),
//		TestVec3(13.f, 14.f, 15.f), TestVec3(16.f, 17.f, 18.f)}) };

	server.sendData(genTestvertexData());

	while (!bComplete1) { devDecoder.tick(); }

}

void compareCamera(auto objId, auto& logger, const auto rawCameraData)
{

	const auto oCamData{ UACE::PkgBlobCoder::decodeCamera(rawCameraData) };
	EXPECT_TRUE(oCamData.has_value());
	const auto cData{ oCamData.value() };

	EXPECT_EQ(cData.posX, 5.f);
	EXPECT_EQ(cData.posY, 6.f);
	EXPECT_EQ(cData.posZ, -7.f);

	EXPECT_EQ(cData.rotAngle, 1.f);
	EXPECT_EQ(cData.rotX, 2.f);
	EXPECT_EQ(cData.rotY, 3.f);
	EXPECT_EQ(cData.rotZ, 4.f);

	EXPECT_EQ(cData.type, 1);

	EXPECT_EQ(cData.orthographicScale, 6.f);

	EXPECT_EQ(cData.clipStart, 0.001f);
	EXPECT_EQ(cData.clipEnd, 100.f);

	EXPECT_EQ(cData.aspectRatio, 1.5f);

	EXPECT_EQ(cData.fov, 23.f);

	std::array<char, 49> aLoadedCamera{};
	EXPECT_EQ(aLoadedCamera.size(), rawCameraData.size());

	const auto bLoaded{ logger.getCamera(objId, aLoadedCamera) };
	EXPECT_TRUE(bLoaded);
	EXPECT_EQ(std::strncmp(aLoadedCamera.data(), rawCameraData.data(), rawCameraData.size()), 0);

}

TEST(deviationDecoder, decodeCamera)
{

	prepareDatabase();
	UACE::DeviationLogger logger("loggerDatabase.sqlite");

	hfog::Alloc::Unified<128_B, 1_kB> ubAlloc;

	BasicTCPServer server(6000);
	std::atomic_flag syncPoint = ATOMIC_FLAG_INIT;

	const auto cbOnCameraCreation = [&syncPoint, &logger](size_t objId, std::span<const char> rawCameraData)
	{	

		EXPECT_EQ(objId, 1);

		compareCamera(objId, logger, rawCameraData);

		syncPoint.test_and_set();

	};

	const auto cbOnStaticMeshCreation = [](size_t, std::span<const char>)
	{	};

	const auto cbOnRename = [](size_t, size_t, size_t, size_t, bool)
	{	};

	const auto onDeletion = [](size_t )
	{	};

	const auto cbOnTransform = [](size_t, std::span<const char>)
	{	};

	const auto cbOnMesh = [](size_t, std::span<const char>)
	{	};

	const auto onCamera = [&logger, &syncPoint](size_t objId, std::span<const char> rawCameraData)
	{

		EXPECT_EQ(objId, 1);

		compareCamera(objId, logger, rawCameraData);

		syncPoint.test_and_set();

	};

	UACE::Deviation::Desc desc{ 
		cbOnCameraCreation,
		cbOnStaticMeshCreation,
		onDeletion, 
		cbOnRename, 
		cbOnTransform, 
		cbOnMesh, 
		onCamera };

	UACE::DeviationDecoder devDecoder(&ubAlloc, desc, "loggerDatabase.sqlite", "127.0.0.1", 6000);

	const auto createBuffer = []<size_t size>(const char(&charArray)[size])
	{
		std::array<char, size - 1> sendData{};
		memcpy(sendData.data(), charArray, sendData.size());
		return sendData;
	};

	constexpr char jsonCreateCamera[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"Cam1\",\
		\"DeviationType\" : \"Creation\",\
		\"DeviationId\" : 1,\
		\"ObjectType\" : \"CAMERA\"}\
" };

	server.sendData(std::to_array(jsonCreateCamera));
	server.sendData(createCameraData());

	while (!syncPoint.test()) { devDecoder.tick(); }
	syncPoint.clear();

	constexpr char jsonCameraData[]{ "\
		{\"PkgType\": \"Deviation\",\
		\"ObjectName\" : \"Cam1\",\
		\"DeviationType\" : \"Camera\",\
		\"DeviationId\" : 2,\
		\"ObjectType\" : \"Entity\"}\
" };

	server.sendData(std::to_array(jsonCameraData));
	server.sendData(createCameraData());

	while (!syncPoint.test()) { devDecoder.tick(); }

//	syncPoint.wait(true);

}
