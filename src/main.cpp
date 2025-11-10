/**
 * Amazon Inventory Management System
 * 
 * A command-line REPL (Read-Eval-Print Loop) application for querying
 * product inventory loaded from a CSV file. Uses a custom hash table
 * for O(1) product lookup by ID and a category index for filtering.
 * 
 * Supported Commands:
 *  - find <Uniq Id>           : Search for a product by its unique ID
 *  - listInventory <Category> : List all products in a specific category
 *  - :help                    : Display command help
 *  - :quit                    : Exit the application
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

#include "../Headers/HashTable.hpp"
#include "../Headers/Parser.hpp"

using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::size_t;
using std::string;
using std::unordered_map;
using std::vector;

namespace {

// ============================================================================
// GLOBAL DATA STRUCTURES
// ============================================================================

/**
 * Primary storage: Hash table mapping Uniq Id -> Product
 * Provides O(1) average-case lookup for finding products by ID
 */
inv::HashTable<inv::Product> g_table;

/**
 * Secondary index: Category -> list of Uniq Ids
 * Enables efficient querying of all products in a given category
 * Products can belong to multiple categories (stored in Product.categories)
 */
unordered_map<string, vector<string>> g_categoryIndex;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Trim leading and trailing whitespace from a string
 * @param s Input string
 * @return Trimmed string
 */
static string trim(const string &s) {
    size_t start = 0; 
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size(); 
    while (end > start && std::isspace(static_cast<unsigned char>(s[end-1]))) --end;
    return s.substr(start, end-start);
}

/**
 * Print a product's details in a formatted, human-readable manner
 * Wraps long product descriptions to improve readability
 * 
 * @param p The product to print
 */
static void printProduct(const inv::Product &p) {
    cout << "Uniq Id: " << p.uniqId << endl;
    cout << "Product Name: " << p.productName << endl;
    cout << "Brand Name: " << p.brandName << endl;
    cout << "Category: " << p.category << endl;
    cout << "List Price: " << p.listPrice << endl;
    cout << "Selling Price: " << p.sellingPrice << endl;
    cout << "Quantity: " << p.quantity << endl;
    if (!p.asin.empty()) cout << "Asin: " << p.asin << endl;
    if (!p.modelNumber.empty()) cout << "Model Number: " << p.modelNumber << endl;
    
    /**
     * Lambda helper to wrap and print long text fields with proper indentation
     * Breaks text into lines that fit within maxWidth characters
     */
    auto wrapAndPrint = [&](const std::string &label, const std::string &text, size_t maxWidth = 100) {
        cout << label;
        if (text.empty()) { cout << endl; return; }
        
        // Split text into words
        std::istringstream iss(text);
        std::vector<std::string> words;
        std::string w;
        while (iss >> w) words.push_back(w);

        const std::string indent = "    "; // 4 spaces for wrapped lines
        size_t lineWidth = maxWidth;

        // Start a new line for the wrapped block
        cout << '\n';
        std::string cur = indent;
        for (size_t i = 0; i < words.size(); ++i) {
            const std::string &word = words[i];
            if (cur.size() + (cur.size() > indent.size() ? 1 : 0) + word.size() > lineWidth) {
                cout << cur << std::endl;
                cur = indent + word;
            } else {
                if (cur.size() > indent.size()) cur += ' ';
                cur += word;
            }
        }
        if (!cur.empty()) cout << cur << std::endl;
    };

    wrapAndPrint("Product Description:", p.productDescription, 100);
    if (!p.stock.empty()) cout << "Stock: " << p.stock << endl;
}

} // namespace

// ============================================================================
// REPL COMMAND HANDLERS
// ============================================================================

/**
 * Display help information about available commands
 */
/**
 * Display help information about available commands
 */
void printHelp()
{
    cout << "Supported list of commands: " << endl;
    cout << " 1. find <inventoryid> - Finds if the inventory exists. If exists, prints details. If not, prints 'Inventory not found'." << endl;
    cout << " 2. listInventory <category_string> - Lists just the id and name of all inventory belonging to the specified category. If the category doesn't exists, prints 'Invalid Category'.\n"
         << endl;
    cout << " Use :quit to quit the REPL" << endl;
}

/**
 * Validate whether a command line is recognized
 * @param line User input command
 * @return true if command is valid, false otherwise
 */
bool validCommand(string line)
{
    return (line == ":help") ||
           (line.rfind("find", 0) == 0) ||
           (line.rfind("listInventory", 0) == 0);
}

/**
 * Evaluate and execute a user command
 * Parses the command and its arguments, then performs the requested action
 * 
 * @param line User input command string
 */
void evalCommand(string line)
{
    if (line == ":help")
    {
        printHelp();
    }
    else if (line.rfind("find", 0) == 0)
    {
        // Command: find <id>
        // Searches for a product by unique ID and displays full details
        auto pos = line.find(' ');
        if (pos == string::npos || pos + 1 >= line.size()) {
            cout << "Inventory not found" << endl;
            return;
        }
        string id = trim(line.substr(pos + 1));
        if (id.empty()) { 
            cout << "Inventory not found" << endl; 
            return; 
        }
        
        // Lookup product in hash table (O(1) average case)
        auto *p = g_table.find(id);
        if (!p) {
            cout << "Inventory not found" << endl;
        } else {
            printProduct(*p);
        }
    }
    else if (line.rfind("listInventory", 0) == 0)
    {
        // Command: listInventory <category>
        // Lists all products belonging to a specific category
        auto pos = line.find(' ');
        if (pos == string::npos || pos + 1 >= line.size()) {
            cout << "Invalid Category" << endl;
            return;
        }
        string category = trim(line.substr(pos + 1));
        
        // Check if category exists in the index
        auto it = g_categoryIndex.find(category);
        if (it == g_categoryIndex.end()) {
            cout << "Invalid Category" << endl;
            return;
        }
        
        // Iterate through all product IDs in this category
        for (const auto &id : it->second) {
            const inv::Product *p = g_table.find(id);
            if (p) {
                cout << id << " - " << p->productName << endl;
            }
        }
    }
}

/**
 * Initialize the application
 * Loads the CSV data file into the hash table and category index,
 * then displays the welcome message
 */
void bootStrap()
{
    cout << "\n Welcome to Amazon Inventory Query System" << endl;
    cout << " enter :quit to exit. or :help to list supported commands." << endl;
    
    // Load CSV data into hash table and build category index
    // The parser sanitizes data and handles multi-line fields
    const string csv = "marketing_sample_for_amazon_com-ecommerce__20200101_20200131__10k_data.csv";
    if (!inv::loadCsv(csv, g_table, g_categoryIndex)) {
        cout << "Failed to load dataset: " << csv << endl;
    }
    cout << "\n> ";
}

/**
 * Main REPL loop
 * Reads user commands, validates, and executes them until user quits
 */
int main(int argc, char const *argv[])
{
    string line;
    bootStrap();  // Initialize and load data
    
    // Main loop: read commands until user enters ":quit"
    while (getline(cin, line) && line != ":quit")
    {
        if (validCommand(line))
        {
            evalCommand(line);
        }
        else
        {
            cout << "Command not supported. Enter :help for list of supported commands" << endl;
        }
        cout << "> ";  // Display prompt for next command
    }
    return 0;
}
