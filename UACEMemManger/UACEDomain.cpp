//#include "UACEDomain.h"
//
//#include <vector>
//
//using namespace UACE::MemManager::Literals;
//
//namespace udom = UACE::MemManager::Domain;
//
//static std::vector<udom::DomainDesc> vDomains;
//static UACE::MemManager::MemSize gTotalomainsSize{};
//
//udom::DomainId udom::createDomain(MemSize size)
//{
//
//	const auto newDomainOffset{ gTotalomainsSize };
//	const auto needPoolSize{ newDomainOffset + size };
//	if (Pool::getPoolSize() < needPoolSize)
//	{
//		return 0;
//	}
//
//	DomainDesc desc;
//	desc.size = size;
//	desc.freeSpace = desc.size;
//	desc.offset = gTotalomainsSize;
//
//	gTotalomainsSize += size;
//
//	vDomains.push_back(desc);
//
//	return vDomains.size();
//
//}
//
//udom::DomainDesc udom::getDomainDesc(DomainId id)
//{
//
//	const auto zeroBaseDomainId{ id - 1 };
//
//	if (zeroBaseDomainId >= vDomains.size())
//	{
//		return {};
//	}
//
//	return vDomains[zeroBaseDomainId];
//
//}
//
//UACE::MemManager::MemSize udom::reserveMemory(DomainId id, MemSize size)
//{
//
//	const auto zeroBaseDomainId{ id - 1 };
//	if (zeroBaseDomainId >= vDomains.size())
//	{
//		return 0;
//	}
//
//	constexpr MemSize minMemBlock = 16_b;
//	const auto numOfMemBlocks{ size / minMemBlock };
//	const auto fraction{ size % minMemBlock };
//	const auto bNeedExpand{ fraction > 0 };
//
//	const auto expandedBlocks{ numOfMemBlocks + 1 * bNeedExpand };
//
//	const auto needMemorySize{ expandedBlocks * minMemBlock };
//
//	auto& domain{ vDomains[zeroBaseDomainId] };
//	if (domain.freeSpace < needMemorySize)
//	{
//		return 0;
//	}
//
//	domain.freeSpace -= needMemorySize;
//
//	return needMemorySize;
//
//}
//
//char* udom::getMemoryPointer(DomainId id)
//{
//
//	const auto zeroBaseDomainId{ id - 1 };
//	if (zeroBaseDomainId >= vDomains.size())
//	{
//		return nullptr;
//	}
//
//	const auto domain{ vDomains[zeroBaseDomainId] };
//	const auto offset{ domain.offset };
//	const auto size{ domain.size };
//	
//	char* ptr{ UACE::MemManager::Pool::getPoolPtr() };
//	auto offsetPtr{ ptr + offset };
//
//	return offsetPtr;
//
//}
//
//void udom::resetDomain(DomainId id)
//{
//
//	const auto zeroBaseDomainId{ id - 1 };
//	if (zeroBaseDomainId >= vDomains.size())
//	{
//		return;
//	}
//
//	auto& domain{ vDomains[zeroBaseDomainId] };
//	domain.freeSpace = domain.size;
//
//}
//
//void udom::clearDomains()
//{
//
//	vDomains.clear();
//	gTotalomainsSize = 0;
//
//}