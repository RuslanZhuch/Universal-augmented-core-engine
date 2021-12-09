module;
#include <array>

export module ScriptData;

export inline constexpr auto funcDataProto{ std::to_array<int>({
	//----------------Function math plus
	//Function address 0
	0,
	//Num of params 1
	2,
	//Params:
	//First param 2
	4,
	//Second param 3
	2,
	//----------------Function math negate
	//Function negate 4
	0,
	//NumOfParams 5
	1,
	//Param 6
	0,
	//----------------Function get obj1
	//Function get obj1 7
	0,
	//Num of params 8
	0,
	//----------------Function process object
	// Function process object 9
	0,
	//Num of params 10
	3,
	//First param (summed val) 11
	0,
	//Second param (negated val) 12
	0,
	//Third param (objects ptr) 13
	0
	}) };

export inline constexpr auto structDataProto{ std::to_array<int>({
	//Sum function address 0
	0,
	//Num of return pointers 1
	2,
	//Num of links pointers 2 
	1,
	//First return pointer 3
	6,
	//Second return pointer 4
	11,
	//Ptr of next link 5
	6,
	//Link 1 Negate func ptr position 6
	4,
	//Num of return pointers 7
	1,
	//Num of links pointers 8
	1,
	//First return pointers 9
	12,
	//Ptr of next link 10
	11,
	//Link 1 Get obj1 func ptr position 11
	7,
	//Num of return pointers 12
	1,
	//Num of link pointers 13
	1,
	//First return pointer 14
	13,
	//Ptr of next link 15
	16,
	//Link 1 Process object func ptr position 16
	9,
	//Num of return pointers 17
	0,
	//Num of link pointers 18
	0,
	//First ret (null) 19
	0
	}) };

