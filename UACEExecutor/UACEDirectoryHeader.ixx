module;
#include <cinttypes>
#include <limits>
#include <type_traits>

export module UACEDirectoryHeader;

import UACEAllocator;
import UACEArray;

export namespace UACE::Map
{

	namespace DirectoryHeaderUtils
	{
		constexpr uint32_t NotFound = std::numeric_limits<uint32_t>::max();
	};

	template <UACE::MemManager::Allocator Alloc>
	class DirectoryHeader
	{

		struct Element
		{
			uint32_t keyId{};
			uint32_t point{};
		};


	public:
		explicit DirectoryHeader(Alloc* alloc, uint32_t numOfElements) 
			: dataPtr(alloc->createRaw<Element>(numOfElements))
		{

			this->maxElements = this->dataPtr.numOfElements;

			this->clear();

		}

		[[nodiscard]] bool getReady() const { return this->dataPtr.allocPtr != nullptr; }

		[[nodiscard]] auto getMaxElements() const { return this->maxElements; }

		[[nodiscard]] bool put(uint32_t keyId, uint32_t point)
		{

			if (this->newElementPtr == nullptr)
				return false;

			this->newElementPtr->keyId = keyId;
			this->newElementPtr->point = point;

			
			this->newElementPtr++;

			this->endElementPtr = this->newElementPtr;

			const auto bOutOfRange{ this->newElementPtr >= (dataPtr.ptr + dataPtr.numOfElements) };
			this->newElementPtr = bOutOfRange ? nullptr : this->newElementPtr;

			this->bNeedSort = true;

			return true;

		}

		[[nodiscard]] uint32_t find(uint32_t keyId)
		{

			if (this->bNeedSort)
				this->bNeedSort = false;

			for (Element* currElement{ dataPtr.ptr }; currElement != this->endElementPtr; currElement++)
			{
				if (currElement->keyId == keyId)
					return currElement->point;
			}

			return DirectoryHeaderUtils::NotFound;

		}

		bool remove(uint32_t keyId)
		{

			return this->find(keyId, [this](Element* el)
				{

					this->endElementPtr--;
					this->newElementPtr = this->endElementPtr;


					auto lastElement = this->newElementPtr;
					*el = *lastElement;

				});

		}

		bool modify(uint32_t keyId, uint32_t point)
		{

			return this->find(keyId, [point](Element* element)
				{
					element->point = point;
				});

		}

		void clear()
		{
			this->newElementPtr = this->dataPtr.ptr;
			this->endElementPtr = this->newElementPtr;
		}
		
	private:
		template<typename Callback>
		bool find(uint32_t keyId, Callback && callback) requires std::is_invocable_v<Callback, Element*>
		{

			for (Element* currElement{ this->dataPtr.ptr }; currElement != this->endElementPtr; currElement++)
			{

				if (currElement->keyId != keyId)
					continue;
				
				callback(currElement);
				return true;
				
			}

			return false;

		}

	private:
		UACE::MemManager::Ptr<Element, Alloc> dataPtr{ nullptr };
		uint32_t maxElements{};
		Element* newElementPtr{ nullptr };
		Element* endElementPtr{ nullptr };

		bool bNeedSort{ false };

	};

};