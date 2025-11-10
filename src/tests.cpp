/**
 * Hash Table Container Tests
 * 
 * This file contains unit tests for the templated HashTable<T> container.
 * Tests use cassert for validation and will abort on any assertion failure.
 * Each test function focuses on a specific aspect of the hash table's behavior.
 */

#include <cassert>
#include <iostream>
#include <string>
#include "../Headers/HashTable.hpp"

using namespace std;

/**
 * Helper function to create a test Product with minimal required fields.
 * Used by test functions to quickly instantiate Product objects.
 * 
 * @param id Unique identifier for the product
 * @param name Product name
 * @param brand Brand name (optional, defaults to empty string)
 * @return A Product instance with test data
 */
inv::Product makeProduct(const string &id, const string &name, const string &brand = "") {
    inv::Product p;
    p.uniqId = id;
    p.productName = name;
    p.brandName = brand;
    p.category = "Test";
    p.categories = {"Test"};
    p.listPrice = "$1.00";
    p.sellingPrice = "$0.99";
    p.quantity = "1";
    return p;
}

// ============================================================================
// INSERT OPERATION TESTS
// ============================================================================

/**
 * Test: Insert a new key-value pair into the hash table
 * 
 * Purpose: Validates that inserting a new item returns true and the item
 *          can be successfully retrieved.
 * 
 * Why chosen: Tests the fundamental insert operation - the most common use case.
 *             Ensures basic functionality works correctly.
 */
void test_insert_new() {
    inv::HashTable<inv::Product> ht(3);  // Small table to test basic insertion
    auto p1 = makeProduct("k1", "First");
    bool ins = ht.insert(p1.uniqId, p1);
    assert(ins == true);  // Should return true for new insertion
    auto *f = ht.find("k1");
    assert(f != nullptr && f->productName == "First");  // Verify retrieval
}

/**
 * Test: Update an existing key with a new value
 * 
 * Purpose: Validates that inserting with an existing key updates the value
 *          and returns false (indicating update, not new insertion).
 * 
 * Why chosen: Tests the update semantic of the hash table. Important for
 *             ensuring the hash table correctly handles duplicate keys by
 *             replacing values rather than creating duplicates.
 */
void test_insert_update() {
    inv::HashTable<inv::Product> ht(3);
    auto p1 = makeProduct("k1", "First");
    ht.insert(p1.uniqId, p1);
    
    // Update the same key with different data
    inv::Product p1b = p1;
    p1b.productName = "First-updated";
    bool ins2 = ht.insert(p1b.uniqId, p1b);
    assert(ins2 == false);  // Should return false for update
    
    auto *f = ht.find("k1");
    assert(f != nullptr && f->productName == "First-updated");  // Verify value was updated
}

// ============================================================================
// FIND OPERATION TESTS
// ============================================================================

/**
 * Test: Search for a non-existent key
 * 
 * Purpose: Validates that finding a missing key returns nullptr.
 * 
 * Why chosen: Tests edge case behavior - what happens when searching for
 *             something that doesn't exist. Critical for preventing crashes
 *             when users query invalid inventory IDs.
 */
void test_find_missing() {
    inv::HashTable<inv::Product> ht(3);
    assert(ht.find("missing") == nullptr);  // Should return nullptr for missing keys
}

// ============================================================================
// ERASE OPERATION TESTS
// ============================================================================

/**
 * Test: Erase an existing key from the hash table
 * 
 * Purpose: Validates that erasing an existing item removes it and returns true.
 * 
 * Why chosen: Tests the deletion operation for normal cases. Ensures items
 *             can be properly removed and are no longer findable afterward.
 *             Important for inventory management when items are discontinued.
 */
void test_erase_existing() {
    inv::HashTable<inv::Product> ht(5);
    auto p1 = makeProduct("e1", "EraseMe");
    ht.insert(p1.uniqId, p1);
    
    assert(ht.find("e1") != nullptr);  // Verify item exists before erasure
    bool erased = ht.erase("e1");
    assert(erased == true);  // Should return true when item is erased
    assert(ht.find("e1") == nullptr);  // Verify item is gone after erasure
}

/**
 * Test: Attempt to erase a non-existent key
 * 
 * Purpose: Validates that erasing a missing key returns false without crashing.
 * 
 * Why chosen: Tests edge case behavior - attempting to delete something that
 *             doesn't exist. Ensures robustness when users try to delete
 *             invalid inventory IDs.
 */
void test_erase_nonexisting() {
    inv::HashTable<inv::Product> ht(5);
    assert(ht.erase("nope") == false);  // Should return false for non-existent keys
}

// ============================================================================
// SIZE AND REHASH TESTS
// ============================================================================

/**
 * Test: Query size and find on an empty hash table
 * 
 * Purpose: Validates that an empty table correctly reports size 0 and
 *          returns nullptr for any find operation.
 * 
 * Why chosen: Tests edge case - empty container behavior. Ensures the hash
 *             table handles the initial state correctly before any data is added.
 */
void test_size_empty() {
    inv::HashTable<int> ht2(7);
    assert(ht2.size() == 0);  // Empty table should report size 0
    assert(ht2.find("no") == nullptr);  // Find should return nullptr in empty table
}

/**
 * Test: Insert many items to trigger rehashing and verify data preservation
 * 
 * Purpose: Validates that the hash table correctly grows (rehashes) when load
 *          factor exceeds threshold, and that all data remains accessible after
 *          rehashing.
 * 
 * Why chosen: Tests critical scalability feature. Hash tables must dynamically
 *             resize to maintain O(1) performance. This ensures:
 *             1. Rehashing is triggered appropriately (load factor > 0.9)
 *             2. All existing data survives the rehash operation
 *             3. The hash table can handle real-world data volumes (100+ items)
 *             
 *             Uses int type to simplify testing and verify template works with
 *             different value types.
 */
void test_size_and_rehash_preserve() {
    inv::HashTable<int> ht(3);  // Start with small table to force rehashing
    const int N = 100;
    
    // Insert 100 items - should trigger multiple rehashes
    for (int i = 0; i < N; ++i) {
        string key = "k" + to_string(i);
        bool inserted = ht.insert(key, i);
        assert(inserted == true);  // Each insertion should be new
    }
    
    assert((int)ht.size() == N);  // Verify size is correct after all insertions
    
    // Verify all items are still accessible after rehashing
    for (int i = 0; i < N; ++i) {
        string key = "k" + to_string(i);
        auto *v = ht.find(key);
        assert(v != nullptr && *v == i);  // Each item should be findable with correct value
    }
}

// ============================================================================
// TEMPLATE FUNCTIONALITY TESTS
// ============================================================================

/**
 * Test: Verify hash table template works with primitive types (int)
 * 
 * Purpose: Validates that the templated HashTable<T> works correctly with
 *          simple primitive types, not just complex structs like Product.
 * 
 * Why chosen: Tests template flexibility and type-independence. Ensures the
 *             hash table is truly generic and can store any type T. Using int
 *             is simpler than Product and isolates template mechanics from
 *             complex data structure issues.
 *             
 *             Also re-tests insert/update behavior with a different type to
 *             ensure the insert-returns-false-on-update semantic works across
 *             different template instantiations.
 */
void test_template_insert_update_int() {
    inv::HashTable<int> ht(5);
    
    // Test insertion with primitive type
    bool ins = ht.insert("one", 1);
    assert(ins == true);  // Should return true for new insertion
    auto *v = ht.find("one");
    assert(v != nullptr && *v == 1);  // Verify correct value stored
    
    // Test update with primitive type
    bool ins2 = ht.insert("one", 11);
    assert(ins2 == false);  // Should return false for update
    v = ht.find("one");
    assert(v != nullptr && *v == 11);  // Verify value was updated
}

/**
 * Main test runner
 * 
 * Executes all test functions in sequence. Each test is independent and
 * creates its own hash table instance. If any assertion fails, the program
 * will abort with an error message indicating which test failed.
 */
int main() {
    cout << "Running container tests...\n";
    
    test_insert_new();
    cout << " test_insert_new passed\n";
    
    test_insert_update();
    cout << " test_insert_update passed\n";
    
    test_find_missing();
    cout << " test_find_missing passed\n";
    
    test_erase_existing();
    cout << " test_erase_existing passed\n";
    
    test_erase_nonexisting();
    cout << " test_erase_nonexisting passed\n";
    
    test_size_empty();
    cout << " test_size_empty passed\n";
    
    test_size_and_rehash_preserve();
    cout << " test_size_and_rehash_preserve passed\n";
    
    test_template_insert_update_int();
    cout << " test_template_insert_update_int passed\n";
    
    cout << "All tests passed.\n";
    return 0;
}
