module;
#include <vector>
#include <array>
export module UACEMemPool;

export import hfog.Alloc;
export import UACEDomain;

export namespace UACE::MemManager
{

	class Pool
	{

	public:

		using PoolSize = mem_t;

		Pool() = delete;
		Pool(const Pool&) = delete;
		Pool(Pool&&) = delete;
		Pool& operator=(const Pool&) = delete;
		Pool& operator=(Pool&&) = delete;

		constexpr Pool(PoolSize size)
		{

			this->vBuffer.resize(size);
			this->ptr = this->vBuffer.data();
			this->size = size;

		}

		[[nodiscard]] constexpr PoolSize getSize() const noexcept { return this->size; }

		[[nodiscard]] constexpr char* getPtr() noexcept { return this->ptr; }

		[[nodiscard]] constexpr MemManager::Domain* createDomain(mem_t size)
		{

			const auto newDomainOffset{ this->totalDomainSize };
			const auto needPoolSize{ newDomainOffset + size };
			if (this->size < needPoolSize)
			{
				return nullptr;
			}

			this->aDomains[this->numOfDomains] = 
				MemManager::Domain(size, this->totalDomainSize, this->ptr);
			this->totalDomainSize += size;

			this->numOfDomains++;

			return &this->aDomains[this->numOfDomains - 1];

		}

		constexpr void clearDomains()
		{

			for (size_t domainId{ 0 }; domainId < this->numOfDomains; domainId++)
			{

				this->aDomains[domainId] = Domain();

			}

		}

	private:

		PoolSize size{ 0 };
		std::vector<char> vBuffer{};
		char* ptr{ nullptr };

		std::array<MemManager::Domain, 32> aDomains;
		size_t numOfDomains{ 0 };
		mem_t totalDomainSize{ 0 };

	};

}