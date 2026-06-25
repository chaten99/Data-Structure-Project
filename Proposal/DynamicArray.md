# Dynamic Array (Vector) — Design Document

---

## Table of Contents

1. [Introduction](#1-introduction)
   - 1.1 [What is a Dynamic Array](#11-what-is-a-dynamic-array)
   - 1.2 [Why Dynamic Arrays Exist](#12-why-dynamic-arrays-exist)
   - 1.3 [Static Arrays vs Dynamic Arrays](#13-static-arrays-vs-dynamic-arrays)
   - 1.4 [Real-World Use Cases](#14-real-world-use-cases)
2. [Templates and typename T](#2-templates-and-typename-t)
   - 2.1 [What typename T Means](#21-what-typename-t-means)
   - 2.2 [Why This Implementation is Templated](#22-why-this-implementation-is-templated)
   - 2.3 [How the Compiler Uses T](#23-how-the-compiler-uses-t)
3. [Design Evolution](#3-design-evolution)
   - 3.1 [Attempt 1 — Fixed-Size Array](#31-attempt-1--fixed-size-array)
   - 3.2 [Attempt 2 — Allocate on Every Insertion](#32-attempt-2--allocate-on-every-insertion)
   - 3.3 [Attempt 3 — Geometric Growth](#33-attempt-3--geometric-growth)
4. [Internal Architecture](#4-internal-architecture)
   - 4.1 [Member Variables](#41-member-variables)
   - 4.2 [Ownership Semantics](#42-ownership-semantics)
   - 4.3 [Class Invariants](#43-class-invariants)
5. [Memory Layout](#5-memory-layout)
6. [Heap Allocation Fundamentals](#6-heap-allocation-fundamentals)
   - 6.1 [Stack vs Heap](#61-stack-vs-heap)
   - 6.2 [Why malloc and free](#62-why-malloc-and-free)
   - 6.3 [Allocation vs Construction — The Two-Phase Model](#63-allocation-vs-construction--the-two-phase-model)
   - 6.4 [Placement New](#64-placement-new)
7. [Constructors and Object Lifetime](#7-constructors-and-object-lifetime)
   - 7.1 [Default Constructor](#71-default-constructor)
   - 7.2 [Initializer List Constructor](#72-initializer-list-constructor)
   - 7.3 [Copy Constructor](#73-copy-constructor)
   - 7.4 [Copy Assignment Operator](#74-copy-assignment-operator)
   - 7.5 [Destructor](#75-destructor)
8. [Rule of Three](#8-rule-of-three)
9. [Growth Strategy Analysis](#9-growth-strategy-analysis)
   - 9.1 [Growth Factor Comparison](#91-growth-factor-comparison)
   - 9.2 [Memory Waste vs Reallocation Count](#92-memory-waste-vs-reallocation-count)
   - 9.3 [Memory Reuse Theorem](#93-memory-reuse-theorem)
10. [Mathematical Growth Factor Proof](#10-mathematical-growth-factor-proof)
    - 10.1 [Reallocation Count Formula](#101-reallocation-count-formula)
    - 10.2 [Numerical Comparison for N = 1,000,000](#102-numerical-comparison-for-n--1000000)
11. [Amortized Analysis of push_back](#11-amortized-analysis-of-push_back)
12. [Function-by-Function Deep Dive](#12-function-by-function-deep-dive)
    - 12.1 [push_back](#121-push_back)
    - 12.2 [pop_back](#122-pop_back)
    - 12.3 [insert](#123-insert)
    - 12.4 [remove](#124-remove)
    - 12.5 [clear](#125-clear)
    - 12.6 [reserve](#126-reserve)
    - 12.7 [shrink_to_fit](#127-shrink_to_fit)
    - 12.8 [operator\[\] and at](#128-operator-and-at)
    - 12.9 [front / back](#129-front--back)
    - 12.10 [find / contains](#1210-find--contains)
    - 12.11 [size / capacity / empty](#1211-size--capacity--empty)
13. [Complexity Proofs](#13-complexity-proofs)

---

## 1. Introduction

### 1.1 What is a Dynamic Array

A dynamic array is a contiguous, resizable sequence container that manages its own heap-allocated memory buffer. Unlike fixed-size arrays whose dimensions must be known at compile time, a dynamic array grows and shrinks at runtime, automatically reallocating its internal buffer when the current capacity is exhausted.

At the hardware level, a dynamic array is nothing more than a pointer to a heap-allocated block of memory, combined with two pieces of metadata — the number of elements currently stored (`size`) and the total number of elements the buffer can hold before a reallocation is required (`capacity`). This simplicity is precisely what makes the dynamic array the most efficient general-purpose sequence container.

This project provides a custom, from-scratch implementation — `Vector<T>` — built using raw `malloc`/`free` memory management, placement `new` for object construction, and explicit destructor calls for object teardown.

### 1.2 Why Dynamic Arrays Exist

The fundamental tension in systems programming is between **static predictability** and **runtime flexibility**:

| Constraint | Static Array | Dynamic Array |
|:---|:---|:---|
| Size known at compile time | **Required** | Not required |
| Memory location | Stack (or static segment) | Heap |
| Resizable | No | Yes |
| Bounds checking | Manual | Optional (via `at()`) |
| Lifetime | Scope-bound | Programmer-controlled |

In real systems, the size of data is almost never known at compile time. A web server does not know how many connections it will handle. A compiler does not know how many tokens are in a source file. A game engine does not know how many particles are on screen. Dynamic arrays exist to solve this fundamental mismatch between the rigidity of static memory and the fluidity of runtime data.

### 1.3 Static Arrays vs Dynamic Arrays

**Static arrays** live on the stack. Their size `N` is baked into the type at compile time. They cannot grow, they cannot shrink, and if `N` is too large, they overflow the stack (typically 1–8 MB).

**Dynamic arrays** separate the control structure from the data. The `Vector<T>` object itself — containing just a pointer, a size, and a capacity — occupies a fixed 24 bytes on the stack (on a 64-bit system). The actual element storage lives on the heap, where gigabytes of memory are available. This indirection is the price paid for runtime flexibility.

```
Static Array (Stack):
┌──────────────────────────────────────┐
│ a₀ │ a₁ │ a₂ │ ... │ aₙ₋₁          │  ← Entire array on the stack
└──────────────────────────────────────┘
  Size fixed at compile time. Cannot grow.

Dynamic Array (Stack + Heap):
  Stack                          Heap
┌─────────────┐         ┌──────────────────────────────┐
│ data ────────│────────→│ a₀ │ a₁ │ a₂ │ ... │  -  │  │
│ size: 3      │         └──────────────────────────────┘
│ capacity: 5  │           Elements can grow at runtime.
└─────────────┘
  24 bytes fixed.
```

### 1.4 Real-World Use Cases

Dynamic arrays are the default data structure in nearly every systems programming context: compiler token streams, AST node children, game engine entity lists, vertex buffers, networking packet buffers, database row buffers, OS process tables, and machine learning tensor storage. The contiguous memory layout makes them the fastest container for sequential access due to CPU cache locality — elements are stored side-by-side, allowing the CPU to prefetch upcoming elements into its cache hierarchy.

---

## 2. Templates and typename T

### 2.1 What typename T Means

When the class is declared as `Vector<T>`, the `T` is a **type parameter** — a placeholder that the programmer fills in at the point of use. `typename` is the keyword that introduces it. `T` itself is just a conventional name (short for "Type"); it could be named anything.

When you write `Vector<int>`, the compiler substitutes every occurrence of `T` inside the class definition with `int`. When you write `Vector<std::string>`, every `T` becomes `std::string`. This substitution happens entirely at **compile time** — no runtime overhead, no type-checking penalty.

The angle-bracket syntax `<T>` is what makes this a **class template** rather than a regular class. A class template is not itself a class — it is a blueprint for generating classes. `Vector<int>` and `Vector<std::string>` are two completely separate classes generated from the same blueprint, each with their own compiled machine code.

### 2.2 Why This Implementation is Templated

Without templates, you would need to write a separate `VectorInt`, `VectorString`, `VectorDouble`, and so on for every type you want to store. Each would be identical in structure, differing only in the type of element. Templates eliminate this duplication:

- **Type safety**: `Vector<int>` cannot accidentally store a `std::string`. The compiler catches type mismatches at compile time.
- **Code reuse**: All the logic for growth, reallocation, and pointer management is written once and works for any type.
- **Zero-cost abstraction**: The compiler generates specialized machine code for each instantiation — `Vector<int>` uses integer-sized slots, `Vector<double>` uses double-sized slots. There is no runtime dispatch, no virtual function overhead, no boxing.
- **Enables `if constexpr` optimizations**: Because `T` is known at compile time, the implementation can take different code paths for trivially copyable vs non-trivially copyable types, eliminating unnecessary work.

### 2.3 How the Compiler Uses T

Every time `T` appears inside the class — in `sizeof(T)`, in `new(&buffer[i]) T(value)`, in `data[i].~T()` — the compiler substitutes the real type. This means:

- `sizeof(T)` becomes `sizeof(int)` = 4 for `Vector<int>`, or `sizeof(std::string)` = 32 for `Vector<std::string>`
- The destructor call `data[i].~T()` becomes `data[i].~int()` (a no-op, optimized away) or `data[i].~basic_string()` (actually frees heap memory the string owns)
- The `is_trivially_copyable<T>` check resolves to `true` for `int` and `false` for `std::string`, selecting the `memcpy` path vs the placement-new path at compile time

This compile-time specialization is what makes the template implementation both generic and maximally efficient.

---

## 3. Design Evolution

### 3.1 Attempt 1 — Fixed-Size Array

**Design**: Allocate a fixed-size buffer at construction time with a hardcoded capacity of 1000 elements.

**Problems**:

| Problem | Impact | Severity |
|:---|:---|:---|
| **Memory waste** | If only 3 elements are stored, 997 slots are wasted | High |
| **Capacity limitation** | Cannot store more than 1000 elements | Critical |
| **Stack overflow** | For large `T` (e.g., 1 KB structs), `1000 × 1024 = 1 MB` overflows the stack | Critical |
| **Inflexible** | Every use case must share the same hardcoded limit | High |

**Verdict**: This approach fundamentally conflates "how much memory is allocated" with "how much data exists." It provides no mechanism to adapt to its actual workload.

### 3.2 Attempt 2 — Allocate on Every Insertion

**Design**: On every `push_back`, allocate exactly `size + 1` bytes, copy all elements, free the old block.

**Problems**: This design incurs a copy of all existing elements on every single insertion. The total number of element copies for N insertions is:

$$\text{Total copies} = 0 + 1 + 2 + \cdots + (N-1) = \frac{N(N-1)}{2} = O(N^2)$$

For N = 1,000,000: approximately 5 × 10¹¹ copy operations. Additionally, every `push_back` triggers a `malloc`/`free` pair — heap allocation is a system call costing hundreds to thousands of CPU cycles. For 1 million insertions, this means 1 million system calls.

**Verdict**: O(N²) total work for N insertions makes this design unusable for any serious workload.

### 3.3 Attempt 3 — Geometric Growth

**Design**: Maintain a `capacity` always ≥ `size`. When `size == capacity`, allocate a new buffer at `2 × capacity`. This is the design used in our `Vector<T>` implementation.

**Why this works**: The total number of element copies across all reallocations for N insertions is:

$$\text{Total copies} = 1 + 2 + 4 + 8 + \cdots + N = 2N - 1 = O(N)$$

Amortized over N insertions: O(N) / N = **O(1)** per insertion. The geometric growth strategy converts O(N²) total work into O(N) total work by performing reallocations exponentially less frequently as the vector grows. This is proven rigorously in [Section 11](#11-amortized-analysis-of-push_back).

---

## 4. Internal Architecture

### 4.1 Member Variables

`Vector<T>` contains exactly three private data members: a raw pointer `data` to the heap-allocated buffer, `currentSize` tracking the number of live constructed elements, and `currentCapacity` tracking the total number of T-sized slots in the buffer.

**`data` pointer**: Represents sole ownership of the heap buffer. The block has space for `currentCapacity` objects of type `T`, but only the first `currentSize` slots contain live, constructed objects. If this pointer is lost without first freeing the memory, a memory leak occurs.

**`currentSize`**: The "logical" size — the number of elements visible to the user. Only indices `[0, currentSize)` contain valid, live objects. Uses `size_t` — an unsigned integer guaranteed to be large enough to represent the size of any object in memory (8 bytes on 64-bit systems, max ~1.8 × 10¹⁹).

**`currentCapacity`**: The "physical" capacity — how many elements could be stored before a reallocation. The invariant `currentSize ≤ currentCapacity` must always hold.

### 4.2 Ownership Semantics

`Vector<T>` follows the **exclusive ownership model**: the `Vector` object owns the memory pointed to by `data`, no other object shares this ownership, and the owner is solely responsible for constructing objects in the buffer, destroying them, and freeing the buffer itself. This is enforced through the Rule of Three ([Section 8](#8-rule-of-three)).

### 4.3 Class Invariants

```
Invariant 1:  0 ≤ currentSize ≤ currentCapacity
Invariant 2:  currentCapacity ≥ 1  (always at least 1 slot allocated)
Invariant 3:  data ≠ nullptr  (buffer is always allocated)
Invariant 4:  Elements at indices [0, currentSize) are live, constructed objects
Invariant 5:  Slots at indices [currentSize, currentCapacity) are raw memory (no live objects)
```

---

## 5. Memory Layout

<p align="center">
<img src="../Memory-Diagrams/DynamicArray-Memory Design.jpeg" width="650">
</p>

The diagram illustrates the complete memory architecture of our `Vector<T>` implementation.

**The `data` Pointer**: Lives inside the `Vector<T>` object on the stack, storing the address of the first byte of the heap-allocated array. Every element access — through `operator[]`, `at()`, `front()`, or `back()` — is a dereference of this pointer with an offset.

**`currentSize`**: When `size = 5`, elements at indices `[0]` through `[4]` are live, fully-constructed objects. Incremented by `push_back` and `insert`, decremented by `pop_back` and `remove`.

**`currentCapacity`**: When `capacity = 8`, the buffer has room for 8 elements total. Only changed during reallocation or when `reserve`/`shrink_to_fit` are called.

**Occupied Slots**: The shaded slots (indices `[0]` through `[4]`) contain valid `T` objects constructed using placement `new`. Their destructors must be called before the buffer is freed.

**Unused Slots**: The empty slots (indices `[5]` through `[7]`) are allocated but uninitialized raw memory. No `T` object exists here — they are reserved bytes waiting for future `push_back` calls.

**Stack Object vs Heap Memory**: The `Vector<T>` object — containing `data`, `currentSize`, and `currentCapacity` — occupies exactly 24 bytes on the stack (8 bytes per field on 64-bit). This is a fixed cost regardless of how many elements are stored. When a reallocation occurs, the stack object still occupies 24 bytes, but its `data` pointer now points to a different, larger heap allocation.

---

## 6. Heap Allocation Fundamentals

### 6.1 Stack vs Heap

```
┌──────────────────────────────────────────────────┐
│                    STACK                          │
│  • Automatic lifetime (scope-based)              │
│  • LIFO allocation/deallocation                  │
│  • Extremely fast (single pointer adjustment)    │
│  • Limited size (typically 1–8 MB)               │
│  ↓ grows downward                                │
├──────────────────────────────────────────────────┤
│  ↑ grows upward                                  │
│                    HEAP                           │
│  • Manual lifetime (programmer-controlled)       │
│  • Arbitrary allocation/deallocation order        │
│  • Slower (bookkeeping, fragmentation)           │
│  • Large (limited by virtual memory, GBs)        │
└──────────────────────────────────────────────────┘
```

`Vector<T>` must use heap memory because the element buffer size is not known at compile time, the buffer must persist across function calls, it may need to grow to megabytes or gigabytes far exceeding stack limits, and it must be relocatable when growing.

### 6.2 Why malloc and free

This implementation uses `malloc`/`free` rather than `new`/`delete` or `calloc`/`realloc`. The choice is deliberate:

**Why not `calloc`?** Zero-initialization is wasted work. The slots will be initialized via placement `new` with the correct constructor. Furthermore, zero bytes are not a valid object representation for non-trivial types — a zero-initialized `std::string` is undefined behavior, not an empty string.

**Why not `realloc`?** `realloc` may move the memory block by silently `memcpy`-ing the contents. For trivially copyable types this is safe, but for non-trivial types like `std::string`, `memcpy` creates bitwise copies that bypass copy/move constructors — leaving two `std::string` objects sharing the same internal buffer, which causes a double-free when both are destroyed.

**Why `malloc`?** It gives raw memory without constructing anything. This is required because the buffer holds `capacity` slots but only `size` of them should have live objects. `malloc` also enables `memcpy` for trivially copyable types (a significant performance optimization) and makes the memory management strategy fully explicit and auditable.

### 6.3 Allocation vs Construction — The Two-Phase Model

In C++, creating an object involves two independent phases. **Phase 1** is memory allocation — obtaining raw bytes from the heap. **Phase 2** is object construction — running a constructor to initialize those bytes into a valid object state. These phases can be performed separately.

This matters critically for `Vector<T>`. With `size = 3` and `capacity = 8`, slots `[0]–[2]` contain live `std::string` objects whose destructors must be called before the buffer is freed. Slots `[3]–[7]` are raw bytes — no `std::string` exists there. Calling a destructor on these slots would be undefined behavior. Using `new std::string[8]` instead of `malloc` would default-construct all 8 slots immediately, wasting constructor calls for slots that may never be used.

### 6.4 Placement New

Placement `new` is the mechanism for Phase 2 — constructing an object at a specific, already-allocated memory address without allocating new memory. The syntax `new(&buffer[i]) T(value)` means: at the address `&buffer[i]`, invoke `T`'s copy constructor with argument `value`. After this call, `buffer[i]` is a live, fully-constructed `T` object.

This is the only correct way to construct objects in a pre-allocated buffer. Writing `buffer[i] = value` would invoke `T::operator=`, which assumes a live object already exists at `buffer[i]`. On raw memory, that is undefined behavior.

Objects constructed with placement `new` must be destroyed with an explicit destructor call (`data[i].~T()`), not with `delete`. The `delete` operator would attempt to free the pointer as if it were the start of its own allocation — corrupting the heap. Explicit destructor calls are one of the very few situations in C++ where this unusual syntax is not just correct but necessary.

---

## 7. Constructors and Object Lifetime

### 7.1 Default Constructor

Initializes `currentSize = 0`, `currentCapacity = 1`, and allocates a 1-slot buffer via `malloc`. The initial capacity of 1 — rather than 0 — maintains Invariant 3 (`data ≠ nullptr`) and eliminates the need for null checks throughout the entire codebase. The cost of one slot is negligible. If `malloc` returns `nullptr` (out of memory), a `std::bad_alloc` exception is thrown for RAII compatibility.

### 7.2 Initializer List Constructor

Enables brace initialization like `Vector<int> v = {10, 20, 30}`. Sets capacity exactly to the initializer list size (no extra slack, since the size is known upfront), allocates the buffer, and populates it. For **trivially copyable types** (integers, plain structs), a single `memcpy` bulk-copies all elements. For **non-trivial types**, each element is individually copy-constructed via placement `new` with full exception safety — if any copy constructor throws, all previously constructed elements are destroyed before re-throwing.

### 7.3 Copy Constructor

Performs a **deep copy**: allocates a completely independent buffer and copies each element from the source. After the copy, the two `Vector` objects share no state. A shallow copy (copying the pointer value) would make both vectors point to the same buffer — when either is destroyed and frees the buffer, the other's `data` pointer becomes dangling, causing use-after-free and double-free crashes.

### 7.4 Copy Assignment Operator

More complex than the copy constructor because the target already has state. Handles two cases: if the existing buffer is large enough (`currentCapacity ≥ other.currentSize`), the existing buffer is reused — old elements are destroyed and new ones are copy-constructed in place without any `malloc`/`free`. If the existing buffer is too small, a new buffer is allocated and filled first; only then are the old elements destroyed and the old buffer freed. This order ensures exception safety: if the copy into the new buffer throws, the original state is still intact. A self-assignment guard (`if (this != &other)`) prevents `v = v` from destroying its own data before copying.

### 7.5 Destructor

Performs two cleanup operations in strict order: first, destroy all live elements in `[0, currentSize)` by calling their destructors; second, free the buffer. The order is critical — freeing memory before calling destructors would make the destructor calls access freed memory (undefined behavior). Marked `noexcept` because destructors must never throw (a throwing destructor during stack unwinding from another exception causes `std::terminate()`).

---

## 8. Rule of Three

The Rule of Three states: if a class defines any one of the following, it must define all three.

| Function | Responsibility | Without It |
|:---|:---|:---|
| **Destructor** `~Vector()` | Free owned memory, destroy objects | Memory leaks when vector goes out of scope |
| **Copy Constructor** `Vector(const Vector&)` | Create independent deep copy | Compiler generates shallow copy → aliasing → double-free |
| **Copy Assignment** `operator=(const Vector&)` | Replace contents with deep copy | Compiler generates shallow assignment → same aliasing bugs |

Any class that manages a heap resource (memory, file handle, network socket) has a compiler-generated default for these functions that is almost certainly wrong. The defaults perform **memberwise copy** — copying the pointer value, not the pointed-to data — creating two objects that believe they exclusively own the same resource.

---

## 9. Growth Strategy Analysis

### 9.1 Growth Factor Comparison

When a reallocation is triggered, the capacity is multiplied by a growth factor `g`. The choice of `g` involves a tradeoff between memory waste and reallocation frequency:

| Growth Factor `g` | Reallocations for N=10⁶ | Peak unused capacity | Memory reuse possible? |
|:---|:---|:---|:---|
| 1.5× | 35 | Up to 33% | Yes (after ~4 reallocations) |
| 1.75× | 25 | Up to 43% | Partial |
| 2× | 20 | Up to 50% | No (never) |

### 9.2 Memory Waste vs Reallocation Count

Higher growth factors mean fewer reallocations and less copy overhead, but more wasted memory. Lower growth factors mean less wasted memory and enable memory reuse from previously freed blocks, but incur more reallocations. The critical insight is that with `g = 2`, each new allocation is larger than the sum of all previous allocations combined — meaning the allocator can never fit the new block into previously freed space.

### 9.3 Memory Reuse Theorem

For growth factor `g`, the sum of all freed allocation sizes after `k` reallocations is:

$$F(k) = \frac{g^k - 1}{g - 1}$$

The next required allocation is $g^k$. Memory reuse is possible only when $F(k) \geq g^k$, which simplifies to:

$$g^k \times (2 - g) \geq 1$$

For large `k`, this requires `2 − g > 0`, i.e., **`g < 2`**. When `g = 2`, the left side is always 0 — memory reuse is structurally impossible. Our implementation uses `g = 2` for its simplicity of analysis and minimum reallocation count, accepting the memory reuse limitation as an acceptable tradeoff.

---

## 10. Mathematical Growth Factor Proof

### 10.1 Reallocation Count Formula

Starting from an initial capacity $C_0$ with growth factor `g`, after `k` reallocations the capacity is $C(k) = C_0 \times g^k$. To store N elements:

$$k = \left\lceil \log_g\!\left(\frac{N}{C_0}\right) \right\rceil = \left\lceil \frac{\ln(N / C_0)}{\ln g} \right\rceil$$

### 10.2 Numerical Comparison for N = 1,000,000

With initial capacity $C_0 = 1$:

| Growth Factor `g` | Formula | Reallocations | Peak Capacity |
|:---|:---|:---|:---|
| 1.5 | $\lceil \ln(10^6)/\ln(1.5) \rceil = \lceil 34.07 \rceil$ | **35** | 1.5³⁵ ≈ 1,223,736 |
| 1.75 | $\lceil \ln(10^6)/\ln(1.75) \rceil = \lceil 24.69 \rceil$ | **25** | 1.75²⁵ ≈ 1,266,874 |
| 2.0 | $\lceil \ln(10^6)/\ln(2) \rceil = \lceil 19.93 \rceil$ | **20** | 2²⁰ = 1,048,576 |

Growth factor 1.5× requires 75% more reallocations than 2×. Growth factor 2× achieves an elegant peak capacity of exactly 2²⁰ = 1,048,576 ≈ 10⁶, making capacity calculations easy to reason about.

---

## 11. Amortized Analysis of push_back

The worst-case cost of a single `push_back` is O(N) — when a reallocation occurs. However, reallocations happen exponentially less frequently as the vector grows. The **amortized** cost — average cost per operation over a sequence — is O(1).

**Proof — Aggregate (Geometric Series) Method**

Starting from capacity 1, growth factor 2, performing N `push_back` operations. Reallocations occur at sizes 1, 2, 4, 8, ..., $2^{\lfloor \log_2 N \rfloor}$. At reallocation `k`, exactly $2^k$ elements are copied.

**Total copy cost across all reallocations**:

$$C_{\text{total}} = \sum_{k=0}^{\lfloor \log_2 N \rfloor} 2^k = 2^{\lfloor \log_2 N \rfloor + 1} - 1 \leq 2N - 1 < 2N$$

**Total insertion cost** (one construction per element): N.

**Grand total**: $C_{\text{total}} + N < 2N + N = 3N$.

**Amortized cost per push_back**:

$$T_{\text{amortized}} = \frac{3N}{N} = 3 = O(1) \quad \square$$

The expensive reallocations are spaced far enough apart that their total cost is absorbed by the many cheap O(1) insertions between them.

---

## 12. Function-by-Function Deep Dive

### 12.1 push_back

**Purpose**: Appends a copy of the given value to the end of the vector.

**Behaviour**: If `size < capacity`, the element is copy-constructed into the existing unused slot at index `currentSize` — O(1). If `size == capacity`, a new buffer of `2 × capacity` bytes is allocated, all existing elements are transferred, old objects are destroyed, and the old buffer is freed before the new element is inserted.

**Best Case**: O(1) — spare capacity available.
**Worst Case**: O(N) — reallocation required.
**Amortized**: O(1) — proven in Section 11. ∎

### 12.2 pop_back

**Purpose**: Removes the last element without deallocating the buffer.

**Behaviour**: Calls the destructor on the last element (skipped for trivially destructible types via compile-time check), then decrements `currentSize`. The slot transitions from a live object to raw memory, available for a future `push_back` without reallocation.

**Complexity**: O(1) always — one comparison, at most one destructor call, one decrement. ∎

### 12.3 insert

**Purpose**: Inserts a value at a specified index, shifting all subsequent elements right.

**Behaviour**: After bounds and capacity checks, if inserting at the end this delegates to `push_back`. Otherwise, the last element is move-constructed into the first unused slot (extending the live region), elements from `[index, currentSize − 1)` are shifted right by move assignment, and the value is copy-assigned into the vacated slot at `index`.

**Best Case**: O(1) — insert at end.
**Worst Case**: O(N) — insert at index 0, shifting all N elements right. ∎

### 12.4 remove

**Purpose**: Removes the element at a specified index, shifting subsequent elements left.

**Behaviour**: Elements from `[index + 1, currentSize)` are shifted left via move assignment. The last slot (now a duplicate) has its destructor called, and `currentSize` is decremented.

**Best Case**: O(1) — remove last element.
**Worst Case**: O(N) — remove first element, shifting N − 1 elements left. ∎

### 12.5 clear

**Purpose**: Destroys all elements but retains the allocated buffer.

**Behaviour**: For non-trivially destructible types, calls each element's destructor in order. Sets `currentSize = 0`. The buffer is **not freed** — capacity is preserved so future insertions can reuse it without reallocation.

**Complexity**: O(N) for non-trivial types (N destructor calls). O(1) for trivial types (compiler eliminates the loop entirely via compile-time check). ∎

### 12.6 reserve

**Purpose**: Ensures capacity is at least `newCapacity`.

**Behaviour**: If `newCapacity ≤ currentCapacity`, this is a no-op — O(1). Otherwise, a new buffer is allocated at the requested size, all elements are transferred, old objects are destroyed, and the old buffer is freed.

**Complexity**: O(1) if already sufficient. O(N) if reallocation occurs. ∎

### 12.7 shrink_to_fit

**Purpose**: Reduces capacity to match the current size, freeing unused memory.

**Behaviour**: Allocates a smaller buffer of size `max(currentSize, 1)`, transfers all elements, and frees the larger old buffer. Net heap usage decreases by `(oldCapacity − currentSize) × sizeof(T)` bytes.

**Complexity**: O(1) if capacity already equals size. O(N) otherwise. ∎

### 12.8 operator[] and at

**`operator[]`** returns a reference to the element at a given index using direct pointer arithmetic — one multiply, one add, one dereference. No bounds checking. **O(1)**.

**`at`** does the same but adds a bounds check first. If `index ≥ currentSize`, throws `std::out_of_range`. The only additional work is one integer comparison. **O(1)**.

The distinction exists because bounds checking costs one branch per access. In tight inner loops accessing millions of elements, that branch can matter. `operator[]` trusts the caller; `at` protects against accidental out-of-bounds access.

### 12.9 front / back

Return references to the first and last elements respectively, with an empty-check before access. Direct dereferences of `data[0]` and `data[currentSize − 1]`. **O(1)** both. ∎

### 12.10 find / contains

**`find`** performs a linear scan from index 0, returning the first index where `data[i] == value`, or `currentSize` (a past-the-end sentinel) if not found. **O(N)** worst case.

**`contains`** delegates entirely to `find` and returns a boolean. Same complexity: O(1) best case (found at index 0), O(N) worst case. ∎

### 12.11 size / capacity / empty

All three return stored member variables or a direct comparison of one. No traversal, no allocation, no computation. **O(1)** always. `size()` is O(1) because `currentSize` is maintained incrementally on every mutation. ∎

---

## 13. Complexity Proofs

| Operation | Complexity | Proof |
|:---|:---|:---|
| `push_back` | O(1) amortized | Geometric series: total cost over N ops < 3N. Amortized: 3N/N = O(1). ∎ |
| `pop_back` | O(1) | 1 comparison + 1 destructor + 1 decrement. No loops. ∎ |
| `insert(index)` | O(N) | Elements shifted = `currentSize − index`. Each shift O(1). Worst case: index = 0 → N shifts. ∎ |
| `remove(index)` | O(N) | Elements shifted = `currentSize − 1 − index`. Each O(1). Worst: index = 0 → N−1 shifts. ∎ |
| `clear` | O(N) / O(1) | Non-trivial T: N destructor calls × O(1). Trivial T: loop compiled away, O(1). ∎ |
| `reserve` | O(N) / O(1) | Reallocation: N transfers × O(1). No-op if already sufficient. ∎ |
| `shrink_to_fit` | O(N) / O(1) | Same structure as `reserve`. ∎ |
| `operator[]` | O(1) | One multiply + one add + one dereference. Constant-time CPU ops. ∎ |
| `at` | O(1) | Same as `operator[]` plus one comparison. Still O(1). ∎ |
| `front` / `back` | O(1) | One comparison + one dereference. ∎ |
| `find` | O(N) | Linear scan: at most N comparisons × O(1) each. ∎ |
| `contains` | O(N) | Delegates to `find`. Identical bounds. ∎ |
| `size` / `capacity` / `empty` | O(1) | Single stored-value return or comparison. ∎ |
| Copy Constructor | O(N) | N element copies × O(1) each. ∎ |
| Copy Assignment | O(N) | Destroy M + copy N = O(M+N) = O(N). Self-assign: O(1). ∎ |
| Destructor | O(N) / O(1) | Non-trivial T: N destructor calls. Trivial T: only `free`. ∎ |