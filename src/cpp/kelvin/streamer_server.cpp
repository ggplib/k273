
// local includes
#include "kelvin/streamer_server.h"
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
using namespace K273::Kelvin;
using namespace K273::Kelvin::Streamer;

///////////////////////////////////////////////////////////////////////////////

static const bool _debug_log = false;
#define TRACE(fmt, args...) if (_debug_log) { K273::l_debug(fmt, ## args); }

///////////////////////////////////////////////////////////////////////////////

class ChildHandler : public StreamHandler {
public:
    ChildHandler(Scheduler* scheduler,
                 StreamProtocol* protocol,
                 ConnectedSocket* conn_sock,
                 ServerHandler* parent);

    virtual ~ChildHandler();

public:
    // inbound from selector
    void doConnect(SelectionKey* key);

    virtual bool isConnected();

private:
    virtual void connected();
    virtual void disconnected();

private:
    ServerHandler* parent;
    bool is_connected;
};

///////////////////////////////////////////////////////////////////////////////

class K273::Kelvin::Streamer::ServerHandler : public EventHandler {
public:
    ServerHandler(Server* server);
    virtual ~ServerHandler();

public:
    // called via deffered
    virtual void onWakeup();

    // called via scheduler event handler
    virtual void doAccept(SelectionKey* key);

    // called from ChildHandler
    void childConnected(ChildProtocol* child);
    void childDisconnected(ChildProtocol* child);

    // required by EventHandler
    virtual std::string repr() const;

private:
    void cleanup();

private:
    Scheduler* scheduler;
    Server* server;
    ConfigInterface* config;
    AcceptingSocket* accept_sock;

    SelectionKey* key;
    bool initialized;

    DEFERRED(Wakeup, ServerHandler, onWakeup);
    Wakeup deferred;

    // XXX Use mapping?
    std::list <ChildProtocol*> children;
};

///////////////////////////////////////////////////////////////////////////////

ChildHandler::ChildHandler(Scheduler* scheduler,
                           StreamProtocol* protocol,
                           ConnectedSocket* conn_sock,
                           ServerHandler* parent) :
    StreamHandler(scheduler, protocol),
    parent(parent),
    is_connected(false) {
    this->sock = conn_sock;
    this->key = this->scheduler->registerHandler(this, this->sock->fileno(), OP_CONNECT);
}

ChildHandler::~ChildHandler() {
}

void ChildHandler::doConnect(SelectionKey* key) {
    ASSERT (key == this->key);
    this->connected();
}

bool ChildHandler::isConnected() {
    return this->is_connected;
}

void ChildHandler::connected() {
    this->is_connected = true;
    this->key->setOps(OP_READ);

    //ZZZXXXthis->protocol->logically_connected = true;

    this->updateReadTimeout();
    this->protocol->connectionMade();

    ChildProtocol* child_prot = static_cast <ChildProtocol*> (this->protocol);
    this->parent->childConnected(child_prot);

    TRACE("ChildHandler::connected %d", this->sock->fileno());
}

void ChildHandler::disconnected() {
    if (this->is_connected) {
        this->key->cancel();

        // cancel the flush and clear buffers
        this->outbuf.clear();

        // cancel the timer if exists
        this->read_timeout_cb.cancel();

        // reset flag
        this->write_waiting_for_os = false;

        // now we are not connected
        this->is_connected = false;

        //ZZZXXXthis->protocol->logically_connected = false;
        this->protocol->connectionLost();

        ChildProtocol* child_prot = static_cast <ChildProtocol*> (this->protocol);
        this->parent->childDisconnected(child_prot);

        this->cleanupSocket();
    }
}

///////////////////////////////////////////////////////////////////////////////

ChildProtocol::ChildProtocol(Scheduler* scheduler,
                             Server* server,
                             ConnectedSocket* conn_sock) :
    StreamProtocol(scheduler, new ChildHandler(scheduler, this, conn_sock, server->getHandler())) {
}

ChildProtocol::~ChildProtocol() {
}

///////////////////////////////////////////////////////////////////////////////

ServerHandler::ServerHandler(Server* server) :
    scheduler(server->getConfig()->scheduler),
    server(server),
    config(server->getConfig()),
    key(nullptr),
    initialized(false),
    deferred(scheduler, this) {

    // adopt accept socket
    this->accept_sock = this->config->accept_sock;
    this->config->accept_sock = nullptr;

    // initialise inside the main loop
    this->deferred.callLater(0);
}

ServerHandler::~ServerHandler() {
    this->cleanup();
    delete this->accept_sock;
    delete this->config;
}

void ServerHandler::onWakeup() {
    // called once at startup to complete initialization
    this->accept_sock->listen(this->config->backlog);
    this->accept_sock->setBlocking(false);
    this->key = this->scheduler->registerHandler(this,
                                                 this->accept_sock->fileno(),
                                                 OP_ACCEPT);
    this->initialized = true;
    TRACE("ServerHandler::onWakeup() initialized");
}

void ServerHandler::doAccept(SelectionKey* key) {
    TRACE("ServerHandler::doAccept() socket accept");
    ASSERT (this->initialized);

    while (true) {
        ConnectedSocket* child_sock = this->accept_sock->accept();

        // all done?
        if (child_sock == nullptr) {
            break;
        }

        // set the socket non blocking
        child_sock->setBlocking(false);

        ChildProtocol* child = this->config->createChild(this->server, child_sock);
        this->children.push_back(child);
    }
}

void ServerHandler::childConnected(ChildProtocol* child) {
    this->server->childConnectionMade(child);
}

void ServerHandler::childDisconnected(ChildProtocol* child) {
    // XXX slow
    this->children.remove(child);
    this->server->childConnectionLost(child);
}

string ServerHandler::repr() const {
    return "ServerHandler";
}

void ServerHandler::cleanup() {
    if (this->initialized) {
        TRACE("ServerHandler::cleanup()");
        this->key->cancel();
        this->accept_sock->shutdown(false);
        this->accept_sock->close(false);
        this->initialized = false;
    }
}

///////////////////////////////////////////////////////////////////////////////

ConfigInterface::ConfigInterface(Scheduler* scheduler, AcceptingSocket* accept_sock, int backlog) :
    scheduler(scheduler),
    accept_sock(accept_sock),
    backlog(backlog) {
}

ConfigInterface::~ConfigInterface() {
}

///////////////////////////////////////////////////////////////////////////////

Server::Server(ConfigInterface* config) :
    config(config),
    handler(new ServerHandler(this)) {
}

Server::~Server() {
    delete this->handler;
}

///////////////////////////////////////////////////////////////////////////////

void Server::childConnectionMade(ChildProtocol* child) {
    //K273::l_log("Created child %s, with fd : %d ", child->repr().c_str(), new_sock->fileno());
    TRACE("Server::childConnectionMade %s", child->repr().c_str());
}

void Server::childConnectionLost(ChildProtocol* child) {
    TRACE("Server::childConnectionLost %s", child->repr().c_str());
}

string Server::repr() const {
    return "ServerProtocol";
}

