module;

export module UACEScriptBasic;
import UACEAllocator;
import UACEArray;

export namespace UACE::Script
{

	constexpr size_t SCRIPT_DATA_BYTES{ 100 };

	template<UACE::MemManager::Allocator Alloc, typename DType>
	using ubPtr_t = UACE::MemManager::Ptr<DType, Alloc>;

	template<UACE::MemManager::Allocator Alloc>
	using dataPtr_t = ubPtr_t<Alloc, UACE::Containers::Array<int, SCRIPT_DATA_BYTES, Alloc>>;

};