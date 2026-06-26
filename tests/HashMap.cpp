#include <iostream>
#include <string>
#include <cassert>
#include <stdexcept>
#include "HashMap.h"

void testSetAndGet() {
    HashMap<std::string, int> map;
    map.set("Alice", 100);
    map.set("Bob", 200);

    assert(map.get("Alice") == 100);
    assert(map.get("Bob") == 200);

    map.set("Alice", 999);
    assert(map.get("Alice") == 999);
}

void testGetException() {
    HashMap<std::string, int> map;
    map.set("Real", 1);

    bool caught = false;
    try {
        map.get("Ghost");
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
}

void testRehashing() {
    HashMap<std::string, int> map;

    for (int i = 0; i < 25; ++i) {
        map.set("Key_" + std::to_string(i), i * 10);
    }

    for (int i = 0; i < 25; ++i) {
        assert(map.get("Key_" + std::to_string(i)) == i * 10);
    }
}

void testRuleOfThree() {
    HashMap<std::string, int> map1;
    map1.set("A", 1);
    map1.set("B", 2);

    HashMap<std::string, int> map2 = map1;
    assert(map2.get("A") == 1);
    assert(map2.get("B") == 2);

    map2.set("A", 99);
    assert(map1.get("A") == 1);
    assert(map2.get("A") == 99);

    HashMap<std::string, int> map3;
    map3.set("Junk", 0);
    map3 = map1;

    assert(map3.get("B") == 2);

    map3 = map3;
    assert(map3.get("B") == 2);
}

void testContains() {
    HashMap<std::string, int> map;
    map.set("Alpha", 10);

    assert(map.contains("Alpha"));
    assert(!map.contains("Beta"));
}

void testRemove() {
    HashMap<std::string, int> map;
    map.set("One", 1);
    map.set("Two", 2);
    map.set("Three", 3);

    map.remove("Two");
    assert(!map.contains("Two"));
    assert(map.contains("One"));
    assert(map.contains("Three"));

    bool caught = false;
    try {
        map.get("Two");
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        map.remove("Ghost");
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
}

void testClear() {
    HashMap<std::string, int> map;
    map.set("X", 100);
    map.set("Y", 200);

    map.clear();

    assert(!map.contains("X"));
    assert(!map.contains("Y"));

    bool caught = false;
    try {
        map.get("X");
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
}

int main() {
    testSetAndGet();
    testGetException();
    testRehashing();
    testRuleOfThree();
    testContains();
    testRemove();   
    testClear();

    std::cout << "All Phase 2 tests passed successfully!\n";
    return 0;
}