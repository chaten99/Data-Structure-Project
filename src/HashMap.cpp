#include "HashMap.h"
#include <functional>
#include <stdexcept>

template<typename K, typename V>
HashMap<K, V>::HashMap() : bucketCount(16), elementCount(0) {
    buckets = new Node*[bucketCount]();
}

template<typename K, typename V>
HashMap<K, V>::HashMap(const HashMap& other) : bucketCount(other.bucketCount), elementCount(0) {
    buckets = new Node*[bucketCount]();
    copyFrom(other);
}

template<typename K, typename V>
HashMap<K, V>& HashMap<K, V>::operator=(const HashMap& other){
    if(this != &other) {
        clear();
        delete[] buckets;

        bucketCount = other.bucketCount;
        elementCount = 0;
        buckets = new Node*[bucketCount]();

        copyFrom(other);
    }
    return *this;
}


template<typename K, typename V>
HashMap<K, V>::~HashMap() noexcept {
    clear();
    delete[] buckets;
    buckets = nullptr;
}
