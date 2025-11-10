#pragma once

#include <string>
#include <vector>
#include <list>
#include <functional>
#include <optional>

namespace inv {

struct Product {
    // Required fields
    std::string uniqId;          // key
    std::string productName;
    std::string brandName;
    std::string category;         // joined for display (e.g., "A | B | C")
    std::vector<std::string> categories; // individual categories for indexing
    std::string listPrice;       // keep as original string (may include $ and commas)
    std::string sellingPrice;    // keep as original string
    std::string quantity;        // keep as original string

    // Optional fields
    std::string asin;
    std::string modelNumber;
    std::string productDescription; // Product Description or About Product
    std::string stock;               // Stock
};

// Templated hash table keyed by std::string
template <typename T>
class HashTable {
public:
    explicit HashTable(std::size_t bucketCount = 1'003) // prime-ish default
        : buckets_(bucketCount) {}

    bool insert(const std::string &key, const T &value) {
        auto &bucket = buckets_[indexFor(key)];
        for (auto &node : bucket) {
            if (node.key == key) {
                node.value = value; // replace existing
                return false;       // updated existing
            }
        }
        bucket.push_back(Node{key, value});
        ++size_;
        if (loadFactor() > kMaxLoadFactor) {
            rehash(buckets_.size() * 2 + 1);
        }
        return true; // inserted new
    }

    T* find(const std::string &key) {
        auto &bucket = buckets_[indexFor(key)];
        for (auto &node : bucket) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr;
    }

    const T* find(const std::string &key) const {
        const auto &bucket = buckets_[indexFor(key)];
        for (const auto &node : bucket) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr;
    }

    bool erase(const std::string &key) {
        auto &bucket = buckets_[indexFor(key)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                --size_;
                return true;
            }
        }
        return false;
    }

    std::size_t size() const { return size_; }

    std::size_t bucketCount() const { return buckets_.size(); }

    double loadFactor() const {
        if (buckets_.empty()) return 0.0;
        return static_cast<double>(size_) / static_cast<double>(buckets_.size());
    }

private:
    struct Node {
        std::string key;
        T value;
    };

    std::vector<std::list<Node>> buckets_;
    std::size_t size_ {0};
    static constexpr double kMaxLoadFactor = 0.9; // rehash threshold

    std::size_t indexFor(const std::string &key) const {
        return std::hash<std::string>{}(key) % buckets_.size();
    }

    void rehash(std::size_t newBucketCount) {
        std::vector<std::list<Node>> newBuckets(newBucketCount);
        for (auto &bucket : buckets_) {
            for (auto &node : bucket) {
                std::size_t idx = std::hash<std::string>{}(node.key) % newBucketCount;
                newBuckets[idx].push_back(std::move(node));
            }
        }
        buckets_.swap(newBuckets);
    }
};

} // namespace inv
