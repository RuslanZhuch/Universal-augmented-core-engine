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

		constexpr Domain() = default;

		constexpr Domain(MemSize size, MemSize offset, char* rootPtr)
			: size(size), offset(offset), freeSpace(size), rootPtr(rootPtr)
		{
		}

		[[nodiscard]] constexpr auto getOffset() const noexcept { return this->offset; }
		[[nodiscard]] constexpr auto getSize() const noexcept { return this->size; }
		[[nodiscard]] constexpr auto getFreeSpace() const noexcept { return this->freeSpace; }

		[[nodiscard]] constexpr MemSize reserveMemory(MemSize size)
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

		[[nodiscard]] constexpr char* getMemoryPointer() const noexcept
		{

			const auto offset{ this->offset };
			const auto size{ this->size };

			auto offsetPtr{ this->rootPtr + offset };

			return offsetPtr;

		}

		constexpr void reset() noexcept
		{

			this->freeSpace = this->size;

		}

	private:

		MemSize offset{ 0 };
		MemSize size{ 0 };
		MemSize freeSpace{ 0 };

		char* rootPtr{ nullptr };

	};



};