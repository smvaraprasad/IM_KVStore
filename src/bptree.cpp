#include "bptree.hpp"

// Node Constructor
template<typename K, typename V>
Bptree<K, V>::Node::Node(bool leaf) : is_leaf(leaf), next(nullptr), prev(nullptr) {
    keys.reserve(M);
    if (leaf) {
        values.reserve(M);
    } else {
        children.reserve(M + 1);
    }
}

// Bptree Constructor & Destructor
template<typename K, typename V>
Bptree<K, V>::Bptree() : root(new Node(true)) {}

template<typename K, typename V>
Bptree<K, V>::~Bptree() {
    clear(root);
}

template<typename K, typename V>
size_t Bptree<K, V>::find_key_index(const Node* node, const K& key) const {
    auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
    return std::distance(node->keys.begin(), it);
}

template<typename K, typename V>
void Bptree<K, V>::clear(Node* node) {
    if (node == nullptr) return;
    if (!node->is_leaf) {
        for (Node* child : node->children) {
            clear(child);
        }
    }
    delete node;
}

template<typename K, typename V>
void Bptree<K, V>::insert(const K& key, const V& value) {
    if (root->keys.size() == MAX_KEYS) {
        Node* new_root = new Node(false);
        new_root->children.push_back(root);
        split_child(new_root, 0, root);
        root = new_root;
    }
    insert_non_full(root, key, value);
}

template<typename K, typename V>
void Bptree<K, V>::insert_non_full(Node* node, const K& key, const V& value) {
    size_t idx = find_key_index(node, key);

    if (node->is_leaf) {
        if (idx < node->keys.size() && node->keys[idx] == key) {
            node->values[idx] = value;
            return;
        }
        node->keys.insert(node->keys.begin() + idx, key);
        node->values.insert(node->values.begin() + idx, value);
    } else {
        if (node->children[idx]->keys.size() == MAX_KEYS) {
            split_child(node, idx, node->children[idx]);
            if (key >= node->keys[idx]) {
                idx++;
            }
        }
        insert_non_full(node->children[idx], key, value);
    }
}

template<typename K, typename V>
void Bptree<K, V>::split_child(Node* parent, size_t idx, Node* child) {
    bool is_leaf = child->is_leaf;
    Node* newNode = new Node(is_leaf);

    if (is_leaf) {
        size_t mid = child->keys.size() / 2;
        newNode->keys.assign(child->keys.begin() + mid, child->keys.end());
        newNode->values.assign(child->values.begin() + mid, child->values.end());

        child->keys.erase(child->keys.begin() + mid, child->keys.end());
        child->values.erase(child->values.begin() + mid, child->values.end());

        newNode->next = child->next;
        newNode->prev = child;
        child->next = newNode;
        if (newNode->next) newNode->next->prev = newNode;

        K promoted_key = newNode->keys[0];
        parent->keys.insert(parent->keys.begin() + idx, promoted_key);
        parent->children.insert(parent->children.begin() + idx + 1, newNode);
    } else {
        size_t mid = child->keys.size() / 2;
        K promoted_key = child->keys[mid];

        newNode->keys.assign(child->keys.begin() + mid + 1, child->keys.end());
        newNode->children.assign(child->children.begin() + mid + 1, child->children.end());

        child->keys.erase(child->keys.begin() + mid, child->keys.end());
        child->children.erase(child->children.begin() + mid + 1, child->children.end());

        parent->keys.insert(parent->keys.begin() + idx, promoted_key);
        parent->children.insert(parent->children.begin() + idx + 1, newNode);
    }
}

template<typename K, typename V>
bool Bptree<K, V>::search(const K& key, V& out_value) const {
    if (root == nullptr) return false;
    Node* curr = root;

    while (!curr->is_leaf) {
        size_t idx = find_key_index(curr, key);
        if (idx < curr->keys.size() && curr->keys[idx] == key) {
            idx++;
        }
        curr = curr->children[idx];
    }

    size_t idx = find_key_index(curr, key);
    if (idx < curr->keys.size() && curr->keys[idx] == key) {
        out_value = curr->values[idx];
        return true;
    }

    return false;
}

template<typename K, typename V>
void Bptree<K, V>::borrow_from_left(Node* parent, size_t idx) {
    Node* child = parent->children[idx];
    Node* sibling = parent->children[idx - 1];

    if (child->is_leaf) {
        child->keys.insert(child->keys.begin(), sibling->keys.back());
        child->values.insert(child->values.begin(), sibling->values.back());
        sibling->keys.pop_back();
        sibling->values.pop_back();
        parent->keys[idx - 1] = child->keys[0];
    } else {
        child->keys.insert(child->keys.begin(), parent->keys[idx - 1]);
        child->children.insert(child->children.begin(), sibling->children.back());
        parent->keys[idx - 1] = sibling->keys.back();
        sibling->keys.pop_back();
        sibling->children.pop_back();
    }
}

template<typename K, typename V>
void Bptree<K, V>::borrow_from_right(Node* parent, size_t idx) {
    Node* child = parent->children[idx];
    Node* sibling = parent->children[idx + 1];

    if (child->is_leaf) {
        child->keys.push_back(sibling->keys.front());
        child->values.push_back(sibling->values.front());
        sibling->keys.erase(sibling->keys.begin());
        sibling->values.erase(sibling->values.begin());
        parent->keys[idx] = sibling->keys[0];
    } else {
        child->keys.push_back(parent->keys[idx]);
        child->children.push_back(sibling->children.front());
        parent->keys[idx] = sibling->keys.front();
        sibling->keys.erase(sibling->keys.begin());
        sibling->children.erase(sibling->children.begin());
    }
}

template<typename K, typename V>
void Bptree<K, V>::merge(Node* parent, size_t idx) {
    Node* child = parent->children[idx];
    Node* sibling = parent->children[idx + 1];

    if (child->is_leaf) {
        child->keys.insert(child->keys.end(), sibling->keys.begin(), sibling->keys.end());
        child->values.insert(child->values.end(), sibling->values.begin(), sibling->values.end());
        child->next = sibling->next;
        if (sibling->next != nullptr) {
            sibling->next->prev = child;
        }
    } else {
        child->keys.push_back(parent->keys[idx]);
        child->keys.insert(child->keys.end(), sibling->keys.begin(), sibling->keys.end());
        child->children.insert(child->children.end(), sibling->children.begin(), sibling->children.end());
    }

    parent->keys.erase(parent->keys.begin() + idx);
    parent->children.erase(parent->children.begin() + idx + 1);
    delete sibling;
}

template<typename K, typename V>
void Bptree<K, V>::remove_internal(Node* node, const K& key) {
    size_t idx = find_key_index(node, key);

    if (node->is_leaf) {
        if (idx < node->keys.size() && node->keys[idx] == key) {
            node->keys.erase(node->keys.begin() + idx);
            node->values.erase(node->values.begin() + idx);
        }
        return;
    }

    if (idx < node->keys.size() && node->keys[idx] == key) {
        idx++;
    }

    Node* child = node->children[idx];

    if (child->keys.size() < MIN_KEYS + 1) {
        if (idx > 0 && node->children[idx - 1]->keys.size() > MIN_KEYS) {
            borrow_from_left(node, idx);
        } else if (idx < node->children.size() - 1 && node->children[idx + 1]->keys.size() > MIN_KEYS) {
            borrow_from_right(node, idx);
        } else {
            if (idx < node->children.size() - 1) {
                merge(node, idx);
            } else {
                merge(node, idx - 1);
                idx--;
            }
        }
    }

    remove_internal(node->children[idx], key);
}

template<typename K, typename V>
bool Bptree<K, V>::remove(const K& key) {
    if (root == nullptr) return false;
    
    V dummy;
    if (!search(key, dummy)) return false;

    remove_internal(root, key);
    if (!root->is_leaf && root->keys.empty()) {
        Node* old_root = root;
        root = root->children[0];
        delete old_root;
    }
    return true;
}

template<typename K, typename V>
void Bptree<K, V>::scan_all() const {
    if (root == nullptr) return;
    Node* curr = root;
    while (!curr->is_leaf) {
        curr = curr->children[0];
    }

    while (curr != nullptr) {
        for (size_t i = 0; i < curr->keys.size(); ++i) {
            std::cout << curr->keys[i] << " -> " << curr->values[i] << "\n";
        }
        curr = curr->next;
    }
}

// ============================================================================
// EXPLICIT TEMPLATE INSTANTIATION
// ============================================================================
// Tells the compiler to generate code for std::string key-value pairs
template class Bptree<std::string, std::string>;