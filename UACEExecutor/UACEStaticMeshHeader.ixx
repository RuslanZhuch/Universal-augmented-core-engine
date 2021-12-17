module;
#include <cinttypes>
#include <cstring>

export module UACEStaticMeshHeader;

import UACEAllocator;
import UACEDirectoryHeader;

export namespace UACE::Map
{

	namespace StaticMeshHeaderUtils
	{
		struct Element
		{
			uint32_t address{};
			uint32_t size{};
		};
	};

	template <UACE::MemManager::Allocator Alloc>
	class StaticMeshHeader
	{

	public:
		explicit StaticMeshHeader(Alloc* alloc, size_t numOfElements)
			:elements(alloc->createRaw<StaticMeshHeaderUtils::Element>(numOfElements)),
			header(alloc, numOfElements),
			maxElements(numOfElements)
		{
			this->clear();
		}

		constexpr auto getIsValid() 
		{
			return this->header.getReady() &&
				this->elements.allocPtr != nullptr;
		}

		bool create(uint32_t objId, uint32_t address, uint32_t size)
		{

			if (size == 0 || 
				this->header.find(objId) != UACE::Map::DirectoryHeaderUtils::NotFound)
			{
				return false;
			}

			StaticMeshHeaderUtils::Element* currElement{ this->elements.ptr };
			for (size_t numChecked{ 0 }; numChecked < this->maxElements; numChecked++, currElement++)
			{
				if (currElement->size == 0)
				{
					currElement->size = size;
					currElement->address = address;

					const uint32_t offset{ static_cast<uint32_t>(currElement - this->elements.ptr) };
					return this->header.put(objId, offset);
				}
			}

			return false;

		}

		StaticMeshHeaderUtils::Element get(uint32_t objId)
		{

			const auto id{ this->header.find(objId) };

			StaticMeshHeaderUtils::Element element{};
			if (id != UACE::Map::DirectoryHeaderUtils::NotFound)
			{
				element = *(elements.ptr + id);
			}

			return element;

		}

		bool remove(uint32_t objId)
		{

			const auto id{ this->header.find(objId) };
			if (id == UACE::Map::DirectoryHeaderUtils::NotFound)
				return false;

			UACE::Map::StaticMeshHeaderUtils::Element* element{ this->elements.ptr + id };
			element->size = 0;
			this->header.remove(objId);

			return true;

		}

		void clear()
		{

			std::memset(this->elements.ptr, 0, this->elements.numOfElements * sizeof(StaticMeshHeaderUtils::Element));

		}

	private:
		UACE::MemManager::Ptr<StaticMeshHeaderUtils::Element, Alloc> elements;
		UACE::Map::DirectoryHeader<Alloc> header;

		size_t maxElements{};
		size_t numOfUsed{};

	};

};