#pragma once

// k273 includes
#include <k273/util.h>

// msgq includes
#include <kelvin/msgq/nto1.h>
#include <kelvin/msgq/1ton.h>

///////////////////////////////////////////////////////////////////////////////

namespace K273::Orbit {

    typedef Kelvin::MsgQ::OneToMany::Producer DedicatedProducer;
    typedef Kelvin::MsgQ::OneToMany::Consumer DedicatedConsumer;

    typedef Kelvin::MsgQ::OneToMany::Producer MasterBroadcastProducer;
    typedef Kelvin::MsgQ::OneToMany::Consumer MasterBroadcastConsumer;

    typedef Kelvin::MsgQ::ManyToOne::Producer MasterInboundProducer;
    typedef Kelvin::MsgQ::ManyToOne::Producer MasterInboundConsumer;

}
