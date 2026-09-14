#ifndef BPTREE_HPP
#define BPTREE_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include <utility>
#include <iostream>

using std::vector;

template<typename K, typename V>
class Bptree {
private:
    static constexpr size_t M = 16;
    static constexpr size_t MAX_KEYS = M - 1;
    static constexpr size_t MIN_KEYS = (M + 1) / 2 - 1;

    struct Node {
        bool is_leaf;
        vector<K> keys;
        vector<V> values;
        vector<Node*> children;
        Node* next;
        Node* prev;

        explicit Node(bool leaf);
    };

    Node* root;

    // Internal helper methods
    void insert_non_full(Node* node, const K& key, const V& value);
    void split_child(Node* parent, size_t idx, Node* child);
    size_t find_key_index(const Node* node, const K& key) const;
    void clear(Node* node);
    void borrow_from_left(Node* parent, size_t idx);
    void borrow_from_right(Node* parent, size_t idx);
    void merge(Node* parent, size_t idx);
    void remove_internal(Node* node, const K& key);

public:
    Bptree();
    ~Bptree();

    // Prevent copies
    Bptree(const Bptree&) = delete;
    Bptree& operator=(const Bptree&) = delete;

    // Public API
    bool search(const K& key, V& out_value) const;
    void insert(const K& key, const V& value);
    bool remove(const K& key);
    void scan_all() const;
};

#endif // BPTREE_HPP