// local includes
#include "orbit/connector.h"
#include "orbit/msgs.h"
#include "orbit/odo.h"

// k273 includes
#include <k273/logging.h>

///////////////////////////////////////////////////////////////////////////////

using namespace Kelvin;
using namespace K273::Orbit;

///////////////////////////////////////////////////////////////////////////////

Connector::Connector(Streamer::ConnectorBase* connector, int orbit_client_id) :
    ConnectingProtocol(connector),
    is_connected(false),
    orbit_client_id(orbit_client_id),
    initialiser(this->scheduler, this) {
}

Connector::~Connector() {
}

void Connector::onBuffer(ByteBuffer& buf) {

    // have we read enough for a message header?
    while ((unsigned) buf.remaining() > sizeof(Connection::MessageHeader)) {

        // get header
        Connection::MessageHeader* header = reinterpret_cast <Connection::MessageHeader*> (buf.getInternalBuf());

        // entire message recieved?
        if ((unsigned) buf.remaining() >= header->message_length) {
            this->onMessage(header);
            buf.skip(header->message_length);
        } else {
            break;
        }
    }
}

void Connector::onMessage(Connection::MessageHeader* header) {
    switch (header->message_type) {
    case Connection::MsgType_Hello:
        K273::l_warning("Should never receive HelloMessage from server... dropping");
        break;

    case Connection::MsgType_InitialiseClient:
        this->handleInitialiseClient(reinterpret_cast <Connection::InitialiseClientMessage*> (header));
        break;

    case Connection::MsgType_Ping:
        this->handlePing(reinterpret_cast <Connection::PingMessage*> (header));
        break;

    case Connection::MsgType_Pong:
        K273::l_warning("Should never receive PongMessage from server... dropping");
        break;

    default:
        K273::l_warning("Unknown message from server... type: %d", header->message_type);
        break;
    }
}

void Connector::handleInitialiseClient(Connection::InitialiseClientMessage* msg) {
    // set session_name
    this->orbit_name = msg->orbit_name;
    K273::l_info("Connecting to orb with session: %s", this->orbit_name.c_str());

    // what is our polltype?  If we are currently in a poll type, what do we do?  Can we change poll type? XXX
    this->poll_type = msg->poll_type;

    //vector <ChannelInfo> channel_info;
    for (unsigned ii=0; ii<msg->number_of_channels; ii++) {
        Connection::ChannelInfo* channel = &msg->channel_info[ii];

        std::string s;
        switch (channel->inbound_outbound) {
        case 0:
            s = "dedicated inbound";
            break;
        case 1:
            s = "dedicated outbound";
            break;
        case 2:
            s = "master inbound";
            break;
        case 3:
            s = "master outbound";
            break;
        default:
            s = "WTF XXX";
            break;
        }

        K273::l_info("Channel %d, out bound bundle group %d, type %s, NAME: %s0",
                     channel->channel_id, channel->outbound_channel_group_id,
                     s.c_str(), channel->path);
    }

    // actually set up the queues etc later
    this->initialiser.callLater(0);
}

void Connector::handlePing(Connection::PingMessage* ping_message) {
    K273::l_debug("Got ping from server");

    Connection::PongMessage msg;
    msg.header.message_type = Connection::MsgType_Pong;
    msg.header.message_length = sizeof(Connection::PongMessage);
    this->write((const char *) &msg, msg.header.message_length);
}

void Connector::connectionMade() {
    K273::l_info("connectionMade in OrbConnector");

    // send hello message to server
    Connection::HelloMessage msg;
    msg.header.message_type = Connection::MsgType_Hello;
    msg.header.message_length = sizeof(Connection::HelloMessage);

    // XXX handle asking for client id from server
    msg.source_id = this->orbit_client_id;

    this->write((const char *) &msg, msg.header.message_length);
}

void Connector::connectionLost() {
    // connection to server lost
    K273::l_error("connectionLost in OrbConnector");
}

std::string Connector::repr() const {
    return "'I am orbit client'";
}

void Connector::onInitialise() {
    K273::l_error("do initialisation");
}

void Connector::run() {
    this->scheduler->run(false);

    const int poll_time_msecs = 100;
    int next_poll_time_msecs = poll_time_msecs;
    while (true) {
        if (this->is_connected) {
            break;
        }

        // poll slowly, until we know exactly which poller to use.
        int minimum_poll_time = this->scheduler->poll(next_poll_time_msecs);
        if (minimum_poll_time == -1) {
            break;
        }

        if (minimum_poll_time < poll_time_msecs) {
            next_poll_time_msecs = minimum_poll_time;

        } else {
            next_poll_time_msecs = poll_time_msecs;
        }
    }

    // now run some other poller...
}
