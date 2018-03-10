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
    class ConnectorBase;
    class ConnectingProtocol;

    ///////////////////////////////////////////////////////////////////////////
    // connectors:

    class ConnectorBase {
    public:
        ConnectorBase(Scheduler* scheduler);
        virtual ~ConnectorBase();

    public:
        virtual ConnectingSocket* createSocket() = 0;

    public:
        Scheduler* scheduler;
    };

    class TCPConnector : public ConnectorBase  {
    public:
        TCPConnector(Scheduler* scheduler, const std::string& ipaddr, int port);
        virtual ~TCPConnector();

    public:
        virtual ConnectingSocket* createSocket();

    private:
        std::string ipaddr;
        int port;
    };

    class UnixConnector : public ConnectorBase  {
    public:
        UnixConnector(Scheduler* scheduler, const std::string& path);
        virtual ~UnixConnector();

    public:
        virtual ConnectingSocket* createSocket();

    private:
        std::string path;
    };

    ///////////////////////////////////////////////////////////////////////////

    class ConnectingProtocol : public StreamProtocol {
    public:
        ConnectingProtocol(ConnectorBase* connector);
        virtual ~ConnectingProtocol();

    public:
        // Public api:

        // force a connect (if already connected, will continue)
        void connect();

        // If reconnecting_secs == 0, does attempt to reconnect (see reset_reconnecting_secs)
        void setReconnectingTime(int reconnecting_secs);

        // can be overriden
        virtual void onConnectFailed(const std::string& msg);
    };

}
}
