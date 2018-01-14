#pragma once

namespace K273 {

    class InplaceList;
    class InplaceIter;

    class InplaceListEntry {

    public:
        InplaceListEntry();
        virtual ~InplaceListEntry();

    private:
        InplaceListEntry* next_entry;
        InplaceListEntry* prev_entry;
        InplaceList* inplace_list;
        friend InplaceList;
    };

    class InplaceList : public InplaceListEntry {

    public:
        InplaceList();
        virtual ~InplaceList();

    public:
        static InplaceListEntry* next(const InplaceListEntry* e) {
            return e->next_entry;
        }

        static InplaceListEntry* prev(const InplaceListEntry* e) {
            return e->prev_entry;
        }

        InplaceListEntry* head() const {
            return this->head_entry;
        }

        InplaceListEntry* tail() const {
            return this->tail_entry;
        }

        void insertBefore(InplaceListEntry* existing_entry, InplaceListEntry* new_entry);
        void pushBack(InplaceListEntry* new_entry);
        void pushFront(InplaceListEntry* new_entry);

        InplaceListEntry* remove(InplaceListEntry* entry);

        int size() const;
        bool empty() const;

        // optimized - will delete the memory on users behalf
        void clear();
        void clearForwards(InplaceListEntry* entry);
        void clearBackwards(InplaceListEntry* entry);

    private:
        InplaceListEntry* head_entry;
        InplaceListEntry* tail_entry;
        int entries_count;
    };

    template <typename T>
    class InplaceRange {
    public:
        class InplaceIter2 {
        public:
            InplaceIter2(InplaceListEntry* e) :
                entry(e) {
            }

            bool operator!=(const InplaceIter2& other) const {
                return this->entry != other.entry;
            }

            // this method must be defined after the definition of IntVector since it needs to use it
            T* operator*() const {
                T* e = static_cast<T*> (this->entry);
                return e;
            }

            const InplaceIter2& operator++() {
                this->entry = InplaceList::next(this->entry);
                return *this;
            }

        private:
            InplaceListEntry* entry;
        };

    public:
        InplaceRange(InplaceList* l) :
            ilist(l) {
        }

        InplaceIter2 begin() const {
            return InplaceIter2(this->ilist->head());
        }

        InplaceIter2 end() const {
            return InplaceIter2(nullptr);
        }

    private:
        InplaceList* ilist;
    };

}
