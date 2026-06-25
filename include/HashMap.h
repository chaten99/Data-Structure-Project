#ifndef HASHMAP_H
#define HASHMAP_H

#include<cstddef>
#include <stdexcept>

template<typename K, typename V>
class HashMap {
    private:
        struct Node{
            K key;
            V value;
            Node* next;

            Node(const K& k, const V& v): key(k), value(v), next(nullptr) {}
        };

        Node** buckets;
        size_t bucketCount;
        size_t elementCount;

        size_t hash(const K& key) const noexcept;
        void rehash();

        void copyFrom(const HashMap& other);
    public:
        HashMap();
        HashMap(const HashMap& other);
        HashMap& operator=(const HashMap& other);
        ~HashMap() noexcept;

        void set(const K& key, const V& value);
        V get(const K& key) const;
        void remove(const K& key);

        bool contains(const K& key) const;
        size_t size() const noexcept;
        float loadFactor() const noexcept;
        void clear();

        bool empty() const noexcept;
};

#include "../src/HashMap.cpp"

#endif
