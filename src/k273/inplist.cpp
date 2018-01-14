// local includes
#include "k273/inplist.h"
#include "k273/exception.h"

///////////////////////////////////////////////////////////////////////////////

using namespace K273;

///////////////////////////////////////////////////////////////////////////////

InplaceListEntry::InplaceListEntry() :
    next_entry(nullptr),
    prev_entry(nullptr),
    inplace_list(nullptr) {
}

InplaceListEntry::~InplaceListEntry() {
}

InplaceList::InplaceList() :
    head_entry(nullptr),
    tail_entry(nullptr),
    entries_count(0) {
}

InplaceList::~InplaceList() {
}

void InplaceList::insertBefore(InplaceListEntry* existing_entry, InplaceListEntry* new_entry) {
    // insert entry before existing_entry

    ASSERT_MSG (new_entry->inplace_list == nullptr, "entry is already in a list");
    ASSERT_MSG (existing_entry->inplace_list == this, "existing entry is not in this list");

    // get reference to previous, and replace previous on existing
    InplaceListEntry* previous = existing_entry->prev_entry;
    existing_entry->prev_entry = new_entry;

    new_entry->prev_entry = previous;
    new_entry->next_entry = existing_entry;
    new_entry->inplace_list = this;

    // are we the head?
    if (previous == nullptr) {
        ASSERT (existing_entry == this->head_entry);
        this->head_entry = new_entry;
    } else {
        previous->next_entry = new_entry;
    }

    this->entries_count++;
}

void InplaceList::pushBack(InplaceListEntry* new_entry) {
    ASSERT_MSG (new_entry->inplace_list == nullptr, "entry is already in a list");

    new_entry->prev_entry = this->tail_entry;
    new_entry->next_entry = nullptr;
    new_entry->inplace_list = this;

    if (this->tail_entry == nullptr) {
        ASSERT (this->head_entry == nullptr);
        this->head_entry = this->tail_entry = new_entry;
    } else {
        this->tail_entry->next_entry = new_entry;
        this->tail_entry = new_entry;
    }

    this->entries_count++;
}

void InplaceList::pushFront(InplaceListEntry* new_entry) {
    if (this->head_entry == nullptr) {
        this->pushBack(new_entry);
    } else {
        this->insertBefore(this->head_entry, new_entry);
    }
}

InplaceListEntry* InplaceList::remove(InplaceListEntry* entry) {
    // returns the next entry if not empty

    ASSERT_MSG (entry->inplace_list != nullptr, "entry is not in a list");

    InplaceListEntry* previous = entry->prev_entry;
    InplaceListEntry* next = entry->next_entry;

    // fix previous if it exists
    if (previous == nullptr) {
        ASSERT (entry == this->head_entry);
        this->head_entry = next;
    } else {
        previous->next_entry = next;
    }

    // fix next if it exists
    if (next == nullptr) {
        ASSERT (entry == this->tail_entry);
        this->tail_entry = previous;
    } else {
        next->prev_entry = previous;
    }

    entry->prev_entry = nullptr;
    entry->next_entry = nullptr;
    entry->inplace_list = nullptr;

    this->entries_count--;

    return next;
}

int InplaceList::size() const {
    return this->entries_count;
}

bool InplaceList::empty() const {
    return (this->size() == 0);
}

void InplaceList::clear() {
    // spin through all and call delete and then set head/tail

    InplaceListEntry* entry = this->head_entry;
    while (entry != nullptr) {
        InplaceListEntry* carcass = entry;
        entry = entry->next_entry;
        delete carcass;
    }

    // reset all
    this->head_entry = nullptr;
    this->tail_entry = nullptr;
    this->entries_count = 0;
}

void InplaceList::clearBackwards(InplaceListEntry* entry) {
    ASSERT_MSG (entry->inplace_list != nullptr, "entry is not in a list");

    // spin through all and call delete and then set head/tail

    InplaceListEntry* next = entry->next_entry;

    // are we the tail?
    if (next == nullptr) {
        this->head_entry = nullptr;
        this->tail_entry = nullptr;
    } else {
        this->head_entry = entry->next_entry;
        next->prev_entry = nullptr;
    }

    while (entry != nullptr) {
        InplaceListEntry* carcass = entry;
        entry = entry->prev_entry;
        delete carcass;
        this->entries_count--;
    }
}
