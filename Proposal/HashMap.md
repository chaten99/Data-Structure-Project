# Hash Map — Design Document

---

## Table of Contents

1. [Introduction](#1-introduction)
   - 1.1 [What is a Hash Map](#11-what-is-a-hash-map)
   - 1.2 [Why Hash Maps Exist](#12-why-hash-maps-exist)
   - 1.3 [Real-World Use Cases](#13-real-world-use-cases)
2. [Design Evolution](#2-design-evolution)
   - 2.1 [Attempt 1 — Direct Array Indexing](#21-attempt-1--direct-array-indexing)
   - 2.2 [Attempt 2 — Open Addressing (Linear Probing)](#22-attempt-2--open-addressing-linear-probing)
   - 2.3 [Attempt 3 — Separate Chaining](#23-attempt-3--separate-chaining)
3. [Internal Architecture](#3-internal-architecture)
   - 3.1 [The Node Structure](#31-the-node-structure)
   - 3.2 [Member Variables](#32-member-variables)
   - 3.3 [Ownership Semantics](#33-ownership-semantics)
   - 3.4 [Class Invariants](#34-class-invariants)
4. [Memory Layout](#4-memory-layout)
5. [The Hash Function — Mathematics](#5-the-hash-function--mathematics)
   - 5.1 [Polynomial Rolling Hash](#51-polynomial-rolling-hash)
   - 5.2 [Why Base 31](#52-why-base-31)
   - 5.3 [Why Modulo bucketCount](#53-why-modulo-bucketcount)
   - 5.4 [Collision Inevitability — The Pigeonhole Principle](#54-collision-inevitability--the-pigeonhole-principle)
6. [Load Factor and Rehashing](#6-load-factor-and-rehashing)
   - 6.1 [Load Factor Definition](#61-load-factor-definition)
   - 6.2 [Why α = 0.75 is the Threshold](#62-why-α--075-is-the-threshold)
   - 6.3 [Rehashing Algorithm](#63-rehashing-algorithm)
   - 6.4 [Amortized O(1) Proof for set()](#64-amortized-o1-proof-for-set)
7. [Separate Chaining — The Collision Strategy](#7-separate-chaining--the-collision-strategy)
8. [Constructors and Object Lifetime](#8-constructors-and-object-lifetime)
   - 8.1 [Default Constructor](#81-default-constructor)
   - 8.2 [Copy Constructor](#82-copy-constructor)
   - 8.3 [Copy Assignment Operator](#83-copy-assignment-operator)
   - 8.4 [Destructor](#84-destructor)
9. [Rule of Three](#9-rule-of-three)
10. [Public API](#10-public-api)
11. [Function-by-Function Deep Dive](#11-function-by-function-deep-dive)
    - 11.1 [set](#111-set)
    - 11.2 [get](#112-get)
    - 11.3 [remove](#113-remove)
    - 11.4 [contains](#114-contains)
    - 11.5 [clear](#115-clear)
    - 11.6 [size / empty / loadFactor](#116-size--empty--loadfactor)
12. [Time Complexity Analysis](#12-time-complexity-analysis)
13. [Complexity Proofs](#13-complexity-proofs)

---

## 1. Introduction

### 1.1 What is a Hash Map

A hash map (also called a hash table or dictionary) is a data structure that stores **key-value pairs** and provides expected O(1) insertion, lookup, and deletion. It achieves this by using a **hash function** to transform a key into an integer index into an internal array of **buckets**. Collisions — two keys mapping to the same bucket — are resolved via **separate chaining**: each bucket holds a linked list of nodes, and colliding keys become nodes in the same list.

At the hardware level, the hash map is a `Node**` — a pointer to a heap-allocated array of `Node*` pointers. Most bucket slots hold `nullptr` (empty). Occupied slots hold a pointer to the head of a short linked list of key-value nodes. The map also tracks `bucketCount` (the array size), `elementCount` (total key-value pairs), and uses their ratio (the **load factor**) to decide when to grow.

### 1.2 Why Hash Maps Exist

Every other sequence structure pays O(N) to search: a sorted array achieves O(log N) with binary search, but insertion is still O(N). A hash map breaks the barrier by avoiding comparison-based search entirely — it computes **where** a key should live in O(1) and goes directly there.

| Operation | Sorted Array | BST (balanced) | Hash Map (avg) |
|:---|:---|:---|:---|
| Search | O(log N) | O(log N) | **O(1)** |
| Insert | O(N) | O(log N) | **O(1) amortized** |
| Delete | O(N) | O(log N) | **O(1)** |
| Ordered iteration | O(N) | O(N) | ✗ (unordered) |

The tradeoff: hash maps sacrifice ordering (keys have no sorted relationship) for speed. If you need "find the minimum key" or "iterate in sorted order," a balanced BST (like `std::map`) is better. If you just need fast lookup by key, a hash map dominates.

### 1.3 Real-World Use Cases

Hash maps are the default associative container in nearly every system:

- **Compilers**: Symbol tables mapping variable names to types and addresses
- **Databases**: Index structures for primary-key lookups
- **Language Runtimes**: Python's `dict`, JavaScript's objects, Java's `HashMap` — all hash maps
- **Caches**: Key = URL or resource ID, value = cached content
- **Graph algorithms**: Visited-node sets in BFS/DFS; adjacency maps for sparse graphs
- **Operating Systems**: Page table entries mapping virtual to physical addresses

---

## 2. Design Evolution

### 2.1 Attempt 1 — Direct Array Indexing

**Design**: Use the key directly as an array index: `data[key] = value`.

```cpp
V data[HUGE_SIZE];   // key is an integer used directly as index
data[42]  = "foo";
data[9999] = "bar";
```

**Problems**:

| Problem | Impact |
|:---|:---|
| **Keys must be integers** | Strings, objects, or compound keys are impossible |
| **Sparse keys waste memory** | Keys 0 and 999,999,999 require a billion-slot array; 999,999,998 slots are empty |
| **Fixed size** | Array size must be declared at compile time |

**Verdict**: Only works when keys are small, dense integers. Completely unusable for strings or sparse key spaces.

### 2.2 Attempt 2 — Open Addressing (Linear Probing)

**Design**: Apply a hash function to map any key to an index. On collision (two keys hashing to the same slot), scan forward linearly to find the next empty slot.

```
hash("alice") = 3  →  bucket[3] = "alice"
hash("bob")   = 3  →  bucket[3] taken, try bucket[4] = "bob"
hash("carol") = 4  →  bucket[4] taken, try bucket[5] = "carol"
```

**Problems**:

**Clustering**: Once a contiguous run of occupied slots forms, every new key that hashes anywhere into that run must probe past the entire cluster. As fill increases, average probe length grows from O(1) toward O(N).

**Tombstone problem**: Deleting a key cannot simply clear the slot — doing so breaks the probe chain. Any key that was displaced past the deleted slot becomes unreachable. The fix is "tombstone" markers (logically deleted slots that are skipped on lookup but count as occupied during insert). As tombstones accumulate, performance degrades and the table must periodically be rebuilt.

**Verdict**: Acceptable for read-heavy workloads with rare deletions, but fragile under mixed insert/delete patterns.

### 2.3 Attempt 3 — Separate Chaining

**Design**: The array holds `Node*` pointers (not values). Each bucket is the head of an independent singly linked list. Colliding keys append to the same list.

```
buckets[3] → ["alice","A"] → ["bob","B"] → nullptr
buckets[7] → ["carol","C"] → nullptr
buckets[0] → nullptr
```

**Why this works**:
- **Deletion is trivial**: unlinking a node from a linked list requires updating one pointer — no tombstones.
- **No clustering**: Each bucket's chain grows independently. A busy bucket does not affect neighbors.
- **Load factor above 1.0 is legal**: The array never needs to be "full" — every bucket can hold arbitrarily many nodes.
- **Chain length stays near constant**: When the load factor is kept below 0.75, the **expected** chain length at any bucket is ≤ 0.75 nodes, making every lookup O(1) in expectation.

---

## 3. Internal Architecture

### 3.1 The Node Structure

```cpp
struct Node {
    K key;
    V value;
    Node* next;
    Node(const K& k, const V& v);
};
```

**`K key`**: The key stored by value. Ownership belongs to the node — when the node is deleted, `~K()` is called automatically. Keys must support `operator==` (for chain search) and be hashable.

**`V value`**: The associated value, also stored by value inside the node.

**`Node* next`**: Singly linked list pointer to the next node in the same bucket's chain. `nullptr` if this is the last node. Only `next` is needed — separate chaining never needs to traverse backwards, so no `prev` pointer is required. This saves 8 bytes per node compared to a doubly linked list.

### 3.2 Member Variables

```cpp
Node** buckets;
size_t bucketCount;
size_t elementCount;
```

**`Node** buckets`**: A pointer to a heap-allocated array of `Node*`. Each `buckets[i]` is either `nullptr` (empty bucket) or the head of a collision chain. The double-pointer notation: the outer `*` says "this is a pointer," and the element type `Node*` says each slot holds a pointer.

**`bucketCount`**: The physical size of the `buckets` array. This is always a fixed value between rehashes, typically a power of two or a prime. The hash function maps every key to `[0, bucketCount)` via modulo.

**`elementCount`**: The total number of key-value pairs across all chains. `elementCount / bucketCount` = load factor α. Maintained incrementally (incremented on insert, decremented on remove) so `size()` is O(1).

### 3.3 Ownership Semantics

`HashMap` follows a **two-level ownership model**:

- The `HashMap` object **owns** the `buckets` array (heap-allocated via `new Node*[bucketCount]`).
- Each non-null `buckets[i]` is the head of a chain of `Node` objects. The `HashMap` owns the entire chain at each bucket (the head, and through `next` pointers, all subsequent nodes).
- Deleting the map requires traversing every chain and `delete`ing every `Node`, then `delete[]`ing the `buckets` array itself.

### 3.4 Class Invariants

```
Invariant 1:  elementCount == sum over all buckets of (chain length at bucket i)
Invariant 2:  0 <= bucketCount (buckets array is always allocated)
Invariant 3:  buckets != nullptr
Invariant 4:  For each chain, all nodes have hash(node->key) % bucketCount == bucket index
Invariant 5:  No two nodes in the entire map have key1 == key2 (keys are unique)
Invariant 6:  loadFactor() == (float)elementCount / (float)bucketCount
```

Invariant 4 is violated during `rehash()` — nodes are temporarily in "wrong" buckets as they are moved. The invariant is restored before `rehash()` returns.

---

## 4. Memory Layout

<p align="center">
    <img src="../Memory-Diagrams/HashMap-Memory Design.jpeg" alt="Memory Layout of HashMap-1" width="600">
    <img src="../Memory-Diagrams/HashMap-Memory Design (2).jpeg" alt="Memory Layout of HashMap-2" width="600">
</p>

The diagrams above show the complete two-level memory architecture of our `HashMap` implementation. Here is a detailed breakdown of every component:

**The `buckets` Array (First Level)**

`buckets` is a raw pointer to a contiguous, heap-allocated array of `Node*` values. With `bucketCount = 8`, this is 8 × 8 = 64 bytes of contiguous memory on a 64-bit system. Each slot is initialized to `nullptr`. This array lives on the heap and is the sole structure the `HashMap` object owns directly. The `HashMap` control structure on the stack (or inside another object) holds just three values: the pointer `buckets`, `bucketCount`, and `elementCount` — 24 bytes total.

**Individual Chains (Second Level)**

Each non-null `buckets[i]` points to the first `Node` in a singly linked list. Nodes in the same chain have different keys that happen to produce the same hash index. Each `Node` is independently heap-allocated and contains `key`, `value`, and `next` — three fields whose sizes depend on `K` and `V`. On a 64-bit system with `K = std::string` and `V = int`, a node occupies approximately 40 bytes (32 for `std::string` SSO buffer + 4 for `int` + 8 for `Node*` + padding).

**Before Rehash (Diagram 1)**

Shows a map with `bucketCount = 8` and several occupied buckets. Some buckets are `nullptr` (empty). One bucket shows a two-node chain — a collision: two different keys that hashed to the same index. The load factor `α = elementCount / 8` is below 0.75.

**After Rehash (Diagram 2)**

Shows the same map after `elementCount / bucketCount` exceeded 0.75. `bucketCount` has doubled to 16. Every existing node has been re-hashed with the new modulus — most nodes move to different bucket indices. The chains are shorter because the larger table spreads nodes more thinly. The old `buckets` array (8 slots) has been `delete[]`d.

**The Two-Level Pointer**

`Node**` is a pointer-to-pointer. `buckets` holds the address of the first `Node*` in the array. `buckets[i]` dereferences one level to get a `Node*`. `buckets[i]->key` dereferences again to get the key. An access like `get("alice")` therefore involves: one hash computation, one array index (O(1)), one or more `Node*` dereferences to walk the chain. This is why hash map lookups involve exactly the number of pointer dereferences equal to the chain length.

---

## 5. The Hash Function — Mathematics

### 5.1 Polynomial Rolling Hash

To convert any key of type `K` (typically `std::string`) into a bucket index, we use a **Polynomial Rolling Hash**. Given a string S of length L with characters S[0], S[1], ..., S[L−1]:

$$\text{Hash}(S) = \left(\sum_{i=0}^{L-1} S[i] \cdot 31^{L-1-i}\right) \bmod m$$

where m = `bucketCount`. Each character is treated as its ASCII integer value and multiplied by a power of the base (31). The sum is taken modulo m to produce a valid bucket index in `[0, m)`.

**Computed iteratively** via Horner's method: each loop iteration is equivalent to `h = h × 31 + c`, which accumulates the polynomial left-to-right using Horner's method: `((S[0]×31 + S[1])×31 + S[2])×31 + ...` — mathematically identical to the formula but requires only one multiply and one add per character.

### 5.2 Why Base 31

The base 31 is chosen because it is:

1. **Prime**: Primes reduce the risk of systematic patterns in keys aliasing to the same bucket. When the base shares a factor with m, many distinct inputs can produce the same hash.
2. **Odd**: Even bases can lose information about the highest-order bits of characters (for `char` values, even multiplication can zero out the LSB in the accumulated hash).
3. **Efficient**: `x × 31 = (x << 5) - x` — compilers optimize this to a shift-subtract, avoiding a division-speed multiply.
4. **Standard**: Base 31 is the hash used by `java.lang.String.hashCode()` and documented in *Effective Java* (Bloch, 2008). Its collision properties are well-studied empirically.

Other common choices: 37, 53, 97. All primes, all produce similar distribution quality.

### 5.3 Why Modulo bucketCount

The polynomial sum grows without bound as the string grows. For a 64-bit `size_t`, it wraps around at 2⁶⁴. After natural wraparound, we apply `% bucketCount` to map the result into `[0, bucketCount)`:

```
bucket_index = h % bucketCount
```

This guarantees a valid array index. However, modulo has an important implication: **when `bucketCount` changes (during rehash), every key's bucket index changes**. A key that lived in bucket 3 with `bucketCount = 8` may live in bucket 11 with `bucketCount = 16`. This is why rehashing must recompute `hash(key) % newBucketCount` for every single node.

### 5.4 Collision Inevitability — The Pigeonhole Principle

No hash function can eliminate collisions when the key space is larger than `bucketCount`. The **Pigeonhole Principle** states: if N items are placed into m containers and N > m, at least one container must hold more than one item.

In our map, there are 2^(8×L) possible strings of length L but only `bucketCount` buckets. For any m < 2^(8L), some two strings must map to the same bucket. Separate chaining accepts this and handles it gracefully — collisions just mean a slightly longer chain to traverse.

---

## 6. Load Factor and Rehashing

### 6.1 Load Factor Definition

The **load factor** α measures how full the hash map is:

$$\alpha = \frac{\text{elementCount}}{\text{bucketCount}}$$

α = 0 means the map is empty. α = 1.0 means on average every bucket has exactly one element. α = 2.0 means on average every bucket has two elements (every lookup traverses a chain of ~2 nodes).

For separate chaining, α directly controls the expected chain length at any given bucket. Under the **Simple Uniform Hashing Assumption** (each key is equally likely to hash to any bucket independently), the expected number of nodes in any one bucket is exactly α.

### 6.2 Why α = 0.75 is the Threshold

The choice of 0.75 is a deliberate engineering tradeoff between time and space:

- **α = 0.5**: Chains are very short (expected 0.5 nodes/bucket). Lookup is extremely fast, but half the allocated buckets are empty — 50% memory waste.
- **α = 0.75**: Expected 0.75 nodes/bucket — essentially O(1) lookup. Memory waste is ~25%. This is the sweet spot used by `java.util.HashMap`, Python's `dict`, and most industrial implementations.
- **α = 1.0**: Expected 1 node/bucket. Lookups are still O(1) in expectation, but variance is higher — some chains will have 3–4 nodes.
- **α = 2.0+**: Chains become long enough that the O(N) worst-case behavior starts to dominate the average, degrading practical performance significantly.

**Mathematical expectation of chain length at threshold α = 0.75**:

Under the uniform hashing assumption, the number of nodes in a given bucket follows a Poisson distribution with mean α. At α = 0.75:

$$P(\text{chain length} = k) = \frac{e^{-0.75} \cdot 0.75^k}{k!}$$

Expected chain length = 0.75. Probability of chain length ≥ 3 ≈ 3.3%. In practice, the vast majority of lookups examine at most 1–2 nodes — effectively O(1).

### 6.3 Rehashing Algorithm

When `elementCount / bucketCount > 0.75`, `rehash()` is triggered:

```
Step 1: newBucketCount = bucketCount × 2
Step 2: Allocate new array: Node** newBuckets = new Node*[newBucketCount]()
        (zero-initialized so all slots start as nullptr)
Step 3: For each old bucket i from 0 to bucketCount − 1:
            Walk the chain at buckets[i]:
                For each node n in the chain:
                    size_t newIndex = hash(n->key) % newBucketCount
                    Detach n from old chain (save n->next)
                    Prepend n to newBuckets[newIndex]   ← O(1) relink, no new allocation
Step 4: delete[] buckets           (free old array)
Step 5: buckets = newBuckets
Step 6: bucketCount = newBucketCount
```

**Critical optimization in Step 3**: Nodes are **relinked**, not reallocated. We do not call `new Node(...)` again — we take the existing node, update its `next` pointer, and prepend it to the new bucket's chain. This makes rehashing O(N) in time (N re-hashes + N pointer assignments) and O(1) in extra space (only the new bucket array is allocated; no new nodes).

### 6.4 Amortized O(1) Proof for set()

Rehashing costs O(N) when triggered. However, rehashing only occurs when the map doubles in size. This is the **same geometric growth argument** as `Vector::push_back`.

**Proof** (Aggregate / Accounting Method):

Let the initial bucket count be m₀ = 1, growth factor = 2. After k rehashes, `bucketCount = m₀ × 2^k`.

Rehash k occurs when `elementCount` reaches `0.75 × 2^k × m₀` elements. At rehash k, we move `0.75 × 2^k × m₀` nodes — call this cost C_k.

Total rehash cost for N insertions:

$$C_{\text{total}} = \sum_{k=0}^{\lfloor\log_2 N\rfloor} C_k \approx \sum_{k=0}^{\log_2 N} 0.75 \cdot 2^k = 0.75 \cdot (2^{\log_2 N + 1} - 1) < 1.5N$$

Total insertion cost (one hash + one allocation per element): N.

**Grand total cost**: C_total + N < 1.5N + N = 2.5N.

**Amortized cost per set()**:

$$T_{\text{amortized}} = \frac{2.5N}{N} = 2.5 = O(1) \quad \square$$

---

## 7. Separate Chaining — The Collision Strategy

Each bucket slot holds a **singly linked list** of nodes. A singly linked list (not doubly) suffices because:

1. All traversals are forward-only: we walk from `buckets[i]` through `->next` until we find the key or hit `nullptr`.
2. Deletion only requires finding the **predecessor** of the node to delete, then relinking `predecessor->next = toDelete->next`. No backward pointer is needed.
3. Saves 8 bytes per node vs a doubly linked list — at scale (millions of entries), this matters.

**Insert into a chain** (new key): Prepend to the head of the bucket's list — O(1). No traversal needed. The new node's `next` points to the old head; `buckets[i]` points to the new node.

**Lookup in a chain**: Traverse from `buckets[i]` comparing keys: `node->key == target`. Return `node->value` on match, or throw on `nullptr`. Expected steps = α ≤ 0.75.

**Delete from a chain**: Walk the chain maintaining a `prev` pointer. When `current->key == target`, set `prev->next = current->next` (or update `buckets[i]` if deleting the head), then `delete current`.

---

## 8. Constructors and Object Lifetime

### 8.1 Default Constructor



**Initial capacity = 16**: Chosen as a small power of two. Starting with 1 bucket would trigger rehash immediately. 16 allows ~12 elements before the first rehash (12/16 = 0.75). The `()` after `new Node*[...]` is essential — without it, the array contains garbage pointers instead of `nullptr`, and every bucket lookup would dereference an invalid address.

### 8.2 Copy Constructor



`copyFrom` traverses every bucket and every chain in `other`, calling `set(node->key, node->value)` for each node. This deep-copies every key-value pair into a brand-new, independent bucket array and node chain set.

**Complexity**: O(bucketCount + N) — O(bucketCount) to zero-initialize the new array, O(N) to copy N elements.

### 8.3 Copy Assignment Operator


**Self-assignment guard**: Without `if (this != &other)`, `clear()` would destroy all nodes, then `copyFrom(other)` would attempt to traverse destroyed chains — undefined behavior.

**Complexity**: O(M + N) where M = current element count (to clear) and N = other's element count (to copy).

### 8.4 Destructor



**Two-step cleanup**: `clear()` only walks the chains and `delete`s the nodes — it does not free `buckets`. The `delete[] buckets` line frees the array of pointers. Both steps are required; skipping either leaks memory.

**`noexcept`**: Destructors must not throw. All `delete` calls are guaranteed not to throw if destructors of `K` and `V` are `noexcept`, which is required for correctness by C++ RAII.

---

## 9. Rule of Three

`HashMap` manages two layers of heap resources: the `buckets` array and the `Node` chains. The compiler-generated defaults for copy and assignment both perform **memberwise copy**: they copy the `buckets` pointer value — two maps now sharing the same bucket array and all the same node chains. When either is destroyed, it calls `clear()` and `delete[] buckets`, freeing memory the other still points to. The other map then holds dangling pointers — every subsequent operation is undefined behavior (use-after-free), and the second destructor causes a double-free (heap corruption).

| Function | Responsibility |
|:---|:---|
| **Destructor** `~HashMap()` | Walk all chains, `delete` all nodes, `delete[]` buckets array |
| **Copy Constructor** `HashMap(const HashMap&)` | Allocate new bucket array, deep-copy all chains |
| **Copy Assignment** `operator=(const HashMap&)` | Clear + delete self, then allocate new array and deep-copy |

---

## 10. Public API

```cpp
template<typename K, typename V>
class HashMap {
private:
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v);
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
    [[nodiscard]] bool empty() const noexcept;
};
```

---

## 11. Function-by-Function Deep Dive

### 11.1 set

**Purpose**: Insert a new key-value pair, or update the value if the key already exists.

**Internal Algorithm**:
1. Compute `index = hash(key) % bucketCount`.
2. Walk the chain at `buckets[index]`. If a node with `node->key == key` is found, update `node->value` and return (no count change).
3. If not found: allocate `new Node(key, value)`, prepend it to `buckets[index]` (O(1) — new node's `next` = old head, `buckets[index]` = new node), increment `elementCount`.
4. If `loadFactor() > 0.75`, call `rehash()`.

**Memory Behaviour**: One `new Node(...)` per new key. Rehash allocates a new `Node*[2 × bucketCount]` array and relinks all existing nodes (no new node allocations during rehash).

**Best Case**: **O(1)** — empty bucket, no rehash.
**Worst Case**: **O(N)** — all N keys collide in one bucket (traversing entire chain), or rehash triggered.
**Amortized**: **O(1)** — proven in Section 6.4. ∎

### 11.2 get

**Purpose**: Return the value associated with a key. Throws `std::out_of_range` if the key does not exist.

**Internal Algorithm**:
1. `index = hash(key) % bucketCount`.
2. Walk `buckets[index]`'s chain. At each node, compare `node->key == key`.
3. Return `node->value` on match. If chain ends (`nullptr`), throw.

**Memory Behaviour**: No allocation or deallocation. Read-only traversal.

**Best Case**: **O(1)** — key is the first node in its bucket's chain.
**Worst Case**: **O(N)** — all N keys in one chain; target is last or absent.
**Average Case**: **O(1)** — expected chain length ≤ α ≤ 0.75 (constant). ∎

### 11.3 remove

**Purpose**: Delete the node with the given key. Throws `std::out_of_range` if not found.

**Internal Algorithm**:
1. `index = hash(key) % bucketCount`.
2. Walk the chain with a `prev` pointer (starts at `nullptr`) and `current = buckets[index]`.
3. On finding `current->key == key`:
   - If `prev == nullptr` (deleting head): `buckets[index] = current->next`.
   - Else: `prev->next = current->next`.
   - `delete current`, `--elementCount`, return.
4. Advance: `prev = current`, `current = current->next`. If `current` reaches `nullptr`, throw.

**Memory Behaviour**: One `delete` (frees one node). No reallocation. Note: this implementation does not shrink the bucket array on removal (a deliberate choice — shrinking would cost O(N) for rare gain).

**Best Case**: **O(1)** — key is head of its chain.
**Worst Case**: **O(N)** — long collision chain.
**Average Case**: **O(1)** — chain length ≤ α. ∎

### 11.4 contains

**Purpose**: Return `true` if the key exists in the map, `false` otherwise. Does not throw.

**Internal Algorithm**: Compute index, walk chain. Return `true` on match, `false` on chain exhaustion.

**Memory Behaviour**: None.

**Best / Worst / Average**: Same as `get` — O(1) average, O(N) worst. ∎

### 11.5 clear

**Purpose**: Remove all key-value pairs and reset all buckets to `nullptr`. The bucket array is retained (no deallocation of `buckets`).

**Internal Algorithm**:
```
For i = 0 to bucketCount − 1:
    current = buckets[i]
    while current != nullptr:
        next = current->next    // save before delete
        delete current
        current = next
    buckets[i] = nullptr
elementCount = 0
```

**Memory Behaviour**: N `delete` calls (one per node). The `buckets` array itself is not freed — capacity is preserved for reuse.

**Best Case**: **O(bucketCount)** — empty map (loop runs `bucketCount` times doing nothing).
**Worst Case**: **O(bucketCount + N)** — must iterate all buckets (even empty ones) and delete N nodes.
**Space Complexity**: O(1) — only frees memory. ∎

### 11.6 size / empty / loadFactor

**Purpose**:
- `size()`: Return `elementCount`.
- `empty()`: Return `elementCount == 0`.
- `loadFactor()`: Return `(float)elementCount / (float)bucketCount`.

**Internal Algorithm**: Each is a single arithmetic operation on stored members.

**Complexity**: **O(1)** for all three — no traversal, no allocation. ∎

---

## 12. Time Complexity Analysis

| Operation | Average Case | Worst Case | Structural Reason |
|:---|:---|:---|:---|
| `set()` | **O(1) amortized** | O(N) | Hash O(1), chain prepend O(1), rehash O(N) amortized away |
| `get()` | **O(1)** | O(N) | Chain length ≤ α ≤ 0.75 in expectation |
| `remove()` | **O(1)** | O(N) | Same chain traversal; delete + relink is O(1) |
| `contains()` | **O(1)** | O(N) | Identical to `get` without the return value |
| `clear()` | **O(bucketCount + N)** | O(bucketCount + N) | Must visit all buckets and delete all N nodes |
| `size()` | **O(1)** | O(1) | Returns stored counter |
| `empty()` | **O(1)** | O(1) | Compares stored counter to 0 |
| `loadFactor()` | **O(1)** | O(1) | One division of two stored members |
| Copy Constructor | **O(bucketCount + N)** | O(bucketCount + N) | Allocate array + copy N nodes |
| Copy Assignment | **O(M + N)** | O(M + N) | Clear M nodes + copy N nodes |
| Destructor | **O(bucketCount + N)** | O(bucketCount + N) | Clear all chains + free array |

---

## 13. Complexity Proofs

### set() — Amortized O(1)

Full proof in Section 6.4 via the Aggregate Method. Total cost over N insertions is bounded by 2.5N (insertion cost N + rehash cost < 1.5N). Amortized: 2.5N / N = O(1). ∎

### get() / contains() — O(1) average, O(N) worst

**Average case proof** (under Simple Uniform Hashing):

Let the map have N elements and `bucketCount = m`. Under uniform hashing, each element is equally likely to be in any bucket. The expected chain length at any bucket is N/m = α.

With rehashing maintaining α ≤ 0.75, the expected number of nodes inspected during `get()` is at most α + 1 ≤ 1.75 (the +1 accounts for the target node itself or the failed final comparison). Since 1.75 is a constant, expected lookup time is O(1). ∎

**Worst case proof**: If all N keys hash to the same bucket (adversarial input), `get()` traverses a chain of length N, performing N comparisons: O(N). ∎

### remove() — O(1) average, O(N) worst

**Proof**: `remove()` traverses the same chain as `get()` with an additional `prev` pointer. Chain length analysis is identical. After finding the node, relinking (`prev->next = current->next`) and `delete current` are both O(1). Expected total: O(α) = O(1). Worst case: O(N). ∎

### clear() — O(bucketCount + N)

**Proof**: The outer loop runs `bucketCount` iterations — O(bucketCount). For each non-null bucket, the inner loop runs `chain_length_i` iterations, deleting one node per iteration. Summing over all buckets: total inner iterations = Σ chain_length_i = N (every node is deleted exactly once). Each `delete` is O(1). Total: O(bucketCount) + O(N) = O(bucketCount + N).

For a well-maintained map with α ≤ 0.75: `N ≤ 0.75 × bucketCount`, so N = O(bucketCount), and O(bucketCount + N) = O(bucketCount). ∎

### Rehash — O(N)

**Proof**: `rehash()` visits every node exactly once (iterating all buckets, all chains). For each node: one `hash()` call (O(L) for string length L, treated as O(1) for fixed-bounded keys), one modulo, one pointer assignment (prepend to new chain). Total: N × O(1) = O(N). Allocating the new array: O(newBucketCount) = O(N) (since rehash doubles, and N ≈ 0.75 × oldBucketCount, so newBucketCount ≈ 1.33N). Grand total: O(N). ∎