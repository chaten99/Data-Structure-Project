# Doubly Linked List — Design Document

---

## Table of Contents

1. [Introduction](#1-introduction)
   - 1.1 [What is a Doubly Linked List](#11-what-is-a-doubly-linked-list)
   - 1.2 [Why Linked Lists Exist](#12-why-linked-lists-exist)
   - 1.3 [Arrays vs Linked Lists](#13-arrays-vs-linked-lists)
   - 1.4 [Real-World Use Cases](#14-real-world-use-cases)
2. [Design Evolution](#2-design-evolution)
   - 2.1 [Attempt 1 — Singly Linked List](#21-attempt-1--singly-linked-list)
   - 2.2 [Attempt 2 — Doubly Linked List](#22-attempt-2--doubly-linked-list)
3. [Internal Architecture](#3-internal-architecture)
   - 3.1 [The Node Structure](#31-the-node-structure)
   - 3.2 [Member Variables](#32-member-variables)
   - 3.3 [Ownership Semantics](#33-ownership-semantics)
   - 3.4 [Class Invariants](#34-class-invariants)
4. [Memory Layout](#4-memory-layout)
5. [Pointer Mechanics](#5-pointer-mechanics)
   - 5.1 [What a Pointer Is](#51-what-a-pointer-is)
   - 5.2 [Node Linking — How the Chain Is Built](#52-node-linking--how-the-chain-is-built)
   - 5.3 [Why Scattered Memory is Fundamental](#53-why-scattered-memory-is-fundamental)
6. [Dynamic Allocation Per Node](#6-dynamic-allocation-per-node)
   - 6.1 [Why Each Node is Heap-Allocated](#61-why-each-node-is-heap-allocated)
   - 6.2 [new vs malloc for Nodes](#62-new-vs-malloc-for-nodes)
   - 6.3 [The Ownership Chain](#63-the-ownership-chain)
7. [Constructors and Object Lifetime](#7-constructors-and-object-lifetime)
   - 7.1 [Default Constructor](#71-default-constructor)
   - 7.2 [Initializer List Constructor](#72-initializer-list-constructor)
   - 7.3 [Copy Constructor](#73-copy-constructor)
   - 7.4 [Copy Assignment Operator](#74-copy-assignment-operator)
   - 7.5 [Destructor](#75-destructor)
8. [Rule of Three](#8-rule-of-three)
9. [Public API](#9-public-api)
10. [Traversal Optimization](#10-traversal-optimization)
    - 10.1 [Bidirectional Traversal](#101-bidirectional-traversal)
    - 10.2 [The N/2 Optimization — Mathematical Proof](#102-the-n2-optimization--mathematical-proof)
    - 10.3 [Why O(N/2) is Still O(N)](#103-why-on2-is-still-on)
11. [Pointer Relinking — The Core Operation](#11-pointer-relinking--the-core-operation)
    - 11.1 [Insert at Front (push_front)](#111-insert-at-front-push_front)
    - 11.2 [Insert at Back (push_back)](#112-insert-at-back-push_back)
    - 11.3 [Insert in the Middle](#113-insert-in-the-middle)
    - 11.4 [Remove at Front (pop_front)](#114-remove-at-front-pop_front)
    - 11.5 [Remove at Back (pop_back)](#115-remove-at-back-pop_back)
    - 11.6 [Remove in the Middle](#116-remove-in-the-middle)
12. [Function-by-Function Deep Dive](#12-function-by-function-deep-dive)
    - 12.1 [push_back](#121-push_back)
    - 12.2 [push_front](#122-push_front)
    - 12.3 [insert](#123-insert)
    - 12.4 [remove](#124-remove)
    - 12.5 [pop_back](#125-pop_back)
    - 12.6 [pop_front](#126-pop_front)
    - 12.7 [front / back](#127-front--back)
    - 12.8 [at](#128-at)
    - 12.9 [find](#129-find)
    - 12.10 [contains](#1210-contains)
    - 12.11 [reverse](#1211-reverse)
    - 12.12 [clear](#1212-clear)
    - 12.13 [size / empty](#1213-size--empty)
    - 12.14 [Copy Constructor](#1214-copy-constructor)
    - 12.15 [Copy Assignment Operator](#1215-copy-assignment-operator)
    - 12.16 [Destructor](#1216-destructor)
13. [Time Complexity Analysis](#13-time-complexity-analysis)
14. [Complexity Proofs](#14-complexity-proofs)
15. [Linked List vs Dynamic Array — When to Choose Which](#15-linked-list-vs-dynamic-array--when-to-choose-which)

---

## 1. Introduction

### 1.1 What is a Doubly Linked List

A doubly linked list is a dynamic, node-based sequence container in which each element is stored in its own independently heap-allocated **node**. Each node holds three things: the data element itself, a `next` pointer to the node that follows it in the sequence, and a `prev` pointer to the node that precedes it. The list itself maintains two sentinel pointers — `head` (pointing to the first node) and `tail` (pointing to the last node) — plus a `size` counter.

At the hardware level, a doubly linked list is a **non-contiguous** data structure. The nodes can live anywhere in memory — a node at address `0x1000` might link to a node at address `0x8FA0`. What makes them a logical sequence is only the pointer chain, not physical adjacency.

This project provides a custom, from-scratch implementation — `List<T>` — built using `new`/`delete` per-node memory management. It is designed as an educational deep-dive into the systems-level engineering behind linked lists: how the two-pointer design guarantees O(1) endpoints, why traversal is O(N), and the mathematical justification for every complexity claim.

### 1.2 Why Linked Lists Exist

The motivation for linked lists comes directly from the limitations of arrays. An array allocates one contiguous block of memory. This is excellent for random access (O(1) by index), but devastating for insertion and deletion at arbitrary positions — every element after the target must be physically shifted in memory.

A linked list trades random access for O(1) pointer relinking. Inserting a node between two existing nodes requires updating exactly four pointers, regardless of how many nodes are in the list. This makes linked lists the ideal structure whenever:

- Insertion and deletion at the ends dominate (queues, stacks, deques)
- The number of elements is unpredictable and grows/shrinks rapidly
- Splitting and merging sequences must be O(1)
- Stable node addresses are required (no reallocation)

### 1.3 Arrays vs Linked Lists

| Property | Dynamic Array | Doubly Linked List |
|:---|:---|:---|
| Memory layout | Contiguous | Scattered (per-node heap allocation) |
| Random access `at(i)` | **O(1)** (pointer + offset) | **O(N)** (must traverse) |
| Insert/remove at ends | O(1) amortized (array), O(1) strict (list) | **O(1) strict** |
| Insert/remove in middle | **O(N)** (shift elements) | O(N) to find + **O(1) to relink** |
| Cache performance | **Excellent** (contiguous, prefetchable) | **Poor** (pointer chasing = cache misses) |
| Memory per element | `sizeof(T)` | `sizeof(T) + 2 × sizeof(pointer)` (16 extra bytes on 64-bit) |
| Iterator stability | Invalidated on reallocation | **Stable** (node addresses never change) |
| No reallocation | No (periodic doubling) | **Yes** (each node allocated independently) |

The critical asymmetry: a dynamic array must shift N elements to insert at index 0 — O(N) work. A doubly linked list adjusts 4 pointers — O(1) work. But to find position `k` in the list requires traversing `k` nodes — O(N) work. To find position `k` in an array is a single multiply-and-add — O(1) work.

### 1.4 Real-World Use Cases

Doubly linked lists appear everywhere that O(1) end-operations or O(1) splicing matters:

- **Operating System Process Schedulers**: The Linux kernel's `task_struct` list uses an intrusive doubly linked list to manage runnable processes. Processes are added and removed from scheduling queues in O(1).
- **LRU Caches**: A doubly linked list combined with a hash map implements a Least Recently Used cache. Moving a node to the front on access is O(1) with stable pointers.
- **Text Editors**: Each line of text can be a node. Inserting a line between two others is O(1) pointer relinking, not O(N) array shifting.
- **Browser History**: Forward and backward navigation maps directly to `next` and `prev` pointers.
- **Music/Playlist Players**: The `next`/`prev` track buttons navigate a doubly linked playlist.
- **Undo/Redo Systems**: Each state is a node; undo walks `prev`, redo walks `next`.
- **Memory Allocators**: The free-list in many `malloc` implementations is a doubly linked list of free memory blocks.

---

## 2. Design Evolution

### 2.1 Attempt 1 — Singly Linked List

**Design**: Each node has only a `next` pointer. The list maintains only a `head` pointer (and optionally a `tail`).

```cpp
struct Node {
    T data;
    Node* next;
};
Node* head;
Node* tail;
size_t listSize;
```

**Operations that work well**:

`push_front` is O(1): allocate a new node, point its `next` at the current head, update head to the new node.

`push_back` with a `tail` pointer is O(1): allocate a new node, link `tail->next` to it, update `tail`.

`pop_front` is O(1): save `head->next`, delete head, update head.

**The Fatal Flaw — `pop_back` is O(N)**:

This is the defining failure of the singly linked list. To delete the last node, you must update `tail` to point to the second-to-last node. But with only `next` pointers, there is no direct way to reach the second-to-last node from `tail`.

The only solution is to traverse the entire list from `head`:



**Proof that `pop_back` is O(N) in a singly linked list**:

Let the list have N nodes: `n₀ → n₁ → n₂ → ... → n_{N-1}`. `tail` points to `n_{N-1}`. To delete `n_{N-1}`, we need a pointer to `n_{N-2}` (so we can set `n_{N-2}->next = nullptr`). Starting from `head = n₀`, reaching `n_{N-2}` requires exactly N−2 steps of `current = current->next`. Therefore, `pop_back` performs Θ(N) work — it is O(N) in all cases, not just the worst case.

| Operation | Singly Linked (with tail) | Problem |
|:---|:---|:---|
| `push_front` | O(1) | — |
| `push_back` | O(1) | — |
| `pop_front` | O(1) | — |
| `pop_back` | **O(N)** | Must traverse to find predecessor of tail |

**Verdict**: A singly linked list cannot be used as a stack (front-only), deque, or double-ended queue with full O(1) guarantees at both ends. The lack of backward linkage is a fundamental architectural deficiency.

### 2.2 Attempt 2 — Doubly Linked List

**Design**: Each node gets a second pointer — `prev` — pointing backward to its predecessor.

```cpp
struct Node {
    T data;
    Node* next;    // Forward link
    Node* prev;    // Backward link ← The fix
};

Node* head;
Node* tail;
size_t listSize;
```

Now `pop_back` is O(1):



`tail->prev` directly gives us the second-to-last node. No traversal required.

**Why the extra `prev` pointer solves the problem**:

Every node now knows its two neighbors in O(1). Given any node `p`, you can reach both `p->next` and `p->prev` with a single pointer dereference. The list is now **symmetrically navigable** — every operation that worked efficiently from `head` now works equally efficiently from `tail`.

**Cost of the fix**: Each node now requires 16 extra bytes (two 8-byte pointers on a 64-bit system) compared to storing only the data. For large `T` (e.g., a 1 KB struct), this overhead is negligible. For small `T` (e.g., `char`), the overhead is enormous (1600% memory overhead). This is a fundamental space tradeoff of doubly linked lists.

---

## 3. Internal Architecture

### 3.1 The Node Structure

```cpp
template<typename T>
struct Node {
    T data;
    Node* next;
    Node* prev;
    Node(const T& value);
};
```

**`T data`**: The user's element, stored by value inside the node. The node **owns** this data. When the node is deleted, the data's destructor is called automatically (because `Node` is a class with a proper destructor, and `delete node` calls `~Node()` which calls `~T()` on `data`).

**`Node* next`**: An 8-byte pointer (on 64-bit systems) holding the memory address of the next node in the sequence, or `nullptr` if this is the last node. Dereferencing this pointer (`node->next->data`) accesses the next element.

**`Node* prev`**: An 8-byte pointer holding the memory address of the previous node, or `nullptr` if this is the first node. This is the key addition over a singly linked list.

**Constructor**: Initializes `next` and `prev` to `nullptr`. A newly constructed node is always isolated — it must be explicitly linked into the list by the insertion function.

### 3.2 Member Variables

The `List<T>` class contains exactly three private data members:

```cpp
template<typename T>
class List {
private:
    Node* head;
    Node* tail;
    size_t listSize;
};
```

**`Node* head`**: Points to the first node. All forward traversals begin here. For an empty list, `head == nullptr`. When a node is inserted at the front, `head` is updated to point to the new node. When the first node is removed, `head` is updated to `head->next`.

**`Node* tail`**: Points to the last node. All backward traversals (and `pop_back`, `push_back`) begin here. For an empty list, `tail == nullptr`. The `tail` pointer is what makes both-end O(1) operations possible — without it, every `push_back` would need to traverse to the end.

**`size_t listSize`**: Maintains an exact count of nodes. This allows `size()` to be O(1) without traversal. It is incremented on every successful insertion and decremented on every successful removal.

**Total size of the `List<T>` control structure**: On a 64-bit system, `head` (8 bytes) + `tail` (8 bytes) + `listSize` (8 bytes) = **24 bytes**. The list's control structure is tiny regardless of how many nodes it manages.

### 3.3 Ownership Semantics

`List<T>` follows the **chain ownership model**:

- `List` owns `head`.
- Each `Node` owns `node->next`.
- Each `Node` owns its `data`.

This means ownership flows through the `next` pointer chain. To destroy the list, you delete `head`, which frees the first node, and must then manually traverse and delete every remaining node (since C++ does not recursively call destructors of `next` — you must do it in the destructor loop).

The `prev` pointers are **non-owning back-references** — they point to nodes owned by their predecessor. Never `delete` via `prev`.

### 3.4 Class Invariants

The following invariants must hold at all times after construction and between any two public method calls:

```
Invariant 1:  listSize == 0 ⟺ head == nullptr ⟺ tail == nullptr
Invariant 2:  listSize == 1 ⟺ head == tail ⟺ head->next == nullptr ⟺ head->prev == nullptr
Invariant 3:  listSize ≥ 2 ⟹ head != tail
Invariant 4:  head->prev == nullptr (head has no predecessor)
Invariant 5:  tail->next == nullptr (tail has no successor)
Invariant 6:  For every interior node p: p->prev->next == p and p->next->prev == p
Invariant 7:  listSize equals the number of nodes reachable by following next from head
```

Invariant 6 is the most important and the most commonly violated during buggy insertion/deletion. Every relinking operation must maintain bidirectional consistency: if you point A's `next` to B, you **must** also point B's `prev` back to A.

---

## 4. Memory Layout

Unlike a dynamic array, whose elements live contiguously in a single heap block, a doubly linked list's nodes are **individually heap-allocated** and can live anywhere in the address space.

```
List<int> control structure (24 bytes on stack or heap):
┌──────────────┐
│ head ────────┼───→ Node at 0x1A20
│ tail ────────┼───────────────────────────────────────────────────────────────┐
│ listSize: 4  │                                                               │
└──────────────┘                                                               │
                                                                               │
Heap (nodes scattered across memory):                                          │
                                                                               │
  0x1A20               0x7C40               0x3F80               0x9B10       │
┌─────────────┐      ┌─────────────┐      ┌─────────────┐      ┌─────────────┤
│ data: 10    │      │ data: 20    │      │ data: 30    │      │ data: 40    ││
│ prev: null  │      │ prev: 0x1A20│      │ prev: 0x7C40│      │ prev: 0x3F80│
│ next: 0x7C40│      │ next: 0x3F80│      │ next: 0x9B10│      │ next: null  ││
└─────────────┘      └─────────────┘      └─────────────┘      └─────────────┤
     head                                                              tail   ←┘
```

**Key observations about this layout**:

**Non-contiguity**: The node addresses (0x1A20, 0x7C40, 0x3F80, 0x9B10) bear no arithmetic relationship to each other. They are wherever `new` happened to find free memory at the time of each allocation. To go from node 1 to node 2, you cannot add `sizeof(Node)` to the address — you must dereference the `next` pointer.

**Pointer chasing**: Every traversal step `current = current->next` is a pointer dereference to a (potentially) cold cache line. On modern CPUs, a cache miss costs 50–300 clock cycles. For a list of 1,000,000 nodes, `at(500000)` incurs ~500,000 potential cache misses — compared to `operator[](500000)` in a vector, which has excellent sequential prefetch behavior.

**Node size overhead**: Each `Node<T>` uses `sizeof(T) + sizeof(Node*) + sizeof(Node*)` = `sizeof(T) + 16` bytes. For `T = int` (4 bytes): 20 bytes per node, a 400% overhead. The `data` field may also require padding for alignment. For `T = double` (8 bytes): 24 bytes per node (with alignment), a 200% overhead.

**Independent lifetimes**: Each node has its own independent lifetime — it is created by `new` and destroyed by `delete`. There is no "buffer" to reallocate. A list of 1,000,000 nodes has performed 1,000,000 separate calls to `new` and will perform 1,000,000 calls to `delete`.

---

## 5. Pointer Mechanics

### 5.1 What a Pointer Is

A pointer is a variable that stores a **memory address**. On a 64-bit system, every pointer is exactly 8 bytes — it holds a 64-bit number that is the address of some location in the virtual address space.

```
Node* p = new Node(42);

p:    [0x0000'7FFF'0000'1A20]    ← 8 bytes holding an address
                    ↓
            Memory at 0x1A20:
            [42][0x0000...0000][0x0000...0000]
             ↑data              ↑next=null      ↑prev=null
```

When you write `p->data`, the CPU:
1. Reads the 8-byte address stored in `p` (value: `0x1A20`)
2. Goes to that address in memory
3. Reads `sizeof(T)` bytes from there

When you write `p->next`, the CPU:
1. Reads `p` to get `0x1A20`
2. Goes to `0x1A20 + sizeof(T)` (the offset of `next` within the struct)
3. Reads 8 bytes → this is another address

This is **pointer chasing** — each dereference follows an address to another address. The doubly linked list is a chain of such indirections.

### 5.2 Node Linking — How the Chain Is Built

When `push_back(value)` is called:

```
Before (list has one node):
head → [10 | next=null | prev=null] ← tail

After push_back(20):
head → [10 | next=→ | prev=null]    [20 | next=null | prev=→ ]
                    ↘                ↗                        ↑
                     ────────────────                       tail
```

In code:



Exactly **4 pointer assignments** and **1 allocation**. No element copying. No traversal. The chain grows by adding one new node and updating the minimal set of pointers.

### 5.3 Why Scattered Memory is Fundamental

The scattered memory layout is not a bug — it is the direct consequence of allocating each node independently, which is itself the feature that enables O(1) insertion anywhere. If the nodes were contiguous (like an array), inserting in the middle would require shifting. The scattered layout is the price of structural flexibility.

This is the core tension of linked lists vs arrays:

- **Array**: O(1) random access (arithmetic addressing), O(N) middle insertion (physical shifting)
- **Linked List**: O(N) random access (pointer chasing), O(1) middle insertion (pointer relinking — given the node pointer)

---

## 6. Dynamic Allocation Per Node

### 6.1 Why Each Node is Heap-Allocated

Each node must be heap-allocated because:

1. **Unknown count at compile time**: The list grows and shrinks at runtime. Nodes cannot be stack-allocated because their number is not known, and stack-allocating in a function and storing a pointer to it would create a dangling pointer when the function returns.

2. **Independent lifetimes**: Each node has its own creation and destruction event. Heap allocation provides this granular lifetime control.

3. **Stable addresses**: Unlike a dynamic array (which moves all elements to a new buffer during reallocation), a linked list node, once created, **never moves**. Its address is stable for its entire lifetime. This means you can safely hold a `Node*` to a list element indefinitely — it will not be invalidated by any insertion or deletion elsewhere in the list (as long as that specific node is not itself removed).

### 6.2 new vs malloc for Nodes

Unlike `Vector<T>` which uses `malloc` + placement `new` to separate allocation from construction, `List<T>` uses standard `new` for nodes:



This is appropriate because for a node, **allocation and construction always happen together**. There is no "reserve capacity" concept — a node exists only because an element is being stored in it. We never want uninitialized node slots. Therefore, the two-phase separation that `malloc` + placement `new` provides is unnecessary overhead here.

Correspondingly, destruction uses standard `delete`:



### 6.3 The Ownership Chain

```
List         head
 │            │
 └────────────→ Node₀ ──next──→ Node₁ ──next──→ Node₂ ──next──→ nullptr
                  ↑                ↑                ↑
                  │    prev        │    prev        │
                nullptr ←──────── ─│←────────────── │←──────────── ...
```

The `List` object owns `head` (and through it, the whole chain). The `tail` pointer is a **non-owning alias** to the last node — it exists only for O(1) tail access and is never used to delete memory.

**Memory leak scenario**: If you lose the `head` pointer (e.g., by overwriting it without first deleting the chain), every node becomes unreachable — a memory leak proportional to N nodes.

**Double-free scenario**: If two `List` objects share the same `head` (shallow copy), both will delete the same nodes in their destructors — a double-free and heap corruption.

These are the exact bugs the **Rule of Three** (Section 8) prevents.

---

## 7. Constructors and Object Lifetime

### 7.1 Default Constructor



**Design decisions**:

- **`head = nullptr`, `tail = nullptr`**: The list starts empty. No nodes are allocated. This satisfies Invariant 1: `listSize == 0 ⟺ head == nullptr ⟺ tail == nullptr`.
- **No allocation**: Unlike `Vector<T>`, which always allocates at least 1 slot in the default constructor, `List<T>` allocates nothing. An empty list is truly free — no heap memory is consumed until the first element is inserted.
- **O(1)**: Three pointer/integer assignments. Constant time regardless of any context.

### 7.2 Initializer List Constructor



This enables brace initialization:

```cpp
List<int> lst = {10, 20, 30, 40};
```

**Algorithm**: Initialize to empty state, then call `push_back` for each element in the initializer list. Each `push_back` allocates one node and links it at the tail — O(1) per element.

**Total complexity**: O(N) where N is the size of the initializer list — N node allocations, N pointer relinkings.

**Exception safety**: If any `push_back` throws (e.g., `new` throws `std::bad_alloc` for a large list, or `T`'s copy constructor throws), the destructor of the partially-constructed `List` will be called, which traverses and deletes all nodes allocated so far. No memory leaks.

### 7.3 Copy Constructor


The copy constructor creates a **deep copy**: an entirely independent chain of nodes containing the same values in the same order. After the copy:

```
BEFORE:  other.head → [A]→[B]→[C]→null      (other owns these 3 nodes)

AFTER:   other.head → [A]→[B]→[C]→null      (other still owns its nodes)
         this->head → [A]→[B]→[C]→null      (this owns 3 NEW nodes, different addresses)
```

Modifying `this` (inserting, removing) does not affect `other`, and vice versa.

**A shallow copy would be catastrophic**: If we simply did `head = other.head; tail = other.tail; listSize = other.listSize;`, both lists would share the same nodes. When the first list is destroyed, it would `delete` all shared nodes. The second list's `head` and `tail` would then be dangling pointers. Any access — even the destructor — would be undefined behavior (use-after-free).

**Complexity**: O(N) — N nodes allocated, N pointer assignments per `push_back`.

### 7.4 Copy Assignment Operator


Unlike `Vector<T>`'s assignment (which may reuse the existing buffer), a linked list cannot reuse its existing nodes for a different list — the node structure itself is the allocation unit. The operation is:

1. **Destroy** all existing nodes via `clear()` — O(M) where M is the current size
2. **Copy** all nodes from `other` via `copyFrom()` — O(N) where N is `other.listSize`

**Total complexity**: O(M + N) = O(max(M, N)) = O(N).

**Self-assignment guard** (`if (this != &other)`): Without this, `clear()` would destroy all nodes, then `copyFrom(other)` would try to traverse a destroyed list — undefined behavior.

**Order matters for exception safety**: `clear()` runs first and cannot fail (destructors are `noexcept`). If `copyFrom` throws mid-way (e.g., `new` fails), the `List` will be in a valid but partially-filled state — not the original state, but also not corrupt (all partially-allocated nodes will be cleaned up by the `List`'s destructor since they've been properly linked).

### 7.5 Destructor



**Critical subtlety**: `next` must be saved **before** `delete current`. Once `delete current` executes, `current` points to freed memory — any dereference including `current->next` is undefined behavior.

**Why `delete` calls `~T()`**: `delete node` calls `Node::~Node()`. Since `Node` contains `T data` as a member, `~Node()` calls `data.~T()` automatically (C++ destructs all data members when a class is destroyed). This is safe — it means we don't need explicit destructor calls like `Vector<T>` does with placement `new`.

**`noexcept`**: The destructor is marked `noexcept` because destructors must not throw. If a destructor throws during stack unwinding (i.e., while handling another exception), `std::terminate()` is called. All `Node` destructors must also be `noexcept`, which is satisfied as long as `~T()` is `noexcept` (which it should be for any well-behaved type).

**Complexity**: O(N) — every node is visited exactly once.

---

## 8. Rule of Three

The **Rule of Three** states: if a class defines any one of the following, it must define all three.

| Function | What It Does | What Happens Without It |
|:---|:---|:---|
| **Destructor** `~List()` | Traverses and `delete`s every node | Memory leak — N nodes allocated, none freed |
| **Copy Constructor** `List(const List&)` | Deep-copies every node | Compiler generates shallow copy → pointer aliasing → double-free on destruction |
| **Copy Assignment** `operator=(const List&)` | Clears self, deep-copies other | Compiler generates shallow assignment → same aliasing bugs |

`List<T>` manages heap memory (the node chain). The compiler-generated defaults for copy constructor and copy assignment both perform **memberwise copy**: they copy the pointer values (`head`, `tail`) but not the pointed-to data. This creates two `List` objects with identical `head` and `tail` pointers — they share ownership of the same node chain. When either is destroyed, it deletes all nodes. The other list then has dangling pointers. Accessing any element — or even just running the destructor — is undefined behavior.

The three functions are implemented as:

```
~List()              → clear() → traverse and delete every node
List(const List&)    → copyFrom(other) → allocate new nodes, copy data
operator=(const List&) → clear() + copyFrom(other)
```

---

## 9. Public API

```cpp
template<typename T>
class List {
public:
    struct Node {
        T data;
        Node* next;
        Node* prev;
        Node(const T& value): data(value), next(nullptr), prev(nullptr) {}
    };

private:
    Node* head;
    Node* tail;
    size_t listSize;
    void copyFrom(const List& other);

public:
    List();
    List(const List& other);
    List& operator=(const List& other);
    ~List() noexcept;

    List(std::initializer_list<T> initList);

    void push_back(const T& value);
    void push_front(const T& value);
    void insert(size_t index, const T& value);
    void remove(size_t index);

    void pop_back();
    void pop_front();

    T& front();
    const T& front() const;

    T& back();
    const T& back() const;

    T& at(size_t index);
    const T& at(size_t index) const;

    [[nodiscard]] bool empty() const noexcept;
    size_t size() const noexcept;
    void clear();
    bool contains(const T& value) const;
    Node* find(const T& value) const;
    void reverse();
};
```

---

## 10. Traversal Optimization

### 10.1 Bidirectional Traversal

Because we have both `head` and `tail` pointers, and because each node has both `next` and `prev` pointers, we can traverse the list from **either end**. This is the key optimization for `at(index)` and `insert(index)`.

Naïve traversal always starts at `head` and follows `next` pointers. For `at(N-1)` (the last element), this requires N−1 steps.

Optimized traversal asks: is the target index in the first half or the second half of the list?

- If `index < listSize / 2`: traverse forward from `head` — at most `⌊N/2⌋` steps.
- If `index ≥ listSize / 2`: traverse backward from `tail` — at most `⌈N/2⌉` steps.



### 10.2 The N/2 Optimization — Mathematical Proof

**Claim**: Bidirectional traversal reduces the maximum number of steps from N−1 to ⌊N/2⌋.

**Proof**:

Let the list have N nodes indexed 0 to N−1. We want to reach node at index `k`.

**Naïve (forward-only)**:
- Steps from `head` = k
- Maximum over all k ∈ {0,...,N−1}: N−1

**Bidirectional**:
- If k < N/2: steps from `head` = k < N/2
- If k ≥ N/2: steps from `tail` = (N−1) − k ≤ (N−1) − N/2 = N/2 − 1 < N/2
- Maximum over all k: ⌊(N−1)/2⌋

**Worst case comparison**:

| Method | Worst-case index | Steps |
|:---|:---|:---|
| Forward-only | k = N−1 | N−1 steps |
| Bidirectional | k = ⌊N/2⌋ or k = ⌈N/2⌉ | ⌊N/2⌋ steps |

**Reduction factor**: The bidirectional method requires at most ⌊N/2⌋ steps vs N−1 steps — approximately half as many in the worst case.

**Concrete example for N = 1,000,000**:
- Forward-only worst case: 999,999 pointer dereferences
- Bidirectional worst case: 499,999 pointer dereferences
- Savings: ~500,000 pointer dereferences per lookup

### 10.3 Why O(N/2) is Still O(N)

Despite the 2× practical speedup, **O(N/2) = O(N)** in Big-O notation. Here is the formal proof:

**Big-O definition**: f(N) = O(g(N)) if there exist constants c > 0 and N₀ such that f(N) ≤ c × g(N) for all N ≥ N₀.

**Claim**: N/2 = O(N).

**Proof**: Let f(N) = N/2 and g(N) = N. We need to find c and N₀ such that N/2 ≤ c × N for all N ≥ N₀.

Choosing c = 1 and N₀ = 1: N/2 ≤ 1 × N is clearly true for all N ≥ 1. ∎

**Intuition**: Big-O captures how growth **scales**, not the absolute speed. Both N/2 and N grow **linearly** — doubling N doubles both. The constant factor (1/2) is irrelevant to asymptotic classification.

This is a crucial engineering insight: the optimization is **real and significant** in practice (2× faster), but does not change the asymptotic category. If you need O(1) access by index, you must use a different data structure (e.g., a dynamic array). No amount of traversal optimization can make a linked list's `at(index)` better than O(N).

---

## 11. Pointer Relinking — The Core Operation

Every insertion and deletion in a doubly linked list reduces to a sequence of pointer reassignments. Understanding these reassignments precisely is essential for writing correct list code. An error in any single pointer assignment can violate Invariant 6 and corrupt the list silently.

### 11.1 Insert at Front (push_front)

**Before** (list: 20 → 30):
```
head → [20 | prev=null | next=→] → [30 | prev=← | next=null] ← tail
```

**Goal**: Insert a new node `[10]` before `[20]`.

**Steps**:
```
Step 1: Allocate newNode with data=10, next=null, prev=null

Step 2: newNode->next = head         (newNode points forward to old head)
        newNode → [10 | prev=null | next=→20]

Step 3: head->prev = newNode         (old head points back to newNode)
        newNode ←prev [20 | ...]

Step 4: head = newNode               (head sentinel updated)

Step 5: if list was empty: tail = newNode   (handle empty list case)

Step 6: ++listSize
```

**After** (list: 10 → 20 → 30):
```
head → [10 | prev=null | next=→] → [20 | prev=← | next=→] → [30 | prev=← | next=null] ← tail
```

**Pointer assignments**: 4 assignments + 1 allocation. **O(1)**.

### 11.2 Insert at Back (push_back)

**Before** (list: 10 → 20):
```
head → [10 | prev=null | next=→] → [20 | prev=← | next=null] ← tail
```

**Goal**: Insert a new node `[30]` after `[20]`.

**Steps**:
```
Step 1: Allocate newNode with data=30, next=null, prev=null

Step 2: newNode->prev = tail         (newNode looks back at old tail)

Step 3: tail->next = newNode         (old tail looks forward to newNode)

Step 4: tail = newNode               (tail sentinel updated)

Step 5: if list was empty: head = newNode   (handle empty list case)

Step 6: ++listSize
```

**Pointer assignments**: 4 assignments + 1 allocation. **O(1)**.

### 11.3 Insert in the Middle

**Before** (list: 10 → 30, inserting 20 between them):
```
head → [10 | prev=null | next=→30] → [30 | prev=←10 | next=null] ← tail
         A                               B
```

**Goal**: Insert `[20]` between A and B. The predecessor is A, the successor is B.

**Steps**:
```
Step 1: Allocate newNode (N) with data=20

Step 2: N->prev = A          (newNode looks back at A)
Step 3: N->next = B          (newNode looks forward to B)
Step 4: A->next = N          (A now looks forward to newNode, not B)
Step 5: B->prev = N          (B now looks back at newNode, not A)
Step 6: ++listSize
```

**Critical ordering**: Steps 2 and 3 must happen before Steps 4 and 5. If you execute Step 4 (A->next = N) before Step 3 (N->next = B), you lose the pointer to B — you no longer know what B is.

A safe mnemonic: **"Set the newcomer's pointers before breaking the old link."**

**After**:
```
head → [10 | prev=null | next=→N] → [20 | prev=←10 | next=→30] → [30 | prev=←N | next=null] ← tail
```

**Pointer assignments**: 4 assignments + 1 allocation. **O(1)** (once the predecessor node is known).

Finding the predecessor requires O(N) traversal — so `insert(index, value)` is O(N) total. But the insertion itself, given the node, is O(1).

### 11.4 Remove at Front (pop_front)

**Before** (list: 10 → 20 → 30):
```
head → [10 | prev=null | next=→20] → [20 | prev=←10 | next=→30] → ...
```

**Steps**:
```
Step 1: Node* toDelete = head           (save pointer to node being removed)

Step 2: head = head->next               (advance head to second node)

Step 3: if head != nullptr:
            head->prev = nullptr        (new head has no predecessor)
        else:
            tail = nullptr              (list is now empty — clear tail too)

Step 4: delete toDelete                 (free the removed node)

Step 5: --listSize
```

**Pointer assignments**: 2–3 assignments + 1 deletion. **O(1)**.

### 11.5 Remove at Back (pop_back)

**Before** (list: 10 → 20 → 30):
```
... → [20 | prev=←10 | next=→30] → [30 | prev=←20 | next=null] ← tail
```

**Steps**:
```
Step 1: Node* toDelete = tail           (save pointer to last node)

Step 2: tail = tail->prev               (retreat tail to second-to-last node)
        // This is the O(1) operation that was O(N) in the singly linked list!

Step 3: if tail != nullptr:
            tail->next = nullptr        (new tail has no successor)
        else:
            head = nullptr              (list is now empty — clear head too)

Step 4: delete toDelete

Step 5: --listSize
```

**Pointer assignments**: 2–3 assignments + 1 deletion. **O(1)**.

The `tail->prev` dereference in Step 2 is the operation that requires the `prev` pointer. In a singly linked list, `tail->prev` doesn't exist, so `pop_back` must traverse from `head` — O(N). The doubly linked list's `prev` pointer makes this a single dereference — O(1).

### 11.6 Remove in the Middle

**Before** (list: 10 → 20 → 30, removing 20):
```
[10 | next=→N] → [20 | prev=←10 | next=→30] → [30 | prev=←N | next=null]
  A                       N (to remove)                 B
```

**Steps**:
```
Step 1: Node* A = N->prev       (A is N's predecessor)
Step 2: Node* B = N->next       (B is N's successor)

Step 3: if A != nullptr: A->next = B   (A now skips N, points to B)
        else: head = B                  (N was head; B becomes new head)

Step 4: if B != nullptr: B->prev = A   (B now skips N, points back to A)
        else: tail = A                  (N was tail; A becomes new tail)

Step 5: delete N

Step 6: --listSize
```

**After**:
```
[10 | next=→30] → [30 | prev=←10 | next=null]
```

**Pointer assignments**: 4 assignments + 1 deletion. **O(1)** given node pointer N.

---

## 12. Function-by-Function Deep Dive

### 12.1 push_back

**Purpose**: Appends a copy of the given value as a new node at the end of the list.

**Internal Algorithm**:
1. Allocate a new `Node` with the given value via `new Node(value)`.
2. If the list is empty (`tail == nullptr`): set both `head` and `tail` to the new node.
3. Otherwise: link the new node after `tail` (4 pointer assignments as in Section 11.2), then update `tail`.
4. Increment `listSize`.

**Memory Behaviour**: One heap allocation of `sizeof(Node)` bytes. No existing nodes are moved or modified (their `data` fields remain untouched; only `tail->next` gains a new value).

**Best Case**: **O(1)** — same as worst case.
**Worst Case**: **O(1)** — one allocation, 4 pointer assignments, one increment.
**Space Complexity**: O(1) — one new node.

**Proof**: The operation performs:
- `new Node(value)`: O(1) (single heap allocation + constructor call)
- At most 4 pointer assignments: O(1) each
- One increment of `listSize`: O(1)

No loops, no recursion, no data-dependent branching on N. Total: O(1). ∎

---

### 12.2 push_front

**Purpose**: Prepends a copy of the given value as a new node at the beginning of the list.

**Internal Algorithm**:
1. Allocate a new `Node`.
2. If the list is empty: set both `head` and `tail` to the new node.
3. Otherwise: link the new node before `head` (4 pointer assignments as in Section 11.1), then update `head`.
4. Increment `listSize`.

**Memory Behaviour**: One heap allocation.

**Best Case**: **O(1)**.
**Worst Case**: **O(1)**.
**Space Complexity**: O(1).

**Proof**: Identical structure to `push_back`. One allocation, at most 4 pointer assignments, one increment. No work proportional to N. Total: O(1). ∎

---

### 12.3 insert

**Purpose**: Inserts a copy of the given value at the specified index, so that the new node ends up at position `index` (0-based) in the list.

**Internal Algorithm**:
1. Bounds check: if `index > listSize`, throw `std::out_of_range`.
2. If `index == 0`: delegate to `push_front` — O(1).
3. If `index == listSize`: delegate to `push_back` — O(1).
4. Otherwise:
   a. Traverse from the closer end (bidirectional optimization) to find the node currently at position `index`.
   b. Allocate a new node.
   c. Perform the 4-pointer middle insertion (as in Section 11.3).
   d. Increment `listSize`.

**Memory Behaviour**: One heap allocation regardless of position.

**Best Case**: **O(1)** — `index == 0` or `index == listSize` (no traversal).
**Worst Case**: **O(N)** — `index == N/2` (the traversal optimization's worst case, ⌊N/2⌋ steps).
**Space Complexity**: O(1) — one new node.

**Proof**:

Let S be the number of traversal steps to reach position `index`. With the bidirectional optimization:

S = min(index, listSize − index)

The maximum of S over all valid indices is ⌊(listSize − 1)/2⌋ ≤ ⌊N/2⌋.

Since ⌊N/2⌋ = Θ(N) (specifically, N/2 − 1/2 < ⌊N/2⌋ ≤ N/2), the traversal is Θ(N) in the worst case.

The insertion itself (pointer relinking + allocation) is O(1).

Total: O(S) + O(1) = O(⌊N/2⌋) = O(N). ∎

---

### 12.4 remove

**Purpose**: Removes the node at the specified index.

**Internal Algorithm**:
1. Bounds check: if `index >= listSize`, throw `std::out_of_range`.
2. If `index == 0`: delegate to `pop_front` — O(1).
3. If `index == listSize − 1`: delegate to `pop_back` — O(1).
4. Otherwise:
   a. Traverse (bidirectional) to find the node at `index`.
   b. Perform the 4-pointer middle removal (as in Section 11.6).
   c. `delete` the node (calls `~T()` on its data).
   d. Decrement `listSize`.

**Memory Behaviour**: One heap deallocation. No other nodes are moved.

**Best Case**: **O(1)** — `index == 0` or `index == listSize − 1`.
**Worst Case**: **O(N)** — `index == N/2`.
**Space Complexity**: O(1) — one fewer node.

**Proof**: Same as `insert`. Traversal dominates: O(⌊N/2⌋) = O(N). The relinking and deletion are O(1). Total: O(N). ∎

---

### 12.5 pop_back

**Purpose**: Removes the last element. Throws `std::out_of_range` if the list is empty.

**Internal Algorithm**:
1. If `listSize == 0`, throw `std::out_of_range`.
2. Save `toDelete = tail`.
3. `tail = tail->prev` — the single backward step that requires `prev`.
4. If `tail != nullptr`: `tail->next = nullptr`. Else: `head = nullptr` (list became empty).
5. `delete toDelete`.
6. `--listSize`.

**Memory Behaviour**: One heap deallocation.

**Best Case**: **O(1)**.
**Worst Case**: **O(1)**.
**Space Complexity**: O(1).

**Proof**: One comparison, one pointer dereference (`tail->prev`), at most two pointer assignments, one `delete`, one decrement. All O(1). No traversal. The `prev` pointer eliminates the O(N) traversal that a singly linked list would require. ∎

---

### 12.6 pop_front

**Purpose**: Removes the first element. Throws `std::out_of_range` if the list is empty.

**Internal Algorithm**:
1. If `listSize == 0`, throw `std::out_of_range`.
2. Save `toDelete = head`.
3. `head = head->next`.
4. If `head != nullptr`: `head->prev = nullptr`. Else: `tail = nullptr`.
5. `delete toDelete`.
6. `--listSize`.

**Memory Behaviour**: One heap deallocation.

**Best Case**: **O(1)**.
**Worst Case**: **O(1)**.
**Space Complexity**: O(1).

**Proof**: One comparison, one pointer dereference, at most two pointer assignments, one `delete`, one decrement. All O(1). ∎

---

### 12.7 front / back

**Purpose**:
- `front()`: Returns a reference to the data of the first node.
- `back()`: Returns a reference to the data of the last node.

Both const and non-const versions exist. Throws `std::out_of_range` if empty.

**Internal Algorithm** (`front`):
1. If `listSize == 0`, throw.
2. Return `head->data`.

**Internal Algorithm** (`back`):
1. If `listSize == 0`, throw.
2. Return `tail->data`.

**Memory Behaviour**: No allocation or deallocation.

**Best Case**: **O(1)**.
**Worst Case**: **O(1)**.
**Space Complexity**: O(1).

**Proof**: One comparison + one pointer dereference (`head->data` or `tail->data`). Both O(1). The direct `head` and `tail` sentinel pointers make this O(1) — contrast with a structure that only maintains `head`, where `back()` would require O(N) traversal. ∎

---

### 12.8 at

**Purpose**: Returns a reference to the element at the given index with bounds checking.

**Internal Algorithm**:
1. If `index >= listSize`, throw `std::out_of_range`.
2. Use the bidirectional traversal optimization to find the node at `index`.
3. Return `node->data`.

**Memory Behaviour**: No allocation or deallocation.

**Best Case**: **O(1)** — `index == 0` (return `head->data`) or `index == listSize − 1` (return `tail->data`). Actually, the traversal still runs but with 0 steps — effectively O(1) via the code path.
**Worst Case**: **O(N)** — `index == N/2` (⌊N/2⌋ traversal steps).
**Space Complexity**: O(1).

**Proof**: Traversal steps = min(index, listSize − 1 − index) ≤ ⌊(N−1)/2⌋ = O(N). The return is O(1). Total: O(N). ∎

**Contrast with `Vector::at`**: For `Vector<T>`, `at(index)` is O(1) — the address `data + index × sizeof(T)` is computed arithmetically. For `List<T>`, `at(index)` is O(N) — there is no arithmetic shortcut; each node must be individually followed. This is the defining performance difference between arrays and linked lists.

---

### 12.9 find

**Purpose**: Returns a `Node*` to the first node whose `data == value`, or `nullptr` if not found.

**Internal Algorithm**:
1. Start at `head`.
2. Walk forward via `next` pointers, comparing `current->data == value` at each step.
3. Return the first matching node pointer, or `nullptr` if no match is found.

**Memory Behaviour**: No allocation or deallocation. Read-only traversal.

**Best Case**: **O(1)** — first node matches (`head->data == value`).
**Worst Case**: **O(N)** — no match, or match is at the last node. Full scan of N nodes.
**Space Complexity**: O(1) — one traversal pointer, no auxiliary storage.

**Proof**: The loop runs at most N iterations (one per node). Each iteration performs one pointer dereference (O(1)) and one equality comparison (O(1) for types with O(1) `operator==`). Total: N × O(1) = O(N). ∎

**Note**: `find` returns a `Node*` rather than an index. This is more powerful — given a `Node*`, subsequent insertion or removal at that position is O(1) (just pointer relinking). If `find` returned an index, you'd have to traverse again to find the node — O(N) twice. The `Node*` interface enables the list's O(1) relinking to be composable with search.

---

### 12.10 contains

**Purpose**: Returns `true` if any node's data equals the given value.

**Internal Algorithm**:
1. Call `find(value)`.
2. Return `find(value) != nullptr`.

**Memory Behaviour**: No allocation or deallocation.

**Best Case**: **O(1)**.
**Worst Case**: **O(N)**.
**Space Complexity**: O(1).

**Proof**: Delegates entirely to `find`. Complexity is identical. ∎

---

### 12.11 reverse

**Purpose**: Reverses the order of all nodes in-place, so the old tail becomes the new head and vice versa.

**Internal Algorithm**:
1. Start at `head`.
2. For each node, swap its `next` and `prev` pointers.
3. After traversal, swap `head` and `tail`.



**Why `current = current->prev` after swap**: After `std::swap(current->next, current->prev)`, what was `next` is now `prev`. To advance forward in the original list (which is now backward-traversal in the new list), we follow the new `prev` — which was the old `next`.

**Memory Behaviour**: No allocation or deallocation. In-place modification.

**Best Case**: **O(N)** — must visit every node once.
**Worst Case**: **O(N)** — same.
**Space Complexity**: O(1) — two pointer variables for traversal and swapping; no auxiliary structures.

**Proof**: The loop visits each of the N nodes exactly once. At each node, `std::swap` exchanges two pointers — O(1). After the loop, `std::swap(head, tail)` is O(1). Total: N × O(1) + O(1) = O(N). ∎

---

### 12.12 clear

**Purpose**: Destroys all nodes and resets the list to an empty state. The `List` object remains valid and reusable.

**Internal Algorithm**:
1. Traverse from `head` to the end, deleting each node (saving `next` before each deletion).
2. Set `head = nullptr`, `tail = nullptr`, `listSize = 0`.

**Memory Behaviour**: N heap deallocations. After `clear()`, the list owns no heap memory.

**Best Case**: **O(1)** — list is already empty (loop body never executes).
**Worst Case**: **O(N)** — N `delete` calls.
**Space Complexity**: O(1) — only a traversal pointer variable.

**Proof**: The loop executes exactly `listSize` iterations. Each iteration: one pointer save (O(1)), one `delete` (O(1) amortized), one pointer advance (O(1)). Total: N × O(1) = O(N). Setting the three members at the end: O(1). ∎

---

### 12.13 size / empty

**Purpose**:
- `size()`: Returns the current number of nodes.
- `empty()`: Returns `true` if the list has no nodes.

**Internal Algorithm**: Return `listSize` or `listSize == 0` directly.

**Memory Behaviour**: None.

**Best Case**: **O(1)**.
**Worst Case**: **O(1)**.
**Space Complexity**: O(1).

**Proof**: Single return of a stored integer. O(1).

**Why maintain `listSize` explicitly?**: Without it, `size()` would require traversing all N nodes — O(N). By incrementing on every push and decrementing on every pop, we maintain an exact count at O(1) cost per mutation, making `size()` O(1). ∎

---

### 12.14 Copy Constructor

**Purpose**: Creates an independent deep copy of another `List`.

**Internal Algorithm**: Initialize to empty, then `push_back` each element from other (via `copyFrom`). Traverses the source list once.

**Memory Behaviour**: N heap allocations (one per node). The result is a completely independent chain of N nodes.

**Best Case**: **O(N)**.
**Worst Case**: **O(N)**.
**Space Complexity**: O(N) — allocates N new nodes.

**Proof**: The loop calls `push_back` N times. Each `push_back` is O(1). Total: N × O(1) = O(N). ∎

---

### 12.15 Copy Assignment Operator

**Purpose**: Replaces the list's contents with a deep copy of another list.

**Internal Algorithm**: `clear()` to destroy current nodes, then `copyFrom(other)` to deep-copy.

**Memory Behaviour**: M deallocations (clearing current list of size M), then N allocations (copying N nodes from other).

**Best Case**: **O(1)** — self-assignment (`this == &other`), immediate return.
**Worst Case**: **O(M + N)** = **O(N)** — clear M nodes + copy N nodes.
**Space Complexity**: O(N) — result has N nodes.

**Proof**: `clear()` is O(M). `copyFrom` is O(N). Total: O(M + N) = O(max(M, N)) = O(N). ∎

---

### 12.16 Destructor

**Purpose**: Destroys all nodes and frees all heap memory.

**Internal Algorithm**: Calls `clear()`.

**Memory Behaviour**: N heap deallocations.

**Best Case**: **O(1)** — empty list.
**Worst Case**: **O(N)** — N nodes deleted.
**Space Complexity**: O(1) — only frees memory, no new allocations.

**Proof**: Delegates to `clear()` — O(N) for N nodes, O(1) for empty list. ∎

---

## 13. Time Complexity Analysis

| Operation | Complexity | Structural Reason |
|:---|:---|:---|
| `push_front` | **O(1)** | `head` pointer gives direct access to front. 4 pointer assignments. |
| `push_back` | **O(1)** | `tail` pointer gives direct access to back. 4 pointer assignments. |
| `pop_front` | **O(1)** | `head` pointer. `head->next` gives new head. 3 pointer ops + 1 `delete`. |
| `pop_back` | **O(1)** | `tail->prev` gives new tail — the `prev` pointer's critical contribution. |
| `front` | **O(1)** | Direct dereference of `head->data`. |
| `back` | **O(1)** | Direct dereference of `tail->data`. |
| `at(index)` | **O(N)** | Memory is scattered — must follow pointer chain node by node. |
| `insert(index)` | **O(N)** | Traversal to find position: O(N). Relinking itself: O(1). |
| `remove(index)` | **O(N)** | Traversal to find node: O(N). Relinking + delete: O(1). |
| `find(value)` | **O(N)** | Linear scan — no index structure, no sorted order assumed. |
| `contains(value)` | **O(N)** | Delegates to `find`. |
| `reverse()` | **O(N)** | Must visit every node once to swap its `next`/`prev`. |
| `clear()` | **O(N)** | Must `delete` every node. |
| `size()` | **O(1)** | Returns stored `listSize` counter. |
| `empty()` | **O(1)** | Compares `listSize == 0`. |
| Copy Constructor | **O(N)** | Allocates N new nodes, copies N data elements. |
| Copy Assignment | **O(N)** | Clears M + copies N = O(M+N) = O(N). |
| Destructor | **O(N)** | Deletes all N nodes. |

---

## 14. Complexity Proofs

### push_front / push_back — O(1)

**Proof**: Both operations perform:
- 1 `new Node(value)` call: O(1) (single heap allocation)
- At most 4 pointer assignments: O(1) each
- 1 increment of `listSize`: O(1)

None of these operations depend on N (the current list size). In particular, there is no loop, no traversal, and no element copying. The constant number of operations means the time is bounded by a constant c for all N ≥ 1, satisfying the Big-O definition with that c and N₀ = 1. Total: O(1). ∎

### pop_front / pop_back — O(1)

**Proof for `pop_back`** (the non-trivial case): `pop_back` performs:
- 1 comparison (`listSize == 0`): O(1)
- 1 pointer read (`tail->prev`): O(1) — single memory dereference
- 1–2 pointer assignments: O(1)
- 1 `delete toDelete`: O(1) amortized (heap deallocation)
- 1 decrement: O(1)

The key step `tail = tail->prev` is **a single pointer dereference** — not a traversal. The `prev` pointer on the tail node directly holds the address of the second-to-last node. No iteration is needed. Total: O(1). ∎

### insert(index, value) — O(N)

**Proof**:

Let T(N, k) be the time for `insert(k, value)` on a list of size N.

For k = 0 (push_front): T(N, 0) = O(1) — no traversal.
For k = N (push_back): T(N, N) = O(1) — no traversal.
For 0 < k < N:
- Traversal steps = min(k, N − k) (bidirectional optimization)
- Each step: 1 pointer dereference = O(1)
- Traversal cost: O(min(k, N − k))
- Insertion cost (once node found): O(1)
- Total: O(min(k, N − k))

Worst case: k = ⌊N/2⌋, giving min(⌊N/2⌋, N − ⌊N/2⌋) = ⌊N/2⌋ steps.

Since ⌊N/2⌋ = Θ(N): T(N, ⌊N/2⌋) = Θ(N).

Therefore the worst-case time complexity of `insert` is Θ(N), which is O(N). ∎

### remove(index) — O(N)

**Proof**: Identical structure to `insert`. Traversal to position k costs O(min(k, N − k)), maximized at k = ⌊N/2⌋ = Θ(N). The removal itself (pointer relinking + `delete`) is O(1). Total worst case: O(N). ∎

### at(index) — O(N)

**Proof**:

With the bidirectional optimization, accessing index k requires:
- If k < N/2: k steps forward from `head`
- If k ≥ N/2: (N − 1 − k) steps backward from `tail`

Number of steps S(k) = min(k, N − 1 − k).

Maximum of S over all k ∈ {0,...,N−1}:

max S(k) = S(⌊(N−1)/2⌋) = ⌊(N−1)/2⌋

For large N: ⌊(N−1)/2⌋ ≈ N/2.

Since N/2 = O(N) (constants are dropped in Big-O), the worst-case is O(N). ∎

**Contrast with array**: `Vector::at(k)` computes address = `data + k × sizeof(T)` — a single multiply-and-add, O(1) regardless of k and N.

### find(value) — O(N)

**Proof**:

Linear search visits nodes in order: n₀, n₁, ..., n_{N-1}, terminating at the first match.

- Best case: match at n₀ → 1 comparison → O(1)
- Worst case: no match → N comparisons → O(N)
- Average case (assuming uniform random target position): expected (N+1)/2 comparisons → O(N)

Each comparison is T::operator==, assumed O(1). Total worst case: N × O(1) = O(N). ∎

**Could find be better than O(N)?** Only with additional data structures:
- **Sorted list + binary search**: O(log N) comparisons, but achieving sorted insertion is O(N) per insert (traversal), and binary search on a linked list is still O(N) because you cannot jump to the middle in O(1).
- **Hash map of values to nodes**: O(1) average lookup, but O(N) extra space and O(1) amortized insert cost to maintain the map.

For the plain unsorted doubly linked list, O(N) linear search is optimal.

### reverse() — O(N)

**Proof**:

The algorithm visits each node exactly once. For each node, it performs `std::swap(current->next, current->prev)` — a constant-time operation (two pointer assignments). After the loop, `std::swap(head, tail)` is O(1).

Total: N × O(1) + O(1) = O(N).

**Lower bound**: Any correct reversal algorithm must at least read every node's position to place it in the reversed list. This requires Ω(N) time. Therefore, our O(N) algorithm is asymptotically optimal for reversal. ∎

### clear() — O(N)

**Proof**:

The loop executes exactly `listSize = N` iterations. Each iteration:
- Saves `next`: 1 pointer read = O(1)
- `delete current`: O(1) amortized
- Advances `current = next`: O(1)

Total: N × O(1) = O(N).

Setting head, tail, listSize at the end: O(1).

Grand total: O(N). ∎

### Copy Constructor — O(N)

**Proof**:

`copyFrom` calls `push_back` once per node in the source list. Each `push_back` is O(1). Source has N nodes. Total: N × O(1) = O(N). ∎

### Destructor — O(N) / O(1) for empty

**Proof**:

Calls `clear()`. If N = 0: O(1) (loop never executes). If N > 0: O(N). ∎

---

## 15. Linked List vs Dynamic Array — When to Choose Which

| Criterion | Choose `List<T>` | Choose `Vector<T>` |
|:---|:---|:---|
| **Dominant operation** | Insertions/deletions at both ends | Random access by index |
| **Iteration pattern** | Sequential (front-to-back or back-to-front) | Random or indexed |
| **Element stability** | Must hold stable pointers/iterators to elements | Pointers can be invalidated (reallocation) |
| **Middle insertion** | Frequent (given a node pointer: O(1)) | Rare (O(N) shifting) |
| **Memory pattern** | Predictable, per-element allocation | Bursty (rare large reallocations) |
| **Cache performance** | Not a concern (or data too large to benefit) | Critical (tight loops, SIMD, prefetch) |
| **Memory overhead** | Acceptable (16 bytes/node extra) | Minimal (`sizeof(T)` per element) |
| **Queue / Deque** | **Yes** — O(1) push/pop at both ends | Requires tricks (circular buffer or `deque`) |
| **Stack** | Yes — O(1) push/pop at one end | Yes — O(1) amortized `push_back`/`pop_back` |
| **Sorted structure** | Poor (insertion sort is O(N²) due to O(N) traversal) | Same, but better cache for sort algorithms |

**The golden rule**: If you ever need O(1) random access (`at(i)`), use a `Vector`. If you need strict O(1) insertion/deletion at both ends and never need random access, use a `List`.

In practice, `Vector<T>` wins for most general-purpose use cases because CPU cache behavior dominates real-world performance. A `Vector` of 1M integers fits in ~4 MB of contiguous memory; a `List` of 1M integers requires ~1M separate heap allocations totaling ~20 MB, with catastrophic cache behavior for any sequential scan.

The `List<T>` wins when:
- You implement a **queue** or **deque** (enqueue at back, dequeue at front — both O(1))
- You need **stable iterators** (no reallocation ever moves nodes)
- You are implementing the **kernel process list**, **LRU cache**, or **undo/redo stack**
- You perform many **O(1) middle insertions** at a known position (e.g., splicing lists)