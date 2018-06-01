#pragma once

#include "k273/exception.h"

namespace K273 {
    // XXX now this is templatised, how different is it from stl list?
    // have inlined data in node same.  Having prev/next same.  Having reference to inplace_list -
    // different.
    // ability to access parent(), next(), prev().
    // 
    // XXX add allocator...
    template <typename T, bool owns_memory=true>
    class InplaceList {
    public:
        using DataType = T;

        // the node is internal to us
        class Node {
        public:
            template<class ... Types> Node(Types ... args) :
                data(args...),
                next_node(nullptr),
                prev_node(nullptr),
                inplace_list(nullptr) {
            }

        public:
            Node* next() {
                return this->next_node;
            }

            Node* prev() {
                return this->prev_node;
            }

            InplaceList* parent() {
                return this->inplace_list;
            }

            DataType& get() {
                return this->data;
            }

            const DataType& get() const {
                return this->data;
            }

        private:
            DataType data;
            Node* next_node;
            Node* prev_node;
            InplaceList* inplace_list;
            friend InplaceList;
        };

    public:
        explicit InplaceList() :
            head_node(nullptr),
            tail_node(nullptr),
            node_count(0) {
        }

        ~InplaceList() {
        }

        InplaceList(InplaceList const &) = delete;
        InplaceList& operator= (const InplaceList&) = delete;

    public:
        /* could also add as convenience wrappers:

           popFront()
           popBack()
           emplaceBack() and otehr emplaceXXX*( friends
        */

        Node* head() const {
            return this->head_node;
        }

        Node* tail() const {
            return this->tail_node;
        }

        template<class ... Types> Node* createNode(Types ... args) {
            return new Node(args ...);
        }

        void pushBack(Node* new_node) {
            ASSERT_MSG (new_node->inplace_list == nullptr, "node is already in a list");

            new_node->prev_node = this->tail_node;
            new_node->next_node = nullptr;
            new_node->inplace_list = this;

            if (this->tail_node == nullptr) {
                ASSERT (this->head_node == nullptr);
                this->head_node = this->tail_node = new_node;
            } else {
                this->tail_node->next_node = new_node;
                this->tail_node = new_node;
            }

            this->node_count++;
        }

        template<class ... Types> void emplaceBack(Types ... args) {
            this->pushBack(new Node(args ...));
        }

        void pushFront(Node* new_node) {
            if (this->head_node == nullptr) {
                this->pushBack(new_node);
            } else {
                this->insertBefore(this->head_node, new_node);
            }
        }

        void insertBefore(Node* existing_node, Node* new_node) {
            // insert node before existing_node

            ASSERT_MSG (new_node->inplace_list == nullptr, "node is already in a list");
            ASSERT_MSG (existing_node->inplace_list == this, "existing node is not in this list");

            // get reference to previous, and replace previous on existing
            Node* previous = existing_node->prev_node;
            existing_node->prev_node = new_node;

            new_node->prev_node = previous;
            new_node->next_node = existing_node;
            new_node->inplace_list = this;

            // are we the head?
            if (previous == nullptr) {
                ASSERT (existing_node == this->head_node);
                this->head_node = new_node;
            } else {
                previous->next_node = new_node;
            }

            this->node_count++;
        }

        void insertAfter(Node* existing_node, Node* new_node) {
            // insert node before existing_node

            ASSERT_MSG (new_node->inplace_list == nullptr, "node is already in a list");
            ASSERT_MSG (existing_node->inplace_list == this, "existing node is not in this list");

            // get reference to previous, and replace previous on existing
            Node* next = existing_node->next_node;
            existing_node->next_node = new_node;

            new_node->prev_node = existing_node;
            new_node->next_node = next;
            new_node->inplace_list = this;

            // are we the tail?
            if (next == nullptr) {
                ASSERT (existing_node == this->tail_node);
                this->tail_node = new_node;
            } else {
                next->prev_node = new_node;
            }

            this->node_count++;
        }

        Node* remove(Node* node) {
            // returns the next node if not empty

            ASSERT_MSG (node->inplace_list != nullptr, "node is not in a list");

            Node* previous = node->prev_node;
            Node* next = node->next_node;

            // fix previous if it exists
            if (previous == nullptr) {
                ASSERT (node == this->head_node);
                this->head_node = next;
            } else {
                previous->next_node = next;
            }

            // fix next if it exists
            if (next == nullptr) {
                ASSERT (node == this->tail_node);
                this->tail_node = previous;
            } else {
                next->prev_node = previous;
            }

            node->prev_node = nullptr;
            node->next_node = nullptr;
            node->inplace_list = nullptr;

            this->node_count--;

            return next;
        }

        int size() const {
            return this->node_count;
        }

        bool empty() const {
            return this->size() == 0;
        }

        void clear() {
            // spin through all and call delete and then set head/tail

            Node* node = this->head_node;
            while (node != nullptr) {
                Node* carcass = node;
                node = node->next_node;
                if (owns_memory) {
                    delete carcass;
                }
            }

            // reset all
            this->head_node = nullptr;
            this->tail_node = nullptr;
            this->node_count = 0;
        }

        void clearToFront(Node* node) {
            ASSERT_MSG (node->inplace_list != nullptr, "node is not in a list");

            // spin through all and call delete and then set head/tail

            Node* next = node->next_node;

            // are we the tail?
            if (next == nullptr) {
                this->clear();
                return;

            }

            this->head_node = node->next_node;
            next->prev_node = nullptr;

            while (node != nullptr) {
                Node* carcass = node;
                node = node->prev_node;
                if (owns_memory) {
                    delete carcass;
                }

                this->node_count--;
            }
        }

    private:
        Node* head_node;
        Node* tail_node;
        int node_count;

    private:
        class nodeIterator {
        public:
            nodeIterator(Node* n) :
                node(n) {
            }

            bool operator!= (const nodeIterator& other) const {
                return this->node != other.node;
            }

            Node* operator*() {
                return this->node;
            }

            const nodeIterator& operator++() {
                this->node = this->node->next();
                return *this;
            }

        private:
            Node* node;
        };

        class dataIterator {
        public:
            dataIterator(Node* n) :
                node(n) {
            }

            bool operator!= (const dataIterator& other) const {
                return this->node != other.node;
            }

            DataType& operator*() {
                return this->node->get();
            }

            const DataType& operator*() const {
                return this->node->get();
            }

            const dataIterator& operator++() {
                this->node = this->node->next();
                return *this;
            }

        private:
            Node* node;
        };

        struct asDataIterator {
            asDataIterator(InplaceList* ourself) :
                ourself(ourself) {
            }
            dataIterator begin() {
                return dataIterator(this->ourself->head());
            }

            dataIterator end() {
                return dataIterator(nullptr);
            }

        private:
            InplaceList* ourself;
        };

    public:
        nodeIterator begin() {
            return nodeIterator(this->head());
        }

        nodeIterator end() {
            return nodeIterator(nullptr);
        }

        asDataIterator asData() {
            return asDataIterator(this);
        }
    };
}
