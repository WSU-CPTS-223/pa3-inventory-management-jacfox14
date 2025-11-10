/**
 * Parser.hpp
 * 
 * CSV parsing and data sanitization utilities for inventory management system.
 * 
 * This header provides RFC 4180-compliant CSV parsing with multi-line record support,
 * data sanitization functions, and specialized handling for multi-category product data.
 * 
 * Key Features:
 * - RFC 4180-compliant CSV parsing with quote handling and escape sequences
 * - Multi-line record support (handles newlines within quoted fields)
 * - Data sanitization (whitespace normalization, CR/LF handling)
 * - Multi-category extraction and deduplication (pipe-delimited categories)
 * - Flexible header mapping (handles arbitrary column orders)
 * 
 * Design Decisions:
 * - All parsing functions in 'detail' namespace (internal implementation)
 * - Header-only implementation for simplicity and inlining
 * - Robust error handling (missing columns default to empty strings)
 * - Category index built during load for O(1) category lookups
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cctype>
#include <sstream>
#include <set>
#include "HashTable.hpp"

namespace inv {

// Detail namespace: Internal implementation details, not part of public API
namespace detail {

/**
 * ltrim - Remove leading whitespace from string
 * 
 * Removes all leading whitespace characters (space, tab, newline, etc.)
 * from the beginning of the string.
 * 
 * @param s Input string to trim
 * @return String with leading whitespace removed
 * 
 * Time Complexity: O(n) where n = string length
 */
inline std::string ltrim(const std::string &s) {
    size_t i = 0; while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; return s.substr(i);
}

/**
 * rtrim - Remove trailing whitespace from string
 * 
 * Removes all trailing whitespace characters from the end of the string.
 * 
 * @param s Input string to trim
 * @return String with trailing whitespace removed
 * 
 * Time Complexity: O(n) where n = string length
 */
inline std::string rtrim(const std::string &s) {
    if (s.empty()) return s;
    size_t i = s.size();
    while (i > 0 && std::isspace(static_cast<unsigned char>(s[i-1]))) --i;
    return s.substr(0, i);
}

/**
 * trim - Remove leading and trailing whitespace from string
 * 
 * Convenience function that combines ltrim and rtrim.
 * 
 * @param s Input string to trim
 * @return String with leading and trailing whitespace removed
 * 
 * Time Complexity: O(n) where n = string length
 */
inline std::string trim(const std::string &s) { return rtrim(ltrim(s)); }

/**
 * sanitize - Clean and normalize text data
 * 
 * Performs comprehensive text sanitization:
 * 1. Converts CR/LF/TAB to single spaces
 * 2. Collapses consecutive whitespace into single space
 * 3. Trims leading and trailing whitespace
 * 
 * This is essential for CSV data that may contain multi-line fields,
 * inconsistent whitespace, or different line ending formats (Windows vs Unix).
 * 
 * Example:
 *   "  Hello\r\n  World  " → "Hello World"
 * 
 * @param s Raw string to sanitize
 * @return Cleaned and normalized string
 * 
 * Time Complexity: O(n) where n = string length
 */
inline std::string sanitize(const std::string &s) {
    // Replace CR/LF with space, collapse consecutive spaces, and trim
    std::string out; out.reserve(s.size());
    char prev = '\0';
    for (char c : s) {
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (prev == ' ') continue; // collapse
            out.push_back(' ');
            prev = ' ';
        } else {
            out.push_back(c);
            prev = c;
        }
    }
    return trim(out);
}

/**
 * extractCategories - Parse and deduplicate multi-category data
 * 
 * Extracts multiple categories from pipe-delimited string (e.g., "Cat1 | Cat2 | Cat1").
 * 
 * Algorithm:
 * 1. Split on '|' delimiter
 * 2. Trim whitespace from each category
 * 3. Remove duplicates (preserves first occurrence order)
 * 4. Filter out empty categories
 * 5. If no valid categories found, return {"NA"}
 * 
 * This handles the common e-commerce pattern of products belonging to multiple
 * categories, which enables efficient category-based search and filtering.
 * 
 * Example:
 *   "Electronics | Computers | Electronics | " → ["Electronics", "Computers"]
 *   "" → ["NA"]
 * 
 * @param raw Pipe-delimited category string
 * @return Vector of unique, trimmed category names
 * 
 * Time Complexity: O(n*m) where n = number of categories, m = avg category length
 */
inline std::vector<std::string> extractCategories(const std::string &raw) {
    // Split on '|' and trim parts; dedupe; if none, return {"NA"}
    std::vector<std::string> parts; parts.reserve(8);
    std::string cur;
    for (char c : raw) {
        if (c == '|') { parts.push_back(trim(cur)); cur.clear(); }
        else { cur.push_back(c); }
    }
    parts.push_back(trim(cur));
    std::set<std::string> uniq;
    std::vector<std::string> cleaned;
    for (auto &p : parts) {
        if (p.empty()) continue;
        if (uniq.insert(p).second) cleaned.push_back(p);
    }
    if (cleaned.empty()) cleaned.push_back("NA");
    return cleaned;
}

/**
 * joinCategories - Convert category vector to display string
 * 
 * Joins multiple categories into a pipe-delimited string for display.
 * This is the inverse operation of extractCategories().
 * 
 * Example:
 *   ["Electronics", "Computers"] → "Electronics | Computers"
 * 
 * @param cats Vector of category names
 * @return Pipe-delimited string representation
 * 
 * Time Complexity: O(n*m) where n = number of categories, m = avg length
 */
inline std::string joinCategories(const std::vector<std::string> &cats) {
    std::ostringstream oss; bool first = true;
    for (const auto &p : cats) {
        if (!first) oss << " | ";
        oss << p; first = false;
    }
    return oss.str();
}

/**
 * cleanPrice - Sanitize price data
 * 
 * Cleans price strings by removing line breaks and extra spaces while
 * preserving currency symbols and numeric formatting.
 * 
 * Example:
 *   " $ 29.99 " → "$29.99"
 *   "£ 15. 00" → "£15.00"
 * 
 * @param raw Raw price string
 * @return Cleaned price string (still as text, not numeric)
 * 
 * Note: Prices kept as strings to preserve currency symbols and formatting
 * 
 * Time Complexity: O(n) where n = string length
 */
inline std::string cleanPrice(const std::string &raw) {
    // Keep as string but remove CR/LF and trim; leave currency symbol if present
    std::string s = sanitize(raw);
    // Remove spaces inside price
    std::string out; out.reserve(s.size());
    for (char c : s) { if (c != ' ') out.push_back(c); }
    return out;
}

/**
 * isBalancedQuotes - Check if CSV line has balanced quotes
 * 
 * Determines whether a CSV line has properly balanced quotes according to
 * RFC 4180 rules. This is essential for multi-line record detection.
 * 
 * RFC 4180 Quote Rules:
 * - Fields containing commas, newlines, or quotes must be quoted
 * - Quotes inside quoted fields are escaped by doubling ("")
 * - A record is complete when quotes are balanced
 * 
 * Algorithm:
 * - Toggle inQuotes state for each unescaped quote
 * - Skip escaped quotes ("") without toggling state
 * - Line is balanced if not inside quotes at end
 * 
 * Example:
 *   'field1,"field2,value",field3' → balanced (true)
 *   'field1,"field2' → unbalanced (false, needs continuation)
 *   'field1,"He said ""Hello""",field3' → balanced (escaped quotes)
 * 
 * @param s CSV line to check
 * @return true if quotes are balanced, false if line needs continuation
 * 
 * Time Complexity: O(n) where n = string length
 */
inline bool isBalancedQuotes(const std::string &s) {
    // Count quotes not escaped by another quote; for RFC4180 we'll consider doubling inside a quoted field
    size_t cnt = 0; bool inQuotes = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '"') {
            if (inQuotes && i + 1 < s.size() && s[i+1] == '"') { ++i; /* escaped */ }
            else { inQuotes = !inQuotes; ++cnt; }
        }
    }
    return !inQuotes; // not inside a quote at end
}

/**
 * readRecord - Read complete CSV record (handles multi-line records)
 * 
 * Reads a complete CSV record from the input stream, handling multi-line
 * records where fields contain embedded newlines within quotes.
 * 
 * Algorithm:
 * 1. Read first line
 * 2. Check if quotes are balanced
 * 3. If unbalanced, continue reading lines until quotes balance
 * 4. Preserve newlines within the record
 * 
 * This is critical for CSV files with description fields that may contain
 * newlines, like product descriptions or customer reviews.
 * 
 * Example:
 *   Line 1: field1,"This is a
 *   Line 2: multi-line field",field3
 *   → Returns: 'field1,"This is a\nmulti-line field",field3'
 * 
 * @param in Input stream to read from
 * @param record Output string to store complete record
 * @return true if record was read, false on EOF
 * 
 * Time Complexity: O(n) where n = total record length
 */
inline bool readRecord(std::istream &in, std::string &record) {
    record.clear();
    std::string line; if (!std::getline(in, line)) return false;
    record = line;
    while (!isBalancedQuotes(record)) {
        std::string extra; if (!std::getline(in, extra)) break; // best effort
        record.push_back('\n');
        record += extra;
    }
    return true;
}

/**
 * parseCsvLine - Parse CSV record into fields
 * 
 * Parses a complete CSV record (possibly multi-line) into individual fields
 * following RFC 4180 rules.
 * 
 * RFC 4180 Parsing Rules:
 * - Fields separated by commas
 * - Fields containing commas/newlines/quotes must be quoted
 * - Quotes inside quoted fields are escaped by doubling ("")
 * - Leading/trailing spaces outside quotes are preserved
 * 
 * Algorithm:
 * - Track inQuotes state
 * - When outside quotes: comma = field separator, quote = start quoted field
 * - When inside quotes: only "" escapes to single quote, everything else literal
 * - Build current field character by character
 * 
 * Example:
 *   'a,b,"c,d",e' → ["a", "b", "c,d", "e"]
 *   '"He said ""Hi""","next"' → ['He said "Hi"', "next"]
 * 
 * @param line Complete CSV record to parse
 * @return Vector of field values (quotes and escapes removed)
 * 
 * Time Complexity: O(n) where n = record length
 */
inline std::vector<std::string> parseCsvLine(const std::string &line) {
    std::vector<std::string> result; std::string cur; bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i+1] == '"') { cur.push_back('"'); ++i; }
                else { inQuotes = false; }
            } else { cur.push_back(c); }
        } else {
            if (c == '"') inQuotes = true;
            else if (c == ',') { result.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
    }
    result.push_back(cur);
    return result;
}

/**
 * HeaderMap - Maps column names to indices
 * 
 * Provides flexible header mapping to handle CSVs with arbitrary column orders.
 * This allows the parser to work with different CSV formats without hardcoding
 * column positions.
 * 
 * Design: Returns -1 (max size_t) for missing columns, which safeGet() handles
 * by returning empty string.
 */
struct HeaderMap {
    std::unordered_map<std::string, size_t> idx;
    
    /**
     * Get column index by name
     * @param name Column name to look up
     * @return Column index, or -1 if not found
     */
    size_t get(const std::string &name) const { auto it = idx.find(name); return it == idx.end() ? static_cast<size_t>(-1) : it->second; }
};

/**
 * buildHeader - Parse CSV header line into HeaderMap
 * 
 * Parses the header line and builds a mapping from column names to indices.
 * Column names are trimmed to handle inconsistent spacing.
 * 
 * Example:
 *   "Uniq Id, Product Name, Category" → {"Uniq Id": 0, "Product Name": 1, "Category": 2}
 * 
 * @param headerLine First line of CSV file
 * @return HeaderMap for efficient column lookups
 * 
 * Time Complexity: O(n) where n = number of columns
 */
inline HeaderMap buildHeader(const std::string &headerLine) {
    HeaderMap h;
    auto cols = parseCsvLine(headerLine);
    for (size_t i = 0; i < cols.size(); ++i) {
        h.idx[trim(cols[i])] = i;
    }
    return h;
}

/**
 * safeGet - Safely extract field from CSV row
 * 
 * Gets field value at given index with bounds checking and missing column handling.
 * Returns empty string if index is -1 (column not found) or out of bounds.
 * 
 * This provides graceful degradation when CSV is missing expected columns.
 * 
 * @param row Parsed CSV row
 * @param idx Column index (may be -1 if column missing)
 * @return Field value, or empty string if index invalid
 * 
 * Time Complexity: O(1)
 */
inline std::string safeGet(const std::vector<std::string> &row, size_t idx) { return (idx == static_cast<size_t>(-1) || idx >= row.size()) ? std::string() : row[idx]; }

} // namespace detail

/**
 * loadCsv - Load products from CSV file into hash table
 * 
 * Main CSV loading function. Reads product data from CSV file, parses and sanitizes
 * all fields, populates the hash table with Product objects, and builds the category
 * index for efficient category-based searches.
 * 
 * Algorithm:
 * 1. Open CSV file and parse header line
 * 2. Build HeaderMap to handle arbitrary column order
 * 3. For each record:
 *    a. Read complete record (handles multi-line fields)
 *    b. Parse into fields
 *    c. Extract and sanitize all product fields
 *    d. Handle multi-category extraction (pipe-delimited)
 *    e. Insert into hash table with uniqId as key
 *    f. Add to category index for each category
 * 4. Skip records with empty/missing uniqId
 * 
 * Field Mapping:
 * - Required: Uniq Id (key), Product Name, Brand Name, Category
 * - Pricing: List Price, Selling Price
 * - Inventory: Quantity, Stock
 * - Optional: Asin, Model Number, Product Description, About Product
 * 
 * Data Transformations:
 * - All text fields: sanitize() - removes CR/LF, collapses whitespace
 * - Price fields: cleanPrice() - removes spaces, preserves currency
 * - Category field: extractCategories() - splits on '|', deduplicates
 * - Missing columns: safeGet() returns empty string (graceful degradation)
 * 
 * Category Index:
 * - Maps category name → list of product IDs
 * - Enables O(1) category lookup + O(k) product retrieval (k = products in category)
 * - Products in multiple categories appear in multiple index entries
 * 
 * Error Handling:
 * - Returns false if file cannot be opened
 * - Skips records with missing uniqId (primary key required)
 * - Empty/missing optional fields default to empty string
 * 
 * @param path Path to CSV file
 * @param table Hash table to populate with products
 * @param categoryIndex Category index to build (category → product IDs)
 * @return true if file loaded successfully, false on file open error
 * 
 * Time Complexity: O(n*m) where n = number of records, m = avg record size
 * Space Complexity: O(n*k) where k = avg categories per product
 */
inline bool loadCsv(const std::string &path, HashTable<Product> &table, std::unordered_map<std::string, std::vector<std::string>> &categoryIndex) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::string headerLine; if (!std::getline(in, headerLine)) return false;
    auto H = detail::buildHeader(headerLine);

    size_t count = 0;
    std::string rec;
    while (detail::readRecord(in, rec)) {
        if (rec.empty()) continue;
        auto cols = detail::parseCsvLine(rec);
        Product p;
        
        // Required fields
        p.uniqId = detail::sanitize(detail::safeGet(cols, H.get("Uniq Id")));
        if (p.uniqId.empty()) continue; // Skip records without primary key
        p.productName = detail::sanitize(detail::safeGet(cols, H.get("Product Name")));
        p.brandName = detail::sanitize(detail::safeGet(cols, H.get("Brand Name")));
        
        // Multi-category handling
        {
            std::string rawCat = detail::sanitize(detail::safeGet(cols, H.get("Category")));
            p.categories = detail::extractCategories(rawCat);
            p.category = detail::joinCategories(p.categories); // for display
        }
        
        // Pricing and inventory
        p.listPrice = detail::cleanPrice(detail::safeGet(cols, H.get("List Price")));
        p.sellingPrice = detail::cleanPrice(detail::safeGet(cols, H.get("Selling Price")));
        p.quantity = detail::sanitize(detail::safeGet(cols, H.get("Quantity")));
        
        // Optional fields
        p.asin = detail::sanitize(detail::safeGet(cols, H.get("Asin")));
        p.modelNumber = detail::sanitize(detail::safeGet(cols, H.get("Model Number")));
        p.productDescription = detail::sanitize(detail::safeGet(cols, H.get("Product Description")));
        if (p.productDescription.empty()) p.productDescription = detail::sanitize(detail::safeGet(cols, H.get("About Product")));
        p.stock = detail::sanitize(detail::safeGet(cols, H.get("Stock")));

        // Insert into hash table
        table.insert(p.uniqId, p);
        
        // Build category index for efficient category searches
        for (const auto &cat : p.categories) {
            categoryIndex[cat].push_back(p.uniqId);
        }
        ++count;
    }
    return true;
}

} // namespace inv
