module;
#include <type_traits>
#include <string>
#include <array>
#include <span>

#include <variant>

export module UACEDeviationDecoder;
import UACEClient;
import UACEJsonCoder;
import UACEDeviationLogger;
import UACEViewerUtils;
import UACEAllocator;
import MemoryManagerCommon;

using namespace UACE::MemManager::Literals;

export namespace UACE::Deviation
{

	template <
		typename OnCameraCreation,
		typename OnStaticMeshCreation,
		typename OnDeletion,
		typename OnRename,
		typename OnTransform,
		typename OnMesh,
		typename OnCamera
	>
	struct Desc
	{
		OnCameraCreation cbOnCameraCreation {};
		OnStaticMeshCreation cbOnStaticMeshCreation {};
		OnDeletion cbOnDeletion {};
		OnRename cbOnRename {};
		OnTransform cbOnTransform {};
		OnMesh cbOnMesh{};
		OnCamera cbOnCamera{};
	};

	template<typename Desc>
	concept DeviationDesc = requires(Desc && d)
	{
		requires std::is_invocable_v<decltype(d.cbOnCameraCreation), size_t, std::span<const char>>;
		requires std::is_invocable_v<decltype(d.cbOnStaticMeshCreation), size_t, std::span<const char>>;
		requires std::is_invocable_v<decltype(d.cbOnDeletion), size_t>;
		requires std::is_invocable_v<decltype(d.cbOnRename), size_t, size_t, size_t, size_t, bool>;
		requires std::is_invocable_v<decltype(d.cbOnTransform), size_t, std::span<const char>>;
		requires std::is_invocable_v<decltype(d.cbOnMesh), size_t, std::span<const char>>;
		requires std::is_invocable_v<decltype(d.cbOnCamera), size_t, std::span<const char>>;
	};

}

export namespace UACE
{

	template <UACE::Deviation::DeviationDesc DevDesc, UACE::MemManager::Allocator Alloc>
	class DeviationDecoder
	{

	public:

	public:

		DeviationDecoder() = delete;
		explicit constexpr DeviationDecoder(
			Alloc* alloc, 
			const DevDesc& desc, 
			const std::string_view dbFile, 
			const std::string_view ip, 
			int port,
			size_t bufferSize = 1_kB)
			:desc(desc), client(alloc, ip, port, bufferSize)
		{
			std::memset(this->dbName, 0, sizeof(this->dbName));
			std::memcpy(this->dbName, dbFile.data(), dbFile.size());
		}

		constexpr void tick()
		{
			if (this->client.getNumOfPkgs() < 2)
			{
				return;
			}

			const auto recSize0{ client.popPkg(this->recPkg.data(), this->recPkg.size()) };

			const auto pkgVariant{ UACE::JsonCoder::decode({ this->recPkg.data(), recSize0 }) };

			if (const auto pkg{ std::get_if<static_cast<size_t>(UACE::JsonCoder::PKG_TYPE::DEVIATION)>(&pkgVariant) }; pkg != nullptr)
			{

				auto logger{ UACE::DeviationLogger(this->dbName) };

				const auto recSize1{ client.popPkg(recPkg.data(), recPkg.size()) };

				const auto objectName{ pkg->objectName.data() };
				const auto hashObjectId{ UACE::ViewerUtils::hashString(objectName) };

				if (strcmp(pkg->deviationType.data(), "Transform") == 0)
				{
					const auto objId{ logger.getObjectId(hashObjectId) };
					if (logger.setObjectTransform(objId, { recPkg.data(), recSize1 }))
						this->desc.cbOnTransform(objId, { recPkg.data(), recSize1 });
				}
				else if (strcmp(pkg->deviationType.data(), "Mesh") == 0)
				{
					const auto objId{ logger.getObjectId(hashObjectId) };
					if (logger.setObjectMesh(objId, { recPkg.data(), recSize1 }))
						this->desc.cbOnMesh(objId, { recPkg.data(), recSize1 });
				}
				else if (strcmp(pkg->deviationType.data(), "Creation") == 0)
				{
					const auto metadata{ std::string_view(recPkg.data(), recSize1) };
					const auto newObjId{ logger.logNewObject(hashObjectId, metadata) };


					if (pkg->objectType == UACE::JsonCoder::ObjectType::CAMERA)
						this->desc.cbOnCameraCreation(newObjId, metadata);
					else if (pkg->objectType == UACE::JsonCoder::ObjectType::MESH)
						this->desc.cbOnStaticMeshCreation(newObjId, metadata);
				}
				else if (strcmp(pkg->deviationType.data(), "Camera") == 0)
				{
					const auto cameraRawData{ std::string_view(recPkg.data(), recSize1) };
					
					const auto objId{ logger.getObjectId(hashObjectId) };
					if (logger.setCamera(objId, cameraRawData))
						this->desc.cbOnCamera(objId, cameraRawData);
				}
				else if (strcmp(pkg->deviationType.data(), "Deletion") == 0)
				{
					const auto objId{ logger.getObjectId(hashObjectId) };
					logger.removeObject(objId);
					this->desc.cbOnDeletion(objId);
				}
				else if (strcmp(pkg->deviationType.data(), "Rename") == 0)
				{
					const auto newName{ std::string_view(recPkg.data(), recSize1) };
					const auto hashNewNameId{ UACE::ViewerUtils::hashString(newName) };

					const auto objId{ logger.getObjectId(hashObjectId) };

					logger.rename(objId, hashNewNameId);

					this->desc.cbOnRename(objId, objId, hashObjectId, hashNewNameId, false);
				}
				else if (strcmp(pkg->deviationType.data(), "Swap") == 0)
				{
					const auto newName{ std::string_view(recPkg.data(), recSize1) };
					const auto hashNewNameId{ UACE::ViewerUtils::hashString(newName) };

					const auto objId{ logger.getObjectId(hashObjectId) };

					this->desc.cbOnRename(objId, objId, hashObjectId, hashNewNameId, true);
				}

				const auto devId{ pkg->deviationId };


			}
			else
			{
				// Not happy path check
				[[maybe_unused]] const auto bPopped{ client.popPkg(recPkg.data(), recPkg.size()) };
			}

		}

	private:

	private:

		DevDesc desc;
		UACE::Client<Alloc> client;

		std::array<char, 1_kB> recPkg{};

		char dbName[32];

	};

}