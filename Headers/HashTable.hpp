/**
 * Hash Table and Product Data Structures
 * 
 * This file contains:
 * 1. Product struct - represents an inventory item
 * 2. HashTable<T> - templated hash table with string keys
 * 
 * The hash table uses separate chaining for collision resolution and
 * automatically resizes when load factor exceeds threshold.
 */

#pragma once

#include <string>
#include <vector>
#include <list>
#include <functional>
#include <optional>

namespace inv {

/**
 * Product - Represents an item in the inventory
 * 
 * Stores both required and optional product information extracted from CSV.
 * Supports multi-category classification where products can belong to
 * multiple categories simultaneously.
 * 
 * Design Notes:
 * - Prices stored as strings to preserve original formatting ($, commas, etc.)
 * - Categories stored in two forms:
 *   1. `category`: Human-readable joined string for display
 *   2. `categories`: Vector of individual categories for indexing
 */
struct Product {
    // Required fields - core product information
    std::string uniqId;          // Unique identifier (used as hash table key)
    std::string productName;     // Product display name
    std::string brandName;       // Manufacturer/brand
    std::string category;        // Joined category string for display (e.g., "Electronics | Computers")
    std::vector<std::string> categories; // Individual category strings for indexing
    std::string listPrice;       // Original price (stored as string with $ and formatting)
    std::string sellingPrice;    // Current sale price (stored as string)
    std::string quantity;        // Available quantity (stored as string)

    // Optional fields - additional product details (may be empty)
    std::string asin;            // Amazon Standard Identification Number
    std::string modelNumber;     // Manufacturer model number
    std::string productDescription; // Detailed product description
    std::string stock;           // Stock status/availability
};

/**
 * HashTable<T> - Templated hash table with string keys
 * 
 * A hash table implementation that maps string keys to values of any type T.
 * Uses separate chaining (linked lists) for collision resolution and
 * automatically resizes when load factor exceeds 0.9 to maintain O(1)
 * average-case performance.
 * 
 * Design Decisions:
 * - Key Type: Fixed to std::string (common use case for this application)
 * - Value Type: Template parameter T (allows flexibility)
 * - Collision Resolution: Separate chaining with std::list
 * - Hash Function: std::hash<std::string> from standard library
 * - Load Factor Threshold: 0.9 (balances space vs. time efficiency)
 * - Resize Strategy: Double size + 1 when threshold exceeded
 * 
 * Time Complexity:
 * - Insert: O(1) average, O(n) worst-case, amortized O(1) with rehashing
 * - Find: O(1) average, O(n) worst-case
 * - Erase: O(1) average, O(n) worst-case
 * - Rehash: O(n) where n is the number of entries
 * 
 * Space Complexity: O(n + m) where n is entries, m is bucket count
 */
template <typename T>
class HashTable {
public:
    /**
     * Constructor - Initialize hash table with specified bucket count
     * 
     * @param bucketCount Initial number of buckets (default: 1003)
     *                    Using a prime-ish number helps distribute hash values
     */
    explicit HashTable(std::size_t bucketCount = 1'003)
        : buckets_(bucketCount) {}

    /**
     * Insert or update a key-value pair
     * 
     * If the key already exists, updates the value and returns false.
     * If the key is new, inserts it and returns true.
     * Automatically triggers rehashing if load factor exceeds threshold.
     * 
     * @param key String key to insert/update
     * @param value Value to associate with the key
     * @return true if new entry was inserted, false if existing entry was updated
     * 
     * Time Complexity: O(1) average, O(n) if rehashing triggered
     */
    bool insert(const std::string &key, const T &value) {
        auto &bucket = buckets_[indexFor(key)];
        
        // Check if key already exists - if so, update it
        for (auto &node : bucket) {
            if (node.key == key) {
                node.value = value; // Replace existing value
                return false;       // Indicate update (not new insertion)
            }
        }
        
        // Key doesn't exist - add new entry
        bucket.push_back(Node{key, value});
        ++size_;
        
        // Check if we need to rehash to maintain performance
        if (loadFactor() > kMaxLoadFactor) {
            rehash(buckets_.size() * 2 + 1);
        }
        return true; // Indicate new insertion
    }

    /**
     * Find a value by key (mutable version)
     * 
     * @param key String key to search for
     * @return Pointer to value if found, nullptr if not found
     * 
     * Time Complexity: O(1) average, O(n) worst-case
     */
    T* find(const std::string &key) {
        auto &bucket = buckets_[indexFor(key)];
        for (auto &node : bucket) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr; // Key not found
    }

    /**
     * Find a value by key (const version)
     * 
     * @param key String key to search for
     * @return Const pointer to value if found, nullptr if not found
     * 
     * Time Complexity: O(1) average, O(n) worst-case
     */
    const T* find(const std::string &key) const {
        const auto &bucket = buckets_[indexFor(key)];
        for (const auto &node : bucket) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr; // Key not found
    }

    /**
     * Remove a key-value pair from the hash table
     * 
     * @param key String key to remove
     * @return true if key was found and removed, false if key didn't exist
     * 
     * Time Complexity: O(1) average, O(n) worst-case
     */
    bool erase(const std::string &key) {
        auto &bucket = buckets_[indexFor(key)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                --size_;
                return true; // Found and erased
            }
        }
        return false; // Key not found
    }

    /**
     * Get the number of key-value pairs in the hash table
     * 
     * @return Number of entries
     * 
     * Time Complexity: O(1)
     */
    std::size_t size() const { return size_; }

    /**
     * Get the current number of buckets
     * 
     * @return Number of buckets in the underlying array
     * 
     * Time Complexity: O(1)
     */
    std::size_t bucketCount() const { return buckets_.size(); }

    /**
     * Calculate current load factor
     * 
     * Load factor = number of entries / number of buckets
     * Higher load factor means more collisions and slower operations.
     * 
     * @return Load factor (0.0 to infinity, typically kept below 1.0)
     * 
     * Time Complexity: O(1)
     */
    double loadFactor() const {
        if (buckets_.empty()) return 0.0;
        return static_cast<double>(size_) / static_cast<double>(buckets_.size());
    }

private:
    /**
     * Node - Internal storage structure for key-value pairs
     * Each bucket contains a linked list of nodes
     */
    struct Node {
        std::string key;
        T value;
    };

    // Hash table storage: vector of buckets, each bucket is a list of nodes
    std::vector<std::list<Node>> buckets_;
    
    // Current number of key-value pairs stored
    std::size_t size_ {0};
    
    // Maximum load factor before triggering rehash
    // 0.9 chosen as a balance: high enough for space efficiency,
    // low enough to keep collision chains short
    static constexpr double kMaxLoadFactor = 0.9;

    /**
     * Compute bucket index for a given key
     * Uses std::hash and modulo to map keys to bucket indices
     * 
     * @param key String key to hash
     * @return Bucket index (0 to buckets_.size() - 1)
     * 
     * Time Complexity: O(1)
     */
    std::size_t indexFor(const std::string &key) const {
        return std::hash<std::string>{}(key) % buckets_.size();
    }

    /**
     * Rehash all entries into a new larger bucket array
     * 
     * Called automatically when load factor exceeds threshold.
     * Creates a new bucket array, rehashes all existing entries into it,
     * then swaps the old array with the new one.
     * 
     * @param newBucketCount New number of buckets (typically 2*old + 1)
     * 
     * Time Complexity: O(n) where n is the number of entries
     */
    void rehash(std::size_t newBucketCount) {
        std::vector<std::list<Node>> newBuckets(newBucketCount);
        
        // Rehash all existing entries into new bucket array
        for (auto &bucket : buckets_) {
            for (auto &node : bucket) {
                // Recompute bucket index with new bucket count
                std::size_t idx = std::hash<std::string>{}(node.key) % newBucketCount;
                newBuckets[idx].push_back(std::move(node));
            }
        }
        
        // Replace old buckets with new buckets
        buckets_.swap(newBuckets);
        // Old buckets automatically destroyed when newBuckets goes out of scope
    }
};

} // namespace inv
