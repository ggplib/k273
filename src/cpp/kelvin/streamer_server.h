#pragma once

// local includes
#include "kelvin/streamer.h"
#include "kelvin/socket.h"
#include "kelvin/selector.h"
#include "kelvin/scheduler.h"
#include "kelvin/bytebuffer.h"

// std includes
#include <list>

namespace K273 {
namespace Streamer {

    ///////////////////////////////////////////////////////////////////////////
    // Forwards
    class Server;
    class ServerHandler;
    class ServerProtocol;
    class ConfigInterface;

    ///////////////////////////////////////////////////////////////////////////

    class ChildProtocol : public StreamProtocol {
    public:
        ChildProtocol(Scheduler*, Server*, ConnectedSocket* sock);
        virtual ~ChildProtocol();
    };

    ///////////////////////////////////////////////////////////////////////////
    // some trickery here with inheritance.  This is to avoid ServerProtocol

    struct ConfigInterface {
        ConfigInterface(Scheduler*, AcceptingSocket*, int backlog);
        virtual ~ConfigInterface();

        virtual ChildProtocol* createChild(Server*, ConnectedSocket* new_sock) const = 0;

        Scheduler* scheduler;
        AcceptingSocket* accept_sock;
        int backlog;
    };

    template <typename ChildProtocol_t>
    struct Config : public ConfigInterface {

        Config(Scheduler* scheduler, AcceptingSocket* accept_sock, int backlog) :
            ConfigInterface(scheduler, accept_sock, backlog) {
        }

        virtual ~Config() {
        }

        virtual ChildProtocol* createChild(Server* server, ConnectedSocket* new_sock) const {
            return new ChildProtocol_t(this->scheduler, server, new_sock);
        }
    };

    ///////////////////////////////////////////////////////////////////////////

    class Server {
    public:
        Server(ConfigInterface* config);
        virtual ~Server();

    public:
        virtual void childConnectionMade(ChildProtocol* child);
        virtual void childConnectionLost(ChildProtocol* child);
        virtual std::string repr() const;

        ConfigInterface* getConfig() const {
            return this->config;
        };

        ServerHandler* getHandler() const {
            return this->handler;
        };

    private:
        ConfigInterface* config;
        ServerHandler* handler;
    };

    ///////////////////////////////////////////////////////////////////////////

    template <typename ChildProtocol_t>
    ConfigInterface* unixConfigHelper(Scheduler* scheduler, const std::string& path) {
        AcceptingSocket* accepting_socket = new UnixAcceptingSocket(path);
        return new Config <ChildProtocol_t> (scheduler, accepting_socket, 10);
    }

    template <typename ChildProtocol_t>
    ConfigInterface* tcpConfigHelper(Scheduler* scheduler, const std::string& ipaddr, int port) {
        AcceptingSocket* accepting_socket = new TcpAcceptingSocket(ipaddr, port);
        return new Config <ChildProtocol_t> (scheduler, accepting_socket, 10);
    }

    ///////////////////////////////////////////////////////////////////////////

}
}
