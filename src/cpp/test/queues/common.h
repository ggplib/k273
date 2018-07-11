#pragma once

// kevlin includes
#include <kelvin/msgq/nto1.h>
#include <kelvin/msgq/1ton.h>

////////////////////////////////////////////////////////////////////////////////

const size_t BUF_SIZE = 42 - sizeof(uint64_t) - sizeof(uint32_t) - sizeof(uint32_t);

struct Event {
    uint64_t ticks;
    uint32_t client_id;
    uint32_t seq;

    char buf[BUF_SIZE];
};

// Queues sizes
const size_t EchoQueueSize = 4096;
const size_t RequestQueueSize = 4096;

// Echo queue types
typedef Kelvin::MsgQ::OneToMany::Producer EchoProducer;
typedef Kelvin::MsgQ::OneToMany::Consumer EchoConsumer;

// Request queue types
typedef Kelvin::MsgQ::ManyToOne::Producer RequestProducer;
typedef Kelvin::MsgQ::ManyToOne::Consumer RequestConsumer;

////////////////////////////////////////////////////////////////////////////////

extern volatile int run_flag;
void setupSignals();

////////////////////////////////////////////////////////////////////////////////

static const char echo_name[] =  "/echo_queue";
static const char request_name[] =  "/request_queue";

