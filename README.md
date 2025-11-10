# Amazon Inventory Management System

A command-line inventory querying system that loads product data from a CSV file and provides fast lookup capabilities using a custom hash table implementation.

## Project Overview

This project implements an inventory management system with the following key features:
- **Custom Hash Table**: Templated hash table implementation with separate chaining for collision resolution
- **CSV Data Parser**: Robust parser that handles multi-line fields, quoted values, and data sanitization
- **Category Indexing**: Secondary index for efficient category-based queries
- **REPL Interface**: Interactive command-line interface for querying inventory

## Architecture

### Core Components

#### 1. Hash Table (`Headers/HashTable.hpp`)
A templated hash table implementation that provides O(1) average-case lookups.

**Key Features:**
- **Templated Design**: `HashTable<T>` can store any value type
- **Separate Chaining**: Uses `std::list` for collision resolution
- **Dynamic Resizing**: Automatically rehashes when load factor exceeds 0.9
- **String Keys**: Uses `std::hash<std::string>` for hashing

**API:**
- `bool insert(const std::string &key, const T &value)`: Insert or update. Returns `true` for new insertion, `false` for update.
- `T* find(const std::string &key)`: Find value by key. Returns pointer to value or `nullptr` if not found.
- `bool erase(const std::string &key)`: Remove entry. Returns `true` if erased, `false` if key didn't exist.
- `size_t size()`: Returns number of entries.
- `double loadFactor()`: Returns current load factor (size / bucket count).

#### 2. Product Data Structure (`Headers/HashTable.hpp`)
Represents a product in the inventory.

**Fields:**
- **Required**: `uniqId`, `productName`, `brandName`, `category`, `categories`, `listPrice`, `sellingPrice`, `quantity`
- **Optional**: `asin`, `modelNumber`, `productDescription`, `stock`

**Multi-Category Support:**
- Products can belong to multiple categories (separated by `|` in CSV)
- `category`: Display string showing all categories joined with `" | "`
- `categories`: Vector of individual category strings for indexing

#### 3. CSV Parser (`Headers/Parser.hpp`)
Robust parser that handles real-world CSV data from web scraping.

**Capabilities:**
- **RFC 4180-compliant**: Handles quoted fields, escaped quotes (`""`), embedded commas
- **Multi-line Records**: Detects and reads records spanning multiple lines
- **Data Sanitization**: 
  - Removes/replaces control characters (CR, LF, tabs)
  - Collapses consecutive whitespace
  - Trims leading/trailing whitespace
- **Multi-Category Extraction**: Splits category strings on `|`, trims, and deduplicates
- **Missing Data Handling**: Uses "NA" for missing categories

**Key Function:**
```cpp
bool loadCsv(const string &path, 
             HashTable<Product> &table,
             unordered_map<string, vector<string>> &categoryIndex)
```
Loads CSV, populates hash table, and builds category index in one pass.

#### 4. REPL Application (`src/main.cpp`)
Interactive command-line interface for querying inventory.

**Data Structures:**
- `g_table`: Hash table mapping Uniq ID → Product
- `g_categoryIndex`: Map of Category → list of Uniq IDs

**Commands:**
- `find <id>`: Display full details of a product by its unique ID
- `listInventory <category>`: List all products in a specific category (shows ID and name)
- `:help`: Display help information
- `:quit`: Exit the application

## How to Build and Run

### Compile and Run Main Application
```bash
make compile
./mainexe
```

### Run Tests
```bash
make test
./testexe
```

### Clean Build Artifacts
```bash
make clean
```

## Testing

This project contains unit-style tests for the hash table container implemented in `Headers/HashTable.hpp`. Tests are implemented using `cassert` and compiled into a separate executable.

### Testing Approach

**Scope**: Tests focus on the templated `HashTable<T>` container (insert, find, update, erase, size, and rehash behavior). They exercise both complex types (`Product`) and primitives (`int`) to verify template flexibility.

**Framework**: Lightweight testing using `cassert`. Each test function validates specific behavior and will abort on any assertion failure, making it easy to identify which test failed.

### Test Functions

#### Insert Operation Tests

**`test_insert_new()`**
- **Purpose**: Validates that inserting a new key-value pair returns `true` and the item can be successfully retrieved.
- **Why Chosen**: Tests the fundamental insert operation—the most common use case. Ensures basic functionality works correctly.
- **What It Tests**: 
  - Insert returns `true` for new keys
  - Inserted values can be found
  - Retrieved values match inserted values

**`test_insert_update()`**
- **Purpose**: Validates that inserting with an existing key updates the value and returns `false`.
- **Why Chosen**: Tests the update semantic of the hash table. Important for ensuring the hash table correctly handles duplicate keys by replacing values rather than creating duplicates.
- **What It Tests**:
  - Insert returns `false` when updating existing keys
  - Updated values replace old values
  - No duplicate entries are created

#### Find Operation Tests

**`test_find_missing()`**
- **Purpose**: Validates that finding a non-existent key returns `nullptr`.
- **Why Chosen**: Tests edge case behavior—what happens when searching for something that doesn't exist. Critical for preventing crashes when users query invalid inventory IDs.
- **What It Tests**:
  - Find returns `nullptr` for missing keys
  - No crashes or undefined behavior on missing lookups

#### Erase Operation Tests

**`test_erase_existing()`**
- **Purpose**: Validates that erasing an existing item removes it and returns `true`.
- **Why Chosen**: Tests the deletion operation for normal cases. Ensures items can be properly removed and are no longer findable afterward. Important for inventory management when items are discontinued.
- **What It Tests**:
  - Erase returns `true` when removing existing keys
  - Erased items cannot be found afterward
  - Hash table state is consistent after deletion

**`test_erase_nonexisting()`**
- **Purpose**: Validates that erasing a missing key returns `false` without crashing.
- **Why Chosen**: Tests edge case behavior—attempting to delete something that doesn't exist. Ensures robustness when users try to delete invalid inventory IDs.
- **What It Tests**:
  - Erase returns `false` for non-existent keys
  - No crashes or undefined behavior on invalid erase

#### Size and Rehash Tests

**`test_size_empty()`**
- **Purpose**: Validates that an empty table correctly reports size 0 and returns `nullptr` for any find operation.
- **Why Chosen**: Tests edge case—empty container behavior. Ensures the hash table handles the initial state correctly before any data is added.
- **What It Tests**:
  - Empty table reports size 0
  - Find returns `nullptr` in empty table
  - No crashes on empty table operations

**`test_size_and_rehash_preserve()`**
- **Purpose**: Validates that the hash table correctly grows (rehashes) when load factor exceeds threshold, and that all data remains accessible after rehashing.
- **Why Chosen**: Tests critical scalability feature. Hash tables must dynamically resize to maintain O(1) performance. This ensures:
  1. Rehashing is triggered appropriately (load factor > 0.9)
  2. All existing data survives the rehash operation
  3. The hash table can handle real-world data volumes (100+ items)
- **What It Tests**:
  - Inserting 100 items into a small (3-bucket) table triggers rehashing
  - Size correctly reflects number of items
  - All items remain findable with correct values after rehash
  - Hash function redistributes items across new bucket array

#### Template Functionality Tests

**`test_template_insert_update_int()`**
- **Purpose**: Validates that the templated `HashTable<T>` works correctly with simple primitive types (`int`), not just complex structs like `Product`.
- **Why Chosen**: Tests template flexibility and type-independence. Ensures the hash table is truly generic and can store any type T. Using `int` is simpler than `Product` and isolates template mechanics from complex data structure issues. Also re-tests insert/update behavior with a different type to ensure the insert-returns-false-on-update semantic works across different template instantiations.
- **What It Tests**:
  - Template compiles and works with primitive types
  - Insert/update semantics work the same for all types
  - No type-specific bugs in template implementation

### Why These Test Cases Were Chosen

1. **Comprehensive Coverage**: Tests cover all major operations (insert, find, erase) and both normal and edge cases.

2. **Real-World Scenarios**: 
   - Normal cases test typical usage patterns
   - Edge cases test error conditions users will encounter (missing items, empty state)
   - Rehash test validates scalability for production data volumes

3. **Type Safety Validation**: Testing with both `Product` (complex struct) and `int` (primitive) ensures the template is truly generic and doesn't have type-specific bugs.

4. **Minimal but Sufficient**: Each test is focused on a single aspect, making failures easy to diagnose. Together they provide confidence in the hash table's correctness without excessive redundancy.

5. **Defensive Programming**: Tests for error conditions (missing keys, empty table, duplicate insertions) ensure the code is robust and won't crash on unexpected input.

## Implementation Details

### Hash Table Collision Resolution
Uses separate chaining with `std::list<Node>` where each bucket stores a list of key-value pairs.

### Rehashing Strategy
When load factor exceeds 0.9:
1. Double bucket count and add 1: `newSize = oldSize * 2 + 1`
2. Rehash all existing entries into new bucket array
3. Swap old array with new array

### CSV Parsing Strategy
1. Read header line and build column name → index map
2. For each record:
   - Use `readRecord()` to handle multi-line quoted fields
   - Parse line into columns using `parseCsvLine()` (handles quotes and escapes)
   - Sanitize each field (remove control chars, collapse whitespace)
   - Extract and normalize categories (split on `|`, trim, dedupe)
   - Insert into hash table and update category index

## File Structure
```
├── Headers/
│   ├── HashTable.hpp       # Templated hash table + Product struct
│   └── Parser.hpp          # CSV parsing and data loading
├── src/
│   ├── main.cpp           # REPL application
│   └── tests.cpp          # Unit tests
├── Makefile               # Build configuration
├── README.md              # This file
└── marketing_sample_*.csv # Data file (10k Amazon products)
```

## Dependencies
- C++14 or later
- Standard Library only (no external dependencies)

## Future Enhancements
- Add concurrency support (thread-safe operations)
- Replace `std::list` with `forward_list` for lower memory overhead
- Add case-insensitive category matching
- Implement fuzzy search for product names
- Add performance benchmarking
- Migrate to unit test framework (Catch2/GoogleTest) for richer reporting
````


