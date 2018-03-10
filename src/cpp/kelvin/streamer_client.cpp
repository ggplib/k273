
// local includes
#include "kelvin/streamer_client.h"
#include "kelvin/streamer.h"

#include "kelvin/socket.h"
#include "kelvin/selector.h"
#include "kelvin/scheduler.h"
#include "kelvin/bytebuffer.h"

// k273 includes
#include <k273/logging.h>
#include <k273/strutils.h>

// std includes
#include <errno.h>
#include <string>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;
using namespace K273::Streamer;

///////////////////////////////////////////////////////////////////////////////

static const bool _debug_log = false;
#define TRACE(fmt, args...) if (_debug_log) { K273::l_debug(fmt, ## args); }

///////////////////////////////////////////////////////////////////////////

class ConnectingHandler : public StreamHandler {
public:
    ConnectingHandler(Scheduler* scheduler, StreamProtocol* protocol, ConnectorBase* connector);
    virtual ~ConnectingHandler();

public:
    // public api (called from ConnectingProtocol):

    void setReconnectingTime(int reconnecting_secs);

public:
    // inbound from selector
    void doConnect(SelectionKey* key);

    virtual bool isConnected();
    void initiateConnect();

private:
    virtual void connected();
    virtual void disconnected();

    void connectTimeout();
    void connectFailed(const std::string& msg);

private:
    ConnectorBase* connector;
    bool is_connected;
    int reconnecting_secs;

    // configurable
    int reset_reconnecting_secs;

    bool connect_in_progress;

    DEFERRED(ConnectInitiate, ConnectingHandler, initiateConnect);
    ConnectInitiate connect_initiate_cb;

    DEFERRED(ConnectTimeout, ConnectingHandler, connectTimeout);
    ConnectTimeout connect_timeout_cb;
};

///////////////////////////////////////////////////////////////////////////////

ConnectingHandler::ConnectingHandler(Scheduler* scheduler, StreamProtocol* protocol,
                                     ConnectorBase* connector) :
    StreamHandler(scheduler, protocol),
    connector(connector),
    is_connected(false),
    reconnecting_secs(1),
    reset_reconnecting_secs(1),
    connect_in_progress(false),
    connect_initiate_cb(scheduler, this),
    connect_timeout_cb(scheduler, this) {

    // start the ball rolling
    this->connect_initiate_cb.callLater(0);
}

ConnectingHandler::~ConnectingHandler() {
}

void ConnectingHandler::setReconnectingTime(int reconnecting_secs) {
    this->reconnecting_secs = this->reset_reconnecting_secs = reconnecting_secs;
}

void ConnectingHandler::doConnect(SelectionKey* key) {
    TRACE("ConnectingHandler::doConnect()");
    ConnectingSocket* connecting_sock = static_cast <ConnectingSocket*> (this->sock);

    ASSERT (this->connect_in_progress);

    try {

        if (connecting_sock->connect()) {
            this->connected();
        }

    } catch (const SocketError& exc) {
        this->connectFailed(exc.getMessage());
    }
}

bool ConnectingHandler::isConnected() {
    return this->is_connected;
}

void ConnectingHandler::initiateConnect() {
    // cancel just incase
    this->connect_initiate_cb.cancel();

    // create and connect
    ConnectingSocket* connecting_sock = this->connector->createSocket();

    connecting_sock->setBlocking(false);

    try {
        connecting_sock->connect();
    } catch (SocketError& exc) {
        K273::l_warning("Connect error %s", exc.getMessage().c_str());
        // do nothing, will clean up with OP_CONNECT
    }

    this->key = this->scheduler->registerHandler(this, connecting_sock->fileno(), OP_CONNECT);

    // hang on to it
    this->sock = connecting_sock;

    // set timeout for connection
    this->connect_timeout_cb.callLater(1000);

    this->connect_in_progress = true;
}

void ConnectingHandler::connected() {
    // cancel the timeout
    this->connect_timeout_cb.cancel();

    // connection variables
    this->connect_in_progress = false;
    this->is_connected = true;
    this->reconnecting_secs = this->reset_reconnecting_secs;

    // set up socket key
    this->key->setOps(OP_READ);

    // call protocol
    //this->protocol->logically_connected = true;

    this->updateReadTimeout();
    this->protocol->connectionMade();

    TRACE("ConnectingHandler::connected %d", this->sock->fileno());
}

void ConnectingHandler::disconnected() {
    if (this->isConnected()) {
        this->key->cancel();

        // cancel the flush and clear buffers
        this->outbuf.clear();
        this->inbuf.clear();

        // cancel the timer if exists
        this->read_timeout_cb.cancel();

        // reset flag
        this->write_waiting_for_os = false;

        // now we are not connected
        this->is_connected = false;

        //this->protocol->logically_connected = false;
        this->protocol->connectionLost();

        TRACE("ConnectingHandler::disconnected %d", this->sock->fileno());

        this->cleanupSocket();

        // attempt to reconnect?
        if (this->reconnecting_secs) {
            K273::l_info("Retrying to connect in %d seconds", this->reconnecting_secs);
            this->connect_initiate_cb.callLater(this->reconnecting_secs * 1000);

        } else {
            K273::l_info("Disconnected and not attempting to reconnect");
        }
    }
}

void ConnectingHandler::connectTimeout() {
    this->connectFailed("Timeout!");
}

void ConnectingHandler::connectFailed(const string& msg) {
    ASSERT (this->connect_in_progress && !this->isConnected());

    // report
    ConnectingProtocol* connecting_protocol = static_cast<ConnectingProtocol*> (this->protocol);
    connecting_protocol->onConnectFailed(msg);

    // cancel key
    this->key->cancel();

    // cleanup socket
    this->cleanupSocket();

    // back off to some threshold
    if (this->reconnecting_secs < 16) {
        this->reconnecting_secs *= 2;
    }

    // try to reconnect later
    if (this->reconnecting_secs) {
        K273::l_info("Retrying to connect in %d seconds", this->reconnecting_secs);
        this->connect_initiate_cb.callLater(this->reconnecting_secs * 1000);

    } else {
        K273::l_info("Failed and not retrying");
    }

    // cancel the timeout
    this->connect_timeout_cb.cancel();

    // not in progress anymore
    this->connect_in_progress = false;
}

///////////////////////////////////////////////////////////////////////////////

ConnectorBase::ConnectorBase(Scheduler* scheduler) :
    scheduler(scheduler) {
}

ConnectorBase::~ConnectorBase() {
}

///////////////////////////////////////////////////////////////////////////////

TCPConnector::TCPConnector(Scheduler* scheduler, const string& ipaddr, int port) :
    ConnectorBase(scheduler),
    ipaddr(ipaddr),
    port(port) {
}

TCPConnector::~TCPConnector() {
}

ConnectingSocket* TCPConnector::createSocket() {
    return new TcpConnectingSocket(this->ipaddr, this->port);
}

///////////////////////////////////////////////////////////////////////////////

UnixConnector::UnixConnector(Scheduler* scheduler, const string& path) :
    ConnectorBase(scheduler),
    path(path) {
}

UnixConnector::~UnixConnector() {
}

ConnectingSocket* UnixConnector::createSocket() {
    return new UnixConnectingSocket(path);
}

///////////////////////////////////////////////////////////////////////////////

ConnectingProtocol::ConnectingProtocol(ConnectorBase* connector) :
    StreamProtocol(connector->scheduler,
                   new ConnectingHandler(connector->scheduler, this, connector)) {
}

ConnectingProtocol::~ConnectingProtocol() {
}

void ConnectingProtocol::connect() {
    if (!this->isConnected()) {
        ConnectingHandler* con_handler = static_cast <ConnectingHandler*> (this->handler);
        con_handler->initiateConnect();
    }
}

void ConnectingProtocol::setReconnectingTime(int reconnecting_secs) {
    ConnectingHandler* con_handler = static_cast <ConnectingHandler*> (this->handler);
    con_handler->setReconnectingTime(reconnecting_secs);
}

void ConnectingProtocol::onConnectFailed(const std::string& msg) {
    K273::l_error("Connection failed for %s : %s",
                  this->repr().c_str(),
                  msg.c_str());
}
