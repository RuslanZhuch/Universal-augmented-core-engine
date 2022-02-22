export module MemoryManagerCommon;

export namespace UACE::MemManager
{

	using MemSize = unsigned int;

}

export namespace UACE::MemManager::Literals
{

	consteval unsigned int operator "" _B(unsigned long long in)
	{
		return static_cast<unsigned int>(in);
	}

	consteval unsigned int operator "" _kB(unsigned long long in)
	{
		return static_cast<unsigned int>(in) * 1024;
	}

	consteval unsigned int operator "" _MB(unsigned long long in)
	{
		return static_cast<unsigned int>(in) * 1024 * 1024;
	}

}