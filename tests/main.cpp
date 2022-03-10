#include <iostream>
#include "ws/lib.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"


using ::testing::Return;

class TestFoo : public ::testing::Test
{
protected:
    TestFoo();
    virtual ~TestFoo();
    virtual void SetUp();
    virtual void TearDown();
};


TestFoo::TestFoo() {};

TestFoo::~TestFoo() {};

void TestFoo::SetUp() {};

void TestFoo::TearDown() {};

TEST_F(TestFoo, testInitialValues)
{
    vertex v(true, 10);
    EXPECT_EQ(v.isDirected(), true);
    EXPECT_EQ(v.getValue(), 10);
}

TEST_F(TestFoo, secondTest)
{
    EXPECT_EQ(2, 2);
}

TEST_F(TestFoo, thirdTest)
{
    int result = suma(10, 12);
    EXPECT_EQ(22, result);
}




int main(int argc, char** argv)

{
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
