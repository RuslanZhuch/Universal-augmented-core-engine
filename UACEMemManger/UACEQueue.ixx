//module;
//#include <array>
//#include <mutex>
//#include <atomic>
//#include <span>
//
//export module UACEQueue;
//
//import UACEAllocator;
//
//export namespace UACE::UTILS
//{
//
//	template<typename T, size_t size, UACE::MemManager::Allocator Alloc>
//	class Queue
//	{
//
//	public:
//		template<typename T>
//		struct Node
//		{
//			T* val{};
//			Node* next{ nullptr };
//		};
//
//	public:
//		constexpr Queue(Alloc* allocator)
//			:allocator(allocator)
//		{
//
//		}
//
//		[[nodiscard]] constexpr bool push(const T& val)
//		{
//
//			std::lock_guard gl(this->m);
//
//			if (this->numOfElements >= size)
//			{
//				return false;
//			}
//			const auto wrappedIndex{ this->computeWrappedIndex(this->currBufferIndex) };
//			auto node{ this->allocator->template create<Node<T>>() };
//			auto data{ this->allocator->create<T>(val) };
//			node->val = data;
//			if (this->head == nullptr)
//			{
//				this->head = node;
//				this->tail = node;
//			}
//			else
//			{
//				this->tail->next = node;
//				this->tail = node;
//			}
//			//this->aBuffer[wrappedIndex] = { val };
//			this->currBufferIndex++;
//			this->numOfElements++;
//			this->bIsEmpty = false;
//			return true;
//		}
//
//		[[nodiscard]] bool pop(T* outObject)
//		{
//
//			std::lock_guard gl(this->m);
//
//			const auto needSize{ sizeof(T) };
//			if (this->numOfElements == 0)
//			{
//				return false;
//			}
//
//			auto node{ this->head };
//			this->head = node->next;
//
//			auto outData{ node->val };
//			this->allocator->free(node);
//
//			std::memcpy(outObject, outData, needSize);
//			this->allocator->free(outData);
//
//			this->numOfElements--;
//			this->firstBufferIndex++;
//			this->bIsEmpty = (this->numOfElements == 0);
//			
//			return true;
//			
//		}
//
//		[[nodiscard]] constexpr auto getIsEmpty() const noexcept { return this->bIsEmpty; }
//		[[nodiscard]] constexpr auto getMaxEls() const noexcept { return size; }
//
//	private:
//		[[nodiscard]] constexpr auto computeWrappedIndex(size_t val) { return val % size; }
//
//	private:
//		Alloc* allocator{ nullptr };
//
//		Node<T>* head{ nullptr };
//		Node<T>* tail{ nullptr };
//		size_t currBufferIndex{ 0 };
//		size_t firstBufferIndex{ 0 };
//		size_t numOfElements{ 0 };
//		bool bIsEmpty{ true };
//
//		std::mutex m;
//
//	};
//
//}
