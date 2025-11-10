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

namespace detail {

inline std::string ltrim(const std::string &s) {
    size_t i = 0; while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; return s.substr(i);
}
inline std::string rtrim(const std::string &s) {
    if (s.empty()) return s;
    size_t i = s.size();
    while (i > 0 && std::isspace(static_cast<unsigned char>(s[i-1]))) --i;
    return s.substr(0, i);
}
inline std::string trim(const std::string &s) { return rtrim(ltrim(s)); }

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

inline std::string joinCategories(const std::vector<std::string> &cats) {
    std::ostringstream oss; bool first = true;
    for (const auto &p : cats) {
        if (!first) oss << " | ";
        oss << p; first = false;
    }
    return oss.str();
}

inline std::string cleanPrice(const std::string &raw) {
    // Keep as string but remove CR/LF and trim; leave currency symbol if present
    std::string s = sanitize(raw);
    // Remove spaces inside price
    std::string out; out.reserve(s.size());
    for (char c : s) { if (c != ' ') out.push_back(c); }
    return out;
}

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

struct HeaderMap {
    std::unordered_map<std::string, size_t> idx;
    size_t get(const std::string &name) const { auto it = idx.find(name); return it == idx.end() ? static_cast<size_t>(-1) : it->second; }
};

inline HeaderMap buildHeader(const std::string &headerLine) {
    HeaderMap h;
    auto cols = parseCsvLine(headerLine);
    for (size_t i = 0; i < cols.size(); ++i) {
        h.idx[trim(cols[i])] = i;
    }
    return h;
}

inline std::string safeGet(const std::vector<std::string> &row, size_t idx) { return (idx == static_cast<size_t>(-1) || idx >= row.size()) ? std::string() : row[idx]; }

} // namespace detail

// Load products from CSV, keeping only the needed fields and cleaning their content
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
        p.uniqId = detail::sanitize(detail::safeGet(cols, H.get("Uniq Id")));
        if (p.uniqId.empty()) continue;
        p.productName = detail::sanitize(detail::safeGet(cols, H.get("Product Name")));
        p.brandName = detail::sanitize(detail::safeGet(cols, H.get("Brand Name")));
        {
            std::string rawCat = detail::sanitize(detail::safeGet(cols, H.get("Category")));
            p.categories = detail::extractCategories(rawCat);
            p.category = detail::joinCategories(p.categories); // for display
        }
        p.listPrice = detail::cleanPrice(detail::safeGet(cols, H.get("List Price")));
        p.sellingPrice = detail::cleanPrice(detail::safeGet(cols, H.get("Selling Price")));
        p.quantity = detail::sanitize(detail::safeGet(cols, H.get("Quantity")));
        // Optional
        p.asin = detail::sanitize(detail::safeGet(cols, H.get("Asin")));
        p.modelNumber = detail::sanitize(detail::safeGet(cols, H.get("Model Number")));
        p.productDescription = detail::sanitize(detail::safeGet(cols, H.get("Product Description")));
        if (p.productDescription.empty()) p.productDescription = detail::sanitize(detail::safeGet(cols, H.get("About Product")));
        p.stock = detail::sanitize(detail::safeGet(cols, H.get("Stock")));

        table.insert(p.uniqId, p);
        for (const auto &cat : p.categories) {
            categoryIndex[cat].push_back(p.uniqId);
        }
        ++count;
    }
    return true;
}

} // namespace inv
