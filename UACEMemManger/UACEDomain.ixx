export module UACEDomain;

import MemoryManagerCommon;

using namespace UACE::MemManager::Literals;

export namespace UACE::MemManager
{

	class Domain
	{

	public:

		using DomainId = unsigned int;

	public:

		Domain() = default;

		Domain(MemSize size, MemSize offset, char* rootPtr)
			: size(size), offset(offset), freeSpace(size), rootPtr(rootPtr)
		{
		}

		constexpr auto getOffset() const noexcept { return this->offset; }
		constexpr auto getSize() const noexcept { return this->size; }
		constexpr auto getFreeSpace() const noexcept { return this->freeSpace; }

//		DomainId createDomain(MemSize size);
//		DomainDesc getDomainDesc(DomainId id);

		MemSize reserveMemory(MemSize size)
		{
			
			constexpr MemSize minMemBlock = 16_b;
			const auto numOfMemBlocks{ size / minMemBlock };
			const auto fraction{ size % minMemBlock };
			const auto bNeedExpand{ fraction > 0 };

			const auto expandedBlocks{ numOfMemBlocks + 1 * bNeedExpand };

			const auto needMemorySize{ expandedBlocks * minMemBlock };

			if (this->freeSpace < needMemorySize)
			{
				return 0;
			}

			this->freeSpace -= needMemorySize;

			return needMemorySize;

		}
//
		char* getMemoryPointer() const noexcept
		{

			const auto offset{ this->offset };
			const auto size{ this->size };

			auto offsetPtr{ this->rootPtr + offset };

			return offsetPtr;

		}
//
		void reset() noexcept
		{

			this->freeSpace = this->size;

		}
//
//		void clearDomains();

	private:

		MemSize offset{ 0 };
		MemSize size{ 0 };
		MemSize freeSpace{ 0 };

		char* rootPtr{ nullptr };

	};



};