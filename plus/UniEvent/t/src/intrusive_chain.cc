#include <catch.hpp>

#include <algorithm>
#include <iostream>
#include <ostream>
#include <panda/unievent/Debug.h>
#include <panda/unievent/IntrusiveChain.h>
#include <panda/refcnt.h>
#include <vector>

using namespace panda;
using namespace unievent;

template <typename T, typename C> std::vector<T> to_vector(const C& chain) {
    std::vector<int> v;
    for (auto node : chain) {
        v.push_back(node->value);
    }
    return v;
}

struct Node;
using NodeSP = iptr<Node>;

struct Node : public virtual Refcnt, public IntrusiveChainNode<NodeSP> {
    Node(int v) : value(v) { }
    
    NodeSP clone() const { return NodeSP(new Node(value)); }
    
    int value;
};

// ensure that there is no more than one reference (which is in the holder itself)
template <typename T> bool check_references(const std::vector<T>& holder) {
    return std::all_of(holder.cbegin(), holder.cend(), [](const T& node) { return node->refcnt() == 1; });
}

std::ostream& operator<<(std::ostream& out, const Node& node) { return out << node.value; }

TEST_CASE("push back", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    chain.push_back(new Node(10));
    chain.push_back(new Node(11));
    chain.push_back(new Node(12));
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10, 11, 12}));
}

TEST_CASE("push front", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    chain.push_front(new Node(12));
    chain.push_front(new Node(11));
    chain.push_front(new Node(10));
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10, 11, 12}));
}

TEST_CASE("insert on empty list", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    auto pos = chain.insert(chain.begin(), new Node(10));
    REQUIRE(pos == chain.begin());
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10}));
}

TEST_CASE("insert as first element", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.begin(), new Node(12));
    chain.insert(chain.begin(), new Node(11));
    chain.insert(chain.begin(), new Node(10));
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10, 11, 12}));
}

TEST_CASE("insert as last element", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.end(), new Node(10));
    chain.insert(chain.end(), new Node(11));
    chain.insert(chain.end(), new Node(12));
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10, 11, 12}));
}

TEST_CASE("insert as middle element", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.begin(), holder[0]);
    chain.insert(chain.end(), holder[2]);
    chain.insert(++chain.begin(), holder[1]);
    REQUIRE((to_vector<int>(chain) == std::vector<int>{1, 2, 3}));
    chain.clear();
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("insert pop mixed 1", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.end(), holder[0]);
    chain.pop_back();
    chain.insert(chain.end(), holder[1]);
    chain.pop_back();
    chain.insert(chain.end(), holder[2]);
    chain.pop_back();
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("insert pop mixed 2", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.end(), holder[0]);
    chain.insert(chain.end(), holder[1]);
    chain.insert(chain.end(), holder[2]);
    chain.pop_back();
    chain.pop_back();
    chain.pop_back();
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("insert pop mixed 3", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3), new Node(4), new Node(5), new Node(6)};
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.end(), holder[0]);
    chain.insert(chain.end(), holder[1]);
    chain.insert(chain.end(), holder[2]);
    chain.pop_back();
    chain.pop_back();
    chain.pop_back();
    REQUIRE(chain.empty());
    chain.insert(chain.end(), holder[3]);
    chain.insert(chain.end(), holder[4]);
    chain.insert(chain.end(), holder[5]);
    chain.pop_back();
    chain.pop_back();
    chain.pop_back();
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("pop back", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain = {new Node(10), new Node(11), new Node(12)};
    chain.pop_back();
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10, 11}));
}

TEST_CASE("pop back too many elements", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    chain.pop_back();
    chain.pop_back();
    chain.pop_back();
    chain.pop_back();
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("pop back on empty chain", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    chain.pop_back();
    REQUIRE(chain.empty());
}

TEST_CASE("pop front", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain = {new Node(10), new Node(11), new Node(12)};
    chain.pop_front();
    REQUIRE((to_vector<int>(chain) == std::vector<int>{11, 12}));
}

TEST_CASE("pop front on empty chain", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    chain.pop_front();
    REQUIRE(chain.empty());
}

TEST_CASE("clear on empty chain", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain;
    chain.clear();
    REQUIRE(chain.empty());
}

TEST_CASE("clear one element chain", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    chain.clear();
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("clear more than one element chain", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    chain.clear();
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("initializer list", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain = {new Node(10), new Node(11), new Node(12)};
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10, 11, 12}));
}

TEST_CASE("prev from the end", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain = {new Node(10)};
    REQUIRE((*std::prev(chain.end()))->value == 10);
}

TEST_CASE("begin", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain = {new Node(10)};
    REQUIRE((*chain.begin())->value == 10);
}

TEST_CASE("next", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain = {new Node(10), new Node(11), new Node(12)};
    REQUIRE((*std::next(chain.begin()))->value == 11);
}

TEST_CASE("clone", "[panda-event][intrusive_chain]") {
    IntrusiveChain<NodeSP> chain1 = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain2;

    // check clone first
    chain2 = chain1.clone();
    
    REQUIRE((to_vector<int>(chain1) == std::vector<int>{1, 2, 3}));
    REQUIRE((to_vector<int>(chain2) == std::vector<int>{1, 2, 3}));

    // than check references    
    std::vector<NodeSP> holder1;
    std::transform(chain1.begin(), chain1.end(), std::back_inserter(holder1), [](const NodeSP& node) {return node;});
    
    REQUIRE(holder1.size() == 3);

    chain1.clear();
    REQUIRE(chain1.empty());
    REQUIRE(check_references(holder1));

    std::vector<NodeSP> holder2;
    std::transform(chain2.begin(), chain2.end(), std::back_inserter(holder2), [](const NodeSP& node) {return node;});
    
    REQUIRE(holder2.size() == 3);

    chain2.clear();
    REQUIRE(chain2.empty());
    REQUIRE(check_references(holder2));
}

TEST_CASE("find element", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    auto pos = std::find_if(chain.cbegin(), chain.cend(), [](const NodeSP& node) { return node->value == 2; });
    REQUIRE(pos != chain.end());
}

TEST_CASE("erase middle element", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    chain.erase(std::find_if(chain.cbegin(), chain.cend(), [](const NodeSP& node) { return node->value == 2; }));
    REQUIRE((to_vector<int>(chain) == std::vector<int>{1, 3}));
    chain.erase(chain.begin());
    chain.erase(chain.begin());
    REQUIRE(check_references(holder));
}

TEST_CASE("erase elements with begin", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    chain.erase(chain.begin());
    chain.erase(chain.begin());
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("erase elements with end", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    chain.erase(std::prev(chain.end()));
    chain.erase(std::prev(chain.end()));
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("erase all insert all", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2)};
    IntrusiveChain<NodeSP> chain;
    std::copy(holder.begin(), holder.end(), std::back_inserter(chain));
    chain.erase(std::prev(chain.end()));
    chain.erase(std::prev(chain.end()));
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
    chain.insert(chain.end(), new Node(10));
    chain.insert(chain.end(), new Node(11));
    chain.insert(chain.end(), new Node(12));
    REQUIRE((to_vector<int>(chain) == std::vector<int>{10, 11, 12}));
}

TEST_CASE("erase insert", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(11), new Node(12)};
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.end(), holder[0]);
    chain.insert(chain.end(), holder[1]);
    REQUIRE((to_vector<int>(chain) == std::vector<int>{1, 2}));
    chain.erase(std::prev(chain.end()));
    chain.insert(chain.end(), holder[2]);
    chain.erase(chain.begin());
    chain.insert(chain.end(), holder[3]);
    REQUIRE((to_vector<int>(chain) == std::vector<int>{11, 12}));
    chain.erase(chain.begin());
    chain.erase(chain.begin());
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}

TEST_CASE("replace element", "[panda-event][intrusive_chain]") {
    std::vector<NodeSP>    holder = {new Node(1), new Node(2), new Node(3), new Node(99)};
    IntrusiveChain<NodeSP> chain;
    chain.insert(chain.end(), holder[0]);
    chain.insert(chain.end(), holder[1]);
    chain.insert(chain.end(), holder[2]);
    chain.insert(chain.erase(std::find_if(chain.cbegin(), chain.cend(), [](const NodeSP& node) { return node->value == 2; })), holder[3]);
    REQUIRE((to_vector<int>(chain) == std::vector<int>{1, 99, 3}));
    chain.erase(chain.begin());
    chain.erase(chain.begin());
    chain.erase(chain.begin());
    REQUIRE(chain.empty());
    REQUIRE(check_references(holder));
}
