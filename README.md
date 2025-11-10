[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/pAwGQi_N)

# PA4 Skeleton Code
We expect a fully functioninig command line REPL application for an inventory querying system. Feel free to modify the skeleton code as you see fit.

## How to run
`make` will compile and execute the skeleton code

## Testing

This project contains unit-style tests for the container implemented in `Headers/HashTable.hpp`. The tests are implemented using `cassert` and are compiled into a separate executable.

To run the tests:

```bash
make test
./testexe
```

Testing approach
- Scope: tests focus on the templated `HashTable<T>` container (insert, find, update, erase, size, and rehash behavior); they exercise both the `Product` value type and a primitive (`int`) to verify templating.
- Framework: simple, light-weight tests using `cassert`. Each test function checks expected behavior and will abort on failure.

Test cases
- test_insert_find_update
	- Normal case: insert a `Product` with key "k1" and verify it can be found and values match.
	- Edge case: update the same key with a new `Product` (same key) and verify that `insert` returns false (indicates update) and the stored value is updated; also verify finding a missing key returns `nullptr`.
- test_erase
	- Normal case: insert a `Product` and erase it; verify it is removed and cannot be found.
	- Edge case: attempt to erase a non-existent key and verify `erase` returns false.
- test_size_and_rehash
	- Normal case: insert 100 integer values into `HashTable<int>` (small initial bucket count) to trigger rehashing; verify `size()` equals the number inserted and all entries remain reachable.
	- Edge case: check operations on an empty table (size 0 and find returns `nullptr`).
- test_template_with_int
	- Normal case: insert and update a simple integer value to validate templated behavior.
	- Edge case: update behaviour checked by comparing return value of subsequent `insert`.

Why these cases
- They validate the core contract of the container: correctness of insert/find/update/erase and resilience across resizing.
- Using both a complex `Product` type and a primitive (`int`) ensures the template works for different value types.


