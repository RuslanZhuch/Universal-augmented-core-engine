module;
#include <fstream>
export module UACEScriptLoader;
export import UACEAllocator;

import UACEScriptDecoder;

export namespace UACE::Script
{

	template<UACE::MemManager::Allocator Alloc>
	[[nodiscard]] constexpr auto loadFromFile(std::string_view filename, Alloc* alloc)
	{

		std::ifstream srcFile(filename.data(), std::ios::binary);
		if (!srcFile.is_open())
		{
			return UACE::Script::Decoder<Alloc>::ScriptData();
		}

		srcFile.seekg(0, std::ios::end);
		const auto dataLen{ srcFile.tellg() };
		srcFile.seekg(0, std::ios::beg);

		std::array<char, 1_kB> fileData;
		srcFile.read(fileData.data(), dataLen);

		auto decoder{ UACE::Script::Decoder(alloc) };
		return decoder.decode(fileData.data());

	}

}

