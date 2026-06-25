# Build Logs
## Session 1: 2026-06-18
### Duration: 3.75 Hours
### Tasks:
    - Created `DesignProposal.md` to outline the design of the data structure library.
    - Made initial commits to set up the project structure and added a basic README files.
    - Made Memory Diagrams for Dynamic Array, LinkedList and HashMap.
### Problem Encountered:
    - None.
### Done:
    - All tasks were completed successfully.

## Session 2: 2026-06-19
### Duration: 5 Hours
### Tasks:
    - Implemented the `Vector` class in `include/Vector.h`.
    - Added member functions such as `push_back`, `pop_back`, `front`, `back`, `at`, `empty`, `size`, and `capacity`.
    - Implemented the constructor, copy constructor, assignment operator, and destructor for the `Vector` class.
### Problem Encountered:
    - Linking errors due to missing implementation of member functions. Resolved by providing definitions for all declared member functions.
    - Linking error due to the templates defined for only one time in the source file. Resolved by adding template before each member function definition.

### Done:
    - All tasks were completed successfully.

## Session 3: 2026-06-20
### Duration: 4 Hours
### Tasks:
    - Change the `Vector` to no use the new keyword and instead use `malloc` and `free` for memory management.
    - Fixed the old erros in the `Vector` class implementation.
### Problem Encountered:
    - Error in the `Vector` class implementation due to incorrect memory allocation. Resolved by ensuring proper use of `malloc` and `free` for dynamic memory management.

### Done:
    - Only the Implementation of `Vector` without new and delete was completed successfully. The other tasks were not completed due to the errors encountered in the `Vector` class implementation.


## Session 4: 2026-06-23
### Duration: 6 Hours
### Tasks:
    - Fix the errors that is going on in the Vector class implementation.
    - Improve the logic of the `Vector` class to handle dynamic resizing and memory management more efficiently.
    - Improve the `Vector` class to handle edge cases such as accessing elements out of bounds and resizing the vector when it reaches its capacity.
    - Improved the archietecture of the Proposal to include from a basic to a final design of the data structure library.

### Problem Encountered:
    - Non-trival datatypes are not working with the `Vector` class. Tried to resolve the issue by mannualy constructing the objects in the `Vector` class using placement new and calling the destructor explicitly. However, this approach is not working as expected and is causing memory leaks and undefined behavior.

### Done:
    - The `Vector` class implementation was improved to handle dynamic resizing and memory management more efficiently.
    - The proposal was improved also.

## Session 5: 2026-06-24
### Duration: 5 Hours
### Tasks:
    - Implemented the `LinkedList` class in `include/LinkedList.h`.
    - Added member functions such as `push_front`, `push_back`, `pop_front`, `pop_back`, `front`, `back`, `at`, `empty`, and `size`.
    - Implemented the constructor, copy constructor, assignment operator, and destructor for the `LinkedList` class.

### Problem Encountered:
    - No problems were encountered during this session.

### Done:
    - All tasks were completed successfully.

## Session 5: 2026-06-25
### Duration: 2 Hours
### Tasks:
    - Implemented Constructor and Destructor copy constructor and assignment operator for the HashMap class.
    - Removed code from the architecture proposal that is not relevant as the implemention needs to be hidden.

### Problem Encountered:
    - No problems were encountered during this session.

