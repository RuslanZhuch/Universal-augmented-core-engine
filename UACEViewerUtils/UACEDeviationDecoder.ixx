module;
#include <type_traits>
#include <string>
#include <array>

#include <variant>

export module UACEDeviationDecoder;
import UACEClient;
import UACEJsonCoder;
import UACEDeviationLogger;
import UACEViewerUtils;
import MemoryManagerCommon;

using namespace UACE::MemManager::Literals;

export namespace UACE::Deviation
{

	template <typename OnCreation,
		typename OnDeletion,
		typename OnRename,
		typename OnTransform,
		typename OnMesh>
	struct Desc
	{
		Desc() = delete;
		explicit Desc(
			const OnCreation& onCreation, 
			const OnDeletion& onDeletion,
			const OnRename& onRename,
			const OnTransform& onTransform,
			const OnMesh& onMesh) :
			cbOnCreation(onCreation),
			cbOnDeletion(onDeletion),
			cbOnRename(onRename),
			cbOnTransform(onTransform),
			cbOnMesh(onMesh)
		{}

		OnCreation cbOnCreation {};
		OnDeletion cbOnDeletion {};
		OnRename cbOnRename {};
		OnTransform cbOnTransform {};
		OnMesh cbOnMesh{};
	};

	template<typename Desc>
	concept DeviationDesc = requires(Desc && d)
	{
		requires std::is_invocable_v<decltype(d.cbOnCreation), size_t, size_t, bool>;
		requires std::is_invocable_v<decltype(d.cbOnDeletion), size_t>;
		requires std::is_invocable_v<decltype(d.cbOnRename), size_t, size_t, size_t, size_t, bool>;
		requires std::is_invocable_v<decltype(d.cbOnTransform), size_t, char*, size_t>;
		requires std::is_invocable_v<decltype(d.cbOnMesh), size_t, char*, size_t>;
	};

}

export namespace UACE
{

	template <UACE::Deviation::DeviationDesc DevDesc, typename Alloc>
	class DeviationDecoder
	{

	public:

	public:

		DeviationDecoder() = delete;
		explicit DeviationDecoder(Alloc* alloc, const DevDesc& desc, const std::string_view dbFile, const std::string_view ip, int port)
			:desc(desc), client(alloc, ip, port)
		{
			std::memset(this->dbName, 0, sizeof(this->dbName));
			std::memcpy(this->dbName, dbFile.data(), dbFile.size());
		}

		void tick()
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
					logger.setObjectTransform(objId, { recPkg.data(), recSize1 });
					this->desc.cbOnTransform(objId, this->recPkg.data(), recSize1);
				}
				else if (strcmp(pkg->deviationType.data(), "Mesh") == 0)
				{
					const auto objId{ logger.getObjectId(hashObjectId) };
					logger.setObjectMesh(objId, { recPkg.data(), recSize1 });
					this->desc.cbOnMesh(objId, this->recPkg.data(), recSize1);
				}
				else if (strcmp(pkg->deviationType.data(), "Creation") == 0)
				{
					const auto baseObjectName{ std::string_view(recPkg.data(), recSize1) };
					const auto hashBaseObjectId{ UACE::ViewerUtils::hashString(baseObjectName) };

					const auto newObjId{ logger.logNewObject(hashObjectId, hashBaseObjectId) };
					size_t baseObjId{ 0 };
					if (hashBaseObjectId != 0)
					{
						baseObjId = logger.getObjectId(hashBaseObjectId);
					}

					this->desc.cbOnCreation(newObjId, baseObjId, hashBaseObjectId != 0);
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

			}
			else
			{
				client.popPkg(recPkg.data(), recPkg.size());
			}

		}

	private:

	private:

		DevDesc desc;
		UACE::Client<Alloc> client;

		std::array<char, 1_kb> recPkg{};

		char dbName[32];

	};

}