#include "UACEMemPool.h"

#include <array>

namespace UACEMem = UACE::MemManager;

UACEMem::Pool::PoolSize UACEMem::Pool::allocPool(MemSize bytes)
{

	if ((this->data != nullptr) || (bytes == 0))
	{
		return 0;
	}

	this->data = new char [bytes];
	this->size = bytes;

	return bytes;

}

UACEMem::Pool::PoolSize UACEMem::Pool::getPoolSize()
{

	return this->size;

}

bool UACEMem::Pool::setPoolBytes(const char* bytes, MemSize size)
{

	if (this->size < size)
	{
		return false;
	}

	const auto err{ memcpy_s(this->data, this->size, bytes, size) };

	return err == 0;

}

bool UACEMem::Pool::getPoolBytes(char* bytes, MemSize size)
{

	auto srcBytes{ this->data };
	if (srcBytes == nullptr)
	{
		return false;
	}
	const auto srcSizes{ this->size };

	const auto err{ memcpy_s(bytes, size, srcBytes, size) };

	return err == 0;

}

char* UACEMem::Pool::getPoolPtr()
{

	auto srcBytes{ this->data };
	return srcBytes;

}

void UACEMem::Pool::dealloc()
{

	delete[] this->data;
	this->data = nullptr;
	this->size = 0;

}