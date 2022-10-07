#include "scriptAssists/tableRow.h"
#include "scriptAssists/scriptCore.h"

namespace Tables
{
	struct Position
	{
		Position(char* ptr, size_t numOfBytes) : 
			x(ptr + 0, 65536 * sizeof(float) + 0),
			y(ptr + 0 + 65536 * sizeof(float), 65536 * sizeof(float) + 0 + 65536 * sizeof(float))
		{	}
		TableRow<float> x;
		TableRow<float> y;
		int numOfElements{};
	};
};