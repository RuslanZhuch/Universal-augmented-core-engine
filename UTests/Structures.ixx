module;
#include <array>

export module Structures;

export struct Mat
{
	using mat_t = std::array<std::array<float, 4>, 4>;
	constexpr Mat() = default;
	constexpr Mat(const mat_t& m) : m(m) {}

	friend constexpr auto operator==(const Mat& mleft, const Mat& mright)
	{
		return mleft.m == mright.m;
	}

	mat_t m{};
};
