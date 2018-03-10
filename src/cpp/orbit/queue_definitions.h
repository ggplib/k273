#pragma once

// k273 includes
#include <k273/util.h>

// msgq includes
#include <msgq/nto1.h>
#include <msgq/1ton.h>

///////////////////////////////////////////////////////////////////////////////

namespace K273::Orbit {

    typedef K273::MsgQ::OneToMany::Producer DedicatedProducer;
    typedef K273::MsgQ::OneToMany::Consumer DedicatedConsumer;

    typedef K273::MsgQ::OneToMany::Producer MasterBroadcastProducer;
    typedef K273::MsgQ::OneToMany::Consumer MasterBroadcastConsumer;

    typedef K273::MsgQ::ManyToOne::Producer MasterInboundProducer;
    typedef K273::MsgQ::ManyToOne::Producer MasterInboundConsumer;

}
