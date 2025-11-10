#include <cassert>
#include <iostream>
#include <string>
#include "../Headers/HashTable.hpp"

using namespace std;

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

void test_insert_find_update() {
    inv::HashTable<inv::Product> ht(3);
    auto p1 = makeProduct("k1", "First");
    bool ins = ht.insert(p1.uniqId, p1);
    assert(ins == true); // inserted new
    auto *f = ht.find("k1");
    assert(f != nullptr);
    assert(f->productName == "First");

    // update existing key
    inv::Product p1b = p1;
    p1b.productName = "First-updated";
    bool ins2 = ht.insert(p1b.uniqId, p1b);
    assert(ins2 == false); // updated existing
    f = ht.find("k1");
    assert(f != nullptr && f->productName == "First-updated");

    // edge: find missing
    assert(ht.find("missing") == nullptr);
}

void test_erase() {
    inv::HashTable<inv::Product> ht(5);
    auto p1 = makeProduct("e1", "EraseMe");
    ht.insert(p1.uniqId, p1);
    assert(ht.find("e1") != nullptr);
    bool erased = ht.erase("e1");
    assert(erased == true);
    assert(ht.find("e1") == nullptr);

    // edge: erase non-existing returns false
    assert(ht.erase("nope") == false);
}

void test_size_and_rehash() {
    // normal: insert several elements and ensure size grows and items remain accessible
    inv::HashTable<int> ht(3);
    const int N = 100;
    for (int i = 0; i < N; ++i) {
        string key = "k" + to_string(i);
        bool inserted = ht.insert(key, i);
        assert(inserted == true);
    }
    assert((int)ht.size() == N);
    // all present
    for (int i = 0; i < N; ++i) {
        string key = "k" + to_string(i);
        auto *v = ht.find(key);
        assert(v != nullptr && *v == i);
    }

    // edge: empty table
    inv::HashTable<int> ht2(7);
    assert(ht2.size() == 0);
    assert(ht2.find("no") == nullptr);
}

void test_template_with_int() {
    inv::HashTable<int> ht(5);
    bool ins = ht.insert("one", 1);
    assert(ins == true);
    auto *v = ht.find("one");
    assert(v != nullptr && *v == 1);
    // update
    bool ins2 = ht.insert("one", 11);
    assert(ins2 == false);
    v = ht.find("one");
    assert(v != nullptr && *v == 11);
}

int main() {
    cout << "Running container tests...\n";
    test_insert_find_update();
    cout << " test_insert_find_update passed\n";
    test_erase();
    cout << " test_erase passed\n";
    test_size_and_rehash();
    cout << " test_size_and_rehash passed\n";
    test_template_with_int();
    cout << " test_template_with_int passed\n";
    cout << "All tests passed.\n";
    return 0;
}
