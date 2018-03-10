#pragma once

#include "orbit/msgs.h"

namespace K273::Orbit {

    class HandlerBase {
    public:
        HandlerBase() {
        }

        virtual ~HandlerBase() {
        }

    public:
        virtual void call(const Odo::MessageHeader* msg) = 0;
    };
}

#define DEFINE_MESSAGE_HANDLER(T, msg_name)                             \
                                                                        \
    class Type##msg_name : public HandlerBase {                         \
    public:                                                             \
    Type##msg_name() :                                                  \
        parent(nullptr) {                                               \
    }                                                                   \
                                                                        \
    ~Type##msg_name() {                                                 \
    }                                                                   \
                                                                        \
    void init(T* p) {                                                   \
        this->parent = p;                                               \
    }                                                                   \
                                                                        \
    void call(const Odo::MessageHeader* msg) {                          \
        this->parent->handle##msg_name(msg);                            \
    }                                                                   \
                                                                        \
    private:                                                            \
    T* parent;                                                          \
    };                                                                  \
                                                                        \
    Type##msg_name _impl_##msg_name;                                    \


#define INIT_MESSAGE_HANDLER(msg_name)                            \
    this->_impl_##msg_name.init(this);                            \
    this->orb->add(#msg_name, &this->_impl_##msg_name);           \
                                                                  \

