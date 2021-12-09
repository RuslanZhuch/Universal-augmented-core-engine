export module MemoryManagerCommon;

export namespace UACE::MemManager
{

	using MemSize = unsigned int;

}

export namespace UACE::MemManager::Literals
{

	constexpr unsigned int operator "" _b(unsigned long long in)
	{
		return static_cast<unsigned int>(in);
	}

	constexpr unsigned int operator "" _kb(unsigned long long in)
	{
		return static_cast<unsigned int>(in) * 1024;
	}

}