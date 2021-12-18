module;
#include <array>
export module UACEMemPool;

export import MemoryManagerCommon;
export import UACEDomain;

export namespace UACE::MemManager
{

	class Pool
	{

	public:

		using PoolSize = MemSize;

		Pool() = delete;
		Pool(const Pool&) = delete;
		Pool(Pool&&) = delete;
		Pool& operator=(const Pool&) = delete;
		Pool& operator=(Pool&&) = delete;

		Pool(PoolSize size)
		{

			this->data = new char[size];
			this->size = size;

		}

		[[nodiscard]] constexpr PoolSize getSize() const noexcept { return this->size; }

		[[nodiscard]] constexpr char* getPtr() noexcept { return this->data; }

		[[nodiscard]] MemManager::Domain* createDomain(MemSize size)
		{

			const auto newDomainOffset{ this->totalDomainSize };
			const auto needPoolSize{ newDomainOffset + size };
			if (this->size < needPoolSize)
			{
				return nullptr;
			}

			MemManager::Domain domain(size, this->totalDomainSize, this->data);

			this->totalDomainSize += size;

			this->aDomains[this->numOfDomains] = domain;
			this->numOfDomains++;

			return &this->aDomains[this->numOfDomains - 1];

		}

		void clearDomains()
		{

			for (size_t domainId{ 0 }; domainId < this->numOfDomains; domainId++)
			{

				this->aDomains[domainId] = Domain();

			}

		}

	private:

		PoolSize size{ 0 };
		mutable char* data{ nullptr };

		std::array<MemManager::Domain, 32> aDomains;
		size_t numOfDomains{ 0 };
		MemSize totalDomainSize{ 0 };

	};

}