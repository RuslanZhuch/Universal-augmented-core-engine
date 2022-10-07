#include "scriptAssists/tableRow.h"
#include "scriptAssists/scriptCore.h"

namespace Tables
{
	struct Damage
	{
		Damage(char* ptr, size_t numOfBytes) : 
			t1(ptr + 0, 512 * sizeof(int16_t) + 0),
			t2(ptr + 0 + 512 * sizeof(int16_t), 512 * sizeof(int8_t) + 0 + 512 * sizeof(int16_t)),
			t3(ptr + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t), 512 * sizeof(uint32_t) + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t)),
			t4(ptr + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t) + 512 * sizeof(uint32_t), 512 * sizeof(uint16_t) + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t) + 512 * sizeof(uint32_t))
		{	}
		TableRow<int16_t> t1;
		TableRow<int8_t> t2;
		TableRow<uint32_t> t3;
		TableRow<uint16_t> t4;
		int numOfElements{};
	};
};