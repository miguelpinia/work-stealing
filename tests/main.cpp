#include <iostream>
#include <list>
#include "ws/lib.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"


using ::testing::Return;

class VertexTest : public ::testing::Test
{
protected:
    VertexTest();
    virtual ~VertexTest();
    virtual void SetUp();
    virtual void TearDown();
};


VertexTest::VertexTest() {};

VertexTest::~VertexTest() {};

void VertexTest::SetUp() {};

void VertexTest::TearDown() {};

TEST_F(VertexTest, testInitialValues)
{
    vertex v(true, 10);
    EXPECT_EQ(v.isDirected(), true);
    EXPECT_EQ(v.getValue(), 10);
}

TEST_F(VertexTest, testGetNeighbours)
{
    vertex v(true, 10);
    std::list<int> neighbours = {7, 5, 16, 8};
    v.setNeighbours(neighbours);
    EXPECT_EQ(v.getNeighbours(), neighbours);
}

TEST_F(VertexTest, testGetChilds)
{
    vertex v(true, 10);
    std::list<int> childs = {7, 5, 16, 8};
    v.setChilds(childs);
    EXPECT_EQ(v.getChilds(), childs);
}

TEST_F(VertexTest, testAddNeighbour)
{
    vertex v;
    std::list<int> neighbours = {1,2,3,4,8,9};
    std::list<int> expected = {1,2,3,4,8,9,10};

    v.setNeighbours(neighbours);
    v.addNeighbour(10);
    std::list<int>::iterator it1, it2 = v.getNeighbours().begin();

    EXPECT_EQ(v.getNeighbours().size(), expected.size());
    for(it1 = expected.begin(); it1 != expected.end(); it1++) {
        EXPECT_EQ(*it1, *it2);
        it2++;
    }
}

TEST_F(VertexTest, tessDeleteNeighbour)
{
    vertex v;
    std::list<int> neighbours = {1,2,3,4,8,9,10};
    std::list<int> expected = {1,2,3,4,9,10};

    v.setNeighbours(neighbours);
    v.deleteNeighbour(8);
    std::list<int>::iterator it1, it2 = v.getNeighbours().begin();

    EXPECT_EQ(v.getNeighbours().size(), expected.size());
    for(it1 = expected.begin(); it1 != expected.end(); it1++) {
        EXPECT_EQ(*it1, *it2);
        it2++;
    }
}

TEST_F(VertexTest, testAddChild)
{
    vertex v;
    std::list<int> childs = {1,2,3,4,8,9};
    std::list<int> expected = {1,2,3,4,8,9,10};

    v.setChilds(childs);
    v.addChild(10);
    std::list<int>::iterator it1, it2 = v.getChilds().begin();

    EXPECT_EQ(v.getChilds().size(), expected.size());
    for(it1 = expected.begin(); it1 != expected.end(); it1++) {
        EXPECT_EQ(*it1, *it2);
        it2++;
    }
}

TEST_F(VertexTest, testDeleteChild)
{
    vertex v;
    std::list<int> childs = {1,2,3,4,8,9,10};
    std::list<int> expected = {1,2,3,4,9,10};

    v.setChilds(childs);
    v.deleteChild(8);
    std::list<int>::iterator it1, it2 = v.getChilds().begin();

    EXPECT_EQ(v.getChilds().size(), expected.size());
    for(it1 = expected.begin(); it1 != expected.end(); it1++) {
        EXPECT_EQ(*it1, *it2);
        it2++;
    }
}




int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
