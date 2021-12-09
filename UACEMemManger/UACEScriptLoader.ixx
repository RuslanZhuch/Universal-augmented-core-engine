module;
#include <fstream>
export module UACEScriptLoader;


import UACEScriptDecoder;

export namespace UACE::Script
{

	template<typename _Alloc>
	auto loadFromFile(std::string_view filename, _Alloc* alloc)
	{

		std::ifstream srcFile(filename.data(), std::ios::binary);
		if (!srcFile.is_open())
		{
			return UACE::Script::Decoder<_Alloc>::ScriptData();
		}

		srcFile.seekg(0, std::ios::end);
		const auto dataLen{ srcFile.tellg() };
		srcFile.seekg(0, std::ios::beg);

		std::array<char, 1_kb> fileData;
		srcFile.read(fileData.data(), dataLen);

		auto decoder{ UACE::Script::Decoder(alloc) };
		return decoder.decode(fileData.data());

	}

}

