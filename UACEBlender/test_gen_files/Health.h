#include "scriptAssists/tableRow.h"
#include "scriptAssists/scriptCore.h"

namespace Tables
{
	struct Health
	{
		Health(char* ptr, size_t numOfBytes) : 
			health(ptr + 0, 1024 * sizeof(int32_t) + 0)
		{	}
		TableRow<int32_t> health;
		int numOfElements{};
	};
};