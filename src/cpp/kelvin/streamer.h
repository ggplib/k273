#pragma once

// local includes
#include "kelvin/socket.h"
#include "kelvin/selector.h"
#include "kelvin/scheduler.h"
#include "kelvin/bytebuffer.h"

namespace Kelvin::Streamer {

    ///////////////////////////////////////////////////////////////////////////
    // Forwards

    class StreamProtocol;

    ///////////////////////////////////////////////////////////////////////////

    class StreamHandler : public EventHandler {

      public:
        StreamHandler(Scheduler* scheduler, StreamProtocol* protocol);
        virtual ~StreamHandler();

      public:
        virtual bool isConnected() = 0;

        // inbound from selector
        virtual void doRead(SelectionKey* key);
        virtual void doWrite(SelectionKey* key);
        virtual void doReadTimeout();
        virtual std::string repr() const;

      public:

        // from user
        void disconnect();
        void write(const char* ptdata, int size);
        void write(ByteBuffer& buf);
        void setReadTimeout(int timeout_secs);
        ConnectedSocket* getSocket();

        void flush();

      protected:
        virtual void connected() = 0;
        virtual void disconnected() = 0;

        void updateReadTimeout();
        void cleanupSocket();

      protected:
        SelectionKey* key;
        Scheduler* scheduler;
        StreamProtocol* protocol;

        ByteBuffer inbuf;
        ByteBuffer outbuf;

        int timeout_secs;
        bool write_waiting_for_os;
        double last_update_read_time_at;

        DEFERRED(ReadTimeout, StreamHandler, doReadTimeout);
        ReadTimeout read_timeout_cb;

        ConnectedSocket* sock;
    };

    ///////////////////////////////////////////////////////////////////////////

    class StreamProtocol {

      public:
        StreamProtocol(Scheduler* scheduler, StreamHandler* handler);
        virtual ~StreamProtocol();

      public:
        // overwritable api
        virtual void dataReceived(ByteBuffer& buf);
        virtual void onBuffer(ByteBuffer& buf);
        virtual void connectionMade();
        virtual void connectionLost();
        virtual std::string repr() const;

      public:
        // fixed api
        void disconnect();
        bool isConnected();

        void write(const char* ptdata, int size);
        void write(ByteBuffer& buf);

        void setReadTimeout(int timeout_secs);
        ConnectedSocket* getSocket();

      protected:
        Scheduler* scheduler;
        StreamHandler* handler;
        bool logically_connected;
    };

}
