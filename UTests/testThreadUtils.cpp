#pragma warning(disable: 5050)
#include "gtest/gtest.h"

import UACEGuardCopy;

TEST(threadsUtils, tsGuardCopy)
{

	using objToWait_t = int;

	objToWait_t valToWait{ 5 };

	auto owner{ UACE::GuardCopy::Holder(&valToWait) };
	auto mirror{ owner.createMirror() };

	EXPECT_FALSE(owner.getReturned());

	EXPECT_EQ(mirror.getPtr(), &valToWait);

	EXPECT_EQ(valToWait, 5);

	EXPECT_EQ(owner.createMirror().getPtr(), nullptr);

	mirror.returnVal();

	EXPECT_TRUE(owner.getReturned());

	EXPECT_EQ(mirror.getPtr(), nullptr);

	EXPECT_NE(owner.createMirror().getPtr(), nullptr);

}

TEST(threadsUtils, tsGuardCopyLifetime)
{

	using objToWait_t = int;

	objToWait_t valToWait{ 5 };

	auto owner{ UACE::GuardCopy::Holder(&valToWait) };
	{
		auto mirror{ owner.createMirror() };
		EXPECT_FALSE(owner.getReturned());
	}
	EXPECT_TRUE(owner.getReturned());

}

TEST(threadsUtils, tsGuardCopyReturn)
{

	using objToWait_t = int;
	using objToReturn_t = bool;

	objToWait_t valToWait{ 5 };

	auto owner{ UACE::GuardCopy::Holder<objToWait_t*, bool>(&valToWait) };
	auto mirror{ owner.createMirror() };

	EXPECT_FALSE(owner.getReturned().has_value());

	EXPECT_EQ(mirror.getPtr(), &valToWait);

	EXPECT_EQ(valToWait, 5);

	mirror.returnVal(true);

	EXPECT_TRUE(owner.getReturned().has_value());
	EXPECT_TRUE(owner.getReturned().value());

	EXPECT_EQ(mirror.getPtr(), nullptr);

	mirror.returnVal(false);
	EXPECT_TRUE(owner.getReturned().value());

}