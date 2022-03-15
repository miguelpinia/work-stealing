#include <iostream>
#include <list>
#include <vector>
#include "ws/lib.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;

class VertexTest : public ::testing::Test
{
protected:
    VertexTest() {}
    ~VertexTest() {}
    void SetUp() {}
    void TearDown() {}
};

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

class EdgeTest : public ::testing::Test
{
protected:
    EdgeTest() {}
    ~EdgeTest() {}
    void SetUp() {}
    void TearDown() {}
};

TEST_F(EdgeTest, testSrcAndDest)
{
    edge e(10, 12);
    EXPECT_EQ(e.getSrc(), 10);
    EXPECT_EQ(e.getDest(), 12);
}

class GraphTest : public ::testing::Test
{
protected:
    GraphTest() {
    }
    ~GraphTest() {}
    void SetUp() {
        directed = false;
        edges.push_back(edge(4,0));
        edges.push_back(edge(0,1));
        edges.push_back(edge(1,2));
        edges.push_back(edge(2,3));
        g = graph(edges, directed, 4, 5, GraphType::RANDOM);
    }
    void TearDown() {}
    std::vector<edge> edges;
    graph g;
    bool  directed;
};


TEST_F(GraphTest, testAddEdgeDirected)
{
    g.setDirected(true);
    g.addEdge(edge(3,4));
    std::list<int> expected = {2, 4};
    std::list<int> neighbours = g.getNeighbours(3);
    EXPECT_THAT(expected, ::testing::ContainerEq(neighbours));
}

TEST_F(GraphTest, testAddEdgeUndirected)
{
    g.setDirected(false);
    g.addEdge(edge(3, 4));
    std::list<int> l0 = {0};
    std::list<int> l1 = {4};
    EXPECT_EQ(l0.front(), g.getNeighbours(4).front());
    EXPECT_EQ(l1.front(), g.getNeighbours(0).front());
}

TEST_F(GraphTest, testAddEdges)
{
    std::vector<edge> newEdges;
    graph g1(newEdges, true, 0, 5, GraphType::RANDOM);
    g1.addEdges(edges);
    std::list<int> l0 = {1};
    std::list<int> l1 = {2};
    std::list<int> l2 = {3};
    std::list<int> l4 = {0};
    EXPECT_EQ(l0.front(), g1.getNeighbours(0).front());
    EXPECT_EQ(l1.front(), g1.getNeighbours(1).front());
    EXPECT_EQ(l2.front(), g1.getNeighbours(2).front());
    EXPECT_EQ(l4.front(), g1.getNeighbours(4).front());
}

TEST_F(GraphTest, testDeleteEdgeDirected)
{
    graph g1(edges, true, 0, 5, GraphType::RANDOM);
    g1.deleteEdge(edge(0, 1));
    std::list<int> expected = {2};
    EXPECT_TRUE(g1.getNeighbours(0).empty());
    EXPECT_THAT(expected, ::testing::ContainerEq(g1.getNeighbours(1)));
}

TEST_F(GraphTest, testDeleteEdgeUndirected)
{
    graph g1(edges, false, 0, 5, GraphType::RANDOM);
    g1.deleteEdge(edge(0, 1));
    EXPECT_EQ(1, int(g1.getNeighbours(0).size()));
    EXPECT_EQ(1, int(g1.getNeighbours(1).size()));
}

TEST_F(GraphTest, testHasEdge)
{
    graph g1(edges, false, 0, 5, GraphType::RANDOM);
    EXPECT_TRUE(g1.hasEdge(edge(0,1)));
}

TEST_F(GraphTest, testHasNeighbours)
{
    EXPECT_TRUE(g.getNeighbours(1).size() > 1);
}

TEST_F(GraphTest, testGetRoot)
{
    EXPECT_EQ(4, g.getRoot());
}

TEST_F(GraphTest, testGetSize)
{
    EXPECT_EQ(5, g.getNumberVertices());
}


TEST_F(GraphTest, testGetType)
{
    EXPECT_EQ(GraphType::RANDOM, g.getType());
}

TEST_F(GraphTest, testGetNumberOfEdges)
{
    graph g1(edges, false, 0, 5, GraphType::RANDOM);
    EXPECT_EQ(8, g.getNumberEdges());
}

TEST_F(GraphTest, testGetChilds)
{
    std::vector<edge> emptyEdges;
    graph g1(emptyEdges, true, 4, 5, GraphType::RANDOM);
    g1.addEdges(edges);
    std::list<int> l0 = {4};
    std::list<int> l1 = {0};
    std::list<int> l2 = {1};
    std::list<int> l3 = {2};
    EXPECT_THAT(l0, ::testing::ContainerEq(g1.getChilds(0)));
    EXPECT_THAT(l1, ::testing::ContainerEq(g1.getChilds(1)));
    EXPECT_THAT(l2, ::testing::ContainerEq(g1.getChilds(2)));
    EXPECT_THAT(l3, ::testing::ContainerEq(g1.getChilds(3)));
}


class TaskArrayTest : public ::testing::Test
{
protected:
    TaskArrayTest() {}
    ~TaskArrayTest() {}
    void SetUp() {}
    void TearDown() {}
};

TEST_F(TaskArrayTest, testGetSize)
{
    taskArrayWithSize array(100);
    EXPECT_EQ(100, array.getSize());
}

TEST_F(TaskArrayTest, testSet)
{
    taskArrayWithSize array(10);
    array.set(0, 10101);
    EXPECT_EQ(10101, array.get(0));
}

TEST_F(TaskArrayTest, testGet)
{
    taskArrayWithSize array(10);
    EXPECT_EQ(-1, array.get(0));
    array.set(0, 10101);
    EXPECT_EQ(10101, array.get(0));
}

class wsncmultTest : public ::testing::Test
{
protected:
    wsncmultTest() {}
    ~wsncmultTest() {}
    void SetUp() {}
    void TearDown() {}
};

TEST_F(wsncmultTest, testIsEmpty)
{
    wsncmult ws(10, 1);
    EXPECT_EQ(true, ws.isEmpty(0));
    wsncmult ws1(10, 2);
    EXPECT_EQ(true, ws1.isEmpty(0));
    ws1.put(10, 1);
    EXPECT_EQ(false, ws1.isEmpty(0));
    EXPECT_EQ(false, ws1.isEmpty(1));
}

TEST_F(wsncmultTest, testNotEmpty)
{
    wsncmult ws(10, 4);
    ws.put(10, 3);
    EXPECT_EQ(false, ws.isEmpty(3));
}

TEST_F(wsncmultTest, testFIFO_take)
{
    wsncmult ws(10, 1);
    for (int i = 0; i < 10; i++) {
        bool inserted = ws.put(i, 0);
        EXPECT_TRUE(inserted);
    }
    for (int i = 0; i < 10; i++) {
        int output = ws.take(0);
        EXPECT_EQ(i, output);
    }
}

TEST_F(wsncmultTest, testFIFO_steal)
{
    wsncmult ws(10, 1);
    for (int i = 0; i < 10; i++) {
        bool inserted = ws.put(i, 0);
        EXPECT_TRUE(inserted);
    }
    for (int i = 0; i < 10; i++) {
        int output = ws.steal(0);
        EXPECT_EQ(i, output);
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
