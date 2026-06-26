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



template<typename K, typename V>
size_t HashMap<K, V>::hash(const K& key) const noexcept {
    size_t h =0;
    for(char c: key) {
        h = h * 31 + static_cast<size_t>(c);
    }
    return h;
}

template<typename K, typename V>
void HashMap<K, V>::rehash() {
    size_t newBucketCount = bucketCount * 2;
    Node** newBuckets = new Node*[newBucketCount]();

    for(size_t i=0; i<bucketCount; ++i) {
        Node* current = buckets[i];
        while(current != nullptr) {
            Node* next = current->next;
            size_t newIndex = hash(current->key) % newBucketCount;

            current->next = newBuckets[newIndex];
            newBuckets[newIndex] = current;

            current = next;
        }
    }
    delete[] buckets;
    buckets = newBuckets;
    bucketCount = newBucketCount;
}

template<typename K, typename V>
void HashMap<K, V>::set(const K& key, const V& value) {
    size_t index = hash(key) % bucketCount;

    Node* current = buckets[index];
    while(current != nullptr) {
        if(current->key == key) {
            current->value = value;
            return ;
        }
        current = current->next;

    }
    Node* newNode = new Node(key, value);
    newNode->next = buckets[index];
    buckets[index] = newNode;
    ++elementCount;

    if(loadFactor() > 0.75f) {
        rehash();
    }
}

template<typename K, typename V>
V HashMap<K, V>::get(const K& key) const {
    size_t index = hash(key) % bucketCount;
    Node* current = buckets[index];

    while(current != nullptr) {
        if(current->key == key) {
            return current->value;
        }
        current= current->next;
    }
    throw std::out_of_range("Key not found in HashMap");
}

template<typename K, typename V>
void HashMap<K, V>::copyFrom(const HashMap& other) {
    for(size_t i = 0; i < other.bucketCount; ++i) {
        Node* current = other.buckets[i];
        while(current != nullptr) {
            set(current->key, current->value);
            current = current->next;
        }
    }
}

template<typename K, typename V>
void HashMap<K, V>::clear() {
    for(size_t i=0; i<bucketCount; ++i){
        Node* current = buckets[i];
        while(current!=nullptr){
            Node* next = current->next;
            delete current;
            current = next;
        }
        buckets[i] = nullptr;
    }
    elementCount = 0;
}

template<typename K, typename V>
float HashMap<K, V>::loadFactor() const noexcept {
    return static_cast<float>(elementCount) / static_cast<float>(bucketCount);
}



template<typename K, typename V>
void HashMap<K, V>::remove(const K& key) {
    size_t index = hash(key) % bucketCount;
    Node* current = buckets[index];
    Node* prev = nullptr;

    while(current != nullptr) {
        if(current->key == key) {
            if(prev == nullptr) {
                buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }
            delete current;
            --elementCount;
            return;
        }
        prev = current;
        current = current->next;
    }
    throw std::out_of_range("Key not found in HashMap");
}

template<typename K, typename V>
bool HashMap<K, V>::contains(const K& key) const {
    size_t index = hash(key) % bucketCount;
    Node* current = buckets[index];

    while(current != nullptr) {
        if(current->key == key){
            return true;
        }
        current = current -> next;
    }
    return false;
}

template<typename K, typename V>
size_t HashMap<K, V>::size() const noexcept {
    return elementCount;
}

template<typename K, typename V>
bool HashMap<K, V>::empty() const noexcept {
    return elementCount == 0;
}
