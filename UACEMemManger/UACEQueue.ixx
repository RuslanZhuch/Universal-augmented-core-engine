module;
#include <array>
#include <mutex>
#include <atomic>

export module UACEQueue;

import UACEUnifiedBlockAllocator;

using ubAlloc = UACE::MemManager::UnifiedBlockAllocator::UnifiedBlockAllocator;

export namespace UACE::UTILS
{

	template<typename T, size_t size>
	class Queue
	{

	public:
		template<typename T>
		struct Node
		{
			T* val{};
			Node* next{ nullptr };
		};

	public:
		Queue(ubAlloc* allocator)
			:allocator(allocator)
		{

		}

		bool push(const T& val)
		{

			std::lock_guard gl(this->m);

			if (this->numOfElements >= size)
			{
				return false;
			}
			const auto wrappedIndex{ this->computeWrappedIndex(this->currBufferIndex) };
			auto node{ this->allocator->create<Node<T>>() };
			auto data{ this->allocator->create<T>(val) };
			node->val = data;
			if (this->head == nullptr)
			{
				this->head = node;
				this->tail = node;
			}
			else
			{
				this->tail->next = node;
				this->tail = node;
			}
			//this->aBuffer[wrappedIndex] = { val };
			this->currBufferIndex++;
			this->numOfElements++;
			this->bIsEmpty = false;
			return true;
		}
		auto pop()
		{

			std::lock_guard gl(this->m);

			if (this->numOfElements == 0)
			{
				return this->allocator->makeUnique<T>(nullptr);
			}

			auto node{ this->head };
			this->head = node->next;

			auto outData{ node->val };
			this->allocator->free(node);

			const auto wrappedIndex{ this->computeWrappedIndex(this->firstBufferIndex) };
			this->numOfElements--;
			this->firstBufferIndex++;
			this->bIsEmpty = (this->numOfElements == 0);
			return this->allocator->makeUnique(outData);

		}

		constexpr auto getIsEmpty() const noexcept { return this->bIsEmpty; }

		constexpr auto getMaxEls() const noexcept { return size; }

	private:
		constexpr auto computeWrappedIndex(size_t val) { return val % size; }

	private:
		ubAlloc* allocator{ nullptr };

		Node<T>* head{ nullptr };
		Node<T>* tail{ nullptr };
		size_t currBufferIndex{ 0 };
		size_t firstBufferIndex{ 0 };
		size_t numOfElements{ 0 };
		bool bIsEmpty{ true };

		std::mutex m;

	};

}
