#pragma once

// k273 includes
#include <k273/exception.h>

// std includes
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <vector>

namespace K273::Kelvin {

    class SocketError: public SysException {
      public:
        SocketError(const std::string& msg);
        virtual ~SocketError();
    };

    ///////////////////////////////////////////////////////////////////////////

    class Socket {
      public:
        Socket(int sockfd);
        virtual ~Socket();

      public:
        int fileno();

        void close(bool throws=true);
        void shutdown(bool throws=true);

        void setBlocking(bool block);

        // only makes sense on tcp connected sockets - no enforcement
        void setTcpNoDelay(bool enable);

        // Maximizes receive buffer of socket to the most allowed by the OS.
        void setMaxReceiveBuffer();
        void setMaxSendBuffer();

      protected:
        int sockfd;
    };

    ///////////////////////////////////////////////////////////////////////////

    class UnconnectedSocket : public Socket {
      public:
        UnconnectedSocket(int sockfd);
        virtual ~UnconnectedSocket();

      public:
        int recv(char* buf, int size, int flags=0);
        std::string getWhoFrom();

      private:
        struct sockaddr_un from;
        socklen_t fromlen;
    };

    ///////////////////////////////////////////////////////////////////////////

    class ConnectedSocket : public Socket {
        /* a socket that may be bound or connected */

      public:
        ConnectedSocket(int sockfd);

        ///
        /// Prepare the socket for calls to recvMultiple.
        /// This must be called before any calls to recvMultiple so the
        /// proper data members are initialized.
        ///
        void enableRecvMultiple();

      public:
        int recv(char* buf, int size, int flags=0);
        int recvScatter(struct iovec* msg_iov, int msg_iovlen, int flags=0);
        ///
        /// Receive multiple messages from the socket
        /// @param flags Receive flags that are past to recvmmsg
        /// @return A vector<iovec> containing buffers and sizes for each returned message
        ///
        const std::vector<iovec>& recvMultiple(int flags=0);

        int send(const char* buf, int size, int flags=0);

      public:
        int default_send_flags;

        // only used for scatter recv
        msghdr msg_hdr;

      private:
        // Members for use with recvMultiple

        // Maximum number of buffers to write messages into
        constexpr static int MaxBufferCount = 128;
        constexpr static int MaxBufferSize = 2048;

        // Array of mmshdr structs that contain msg_hdr and msg_len
        mmsghdr mmheaders[MaxBufferCount];

        // Array of iovec structs that contain a pointer to the buffer and its length
        iovec iov[MaxBufferCount];

        // Buffers for storing data from recvmmsg
        std::unique_ptr<char[]> recvmmsg_buffer;

        // Vector used to return the buffers and sizes from recvmmsg
        std::vector<iovec> recvmmsg_results;
    };

    ///////////////////////////////////////////////////////////////////////////

    // bunch of connecting sockets

    class ConnectingSocket : public ConnectedSocket {
      public:
        ConnectingSocket(int sockfd);
        virtual ~ConnectingSocket();

      public:
        bool isConnected();
        bool connect();

      protected:
        virtual struct sockaddr* getAddr() = 0;
        virtual int getAddrSize() = 0;

      protected:
        bool connected;
    };

    class TcpConnectingSocket : public ConnectingSocket {
      public:
        TcpConnectingSocket(const std::string& ipaddr, int port);
        virtual ~TcpConnectingSocket();

      protected:
        virtual struct sockaddr* getAddr();
        virtual int getAddrSize();

      private:
        struct sockaddr_in addr;
    };

    class UnixConnectingSocket : public ConnectingSocket {
      public:
        UnixConnectingSocket(const std::string& path);
        virtual ~UnixConnectingSocket();

      protected:
        virtual struct sockaddr* getAddr();
        virtual int getAddrSize();

      private:
        struct sockaddr_un addr;
    };

    ///////////////////////////////////////////////////////////////////////////

    // bunch of accepting sockets

    class AcceptingSocket : public Socket {
      public:
        AcceptingSocket(int sockfd);
        virtual ~AcceptingSocket();

      public:
        bool isListening();
        void listen(int backlog);
        ConnectedSocket* accept();

      protected:
        virtual struct sockaddr* getAddr();
        virtual int getAddrSize();
        virtual ConnectedSocket* createSocket(int fd);

      private:
        int listening;
    };

    class TcpAcceptingSocket : public AcceptingSocket {
      public:
        TcpAcceptingSocket(const std::string& ipaddr, int port);
        virtual ~TcpAcceptingSocket();

      protected:
        virtual struct sockaddr* getAddr();
        virtual int getAddrSize();

      private:
        struct sockaddr_in addr;
    };

    class UnixAcceptingSocket : public AcceptingSocket {
      public:
        UnixAcceptingSocket(const std::string& path);
        virtual ~UnixAcceptingSocket();

      protected:
        virtual struct sockaddr* getAddr();
        virtual int getAddrSize();
        virtual ConnectedSocket* createSocket(int fd);

      private:
        std::string path;
        struct sockaddr_un addr;
    };

    ///////////////////////////////////////////////////////////////////////////
    // UDP socket factories (XXX condense into one function?)

    // (XXX fix up stupid fucking naming)

    /** Creates UDP socket and binds it for reading on \a ipaddr and \a port
     *  with optional binding to a given network interafce, which overwrites
     *  the IP address.
     */
    ConnectedSocket* udpReadSocket(const std::string& ipaddr, int port, const std::string& ifn="");

    /** Creates UDP socket and connects it for output to given IP address
     *  and port optionally enabling broadcast output.
     */
    ConnectedSocket* udpWriteSocket(const std::string& ipaddr, int port, bool broadcast=false);

    /** Creates UDP socket, binds it to given port and joins it to given
     *  multicast group. Optional interface name overwrites the binding to
     *  the address of the interface.
     */
    ConnectedSocket* multicastSocket(const std::string& group, int port, const std::string& ifn="");

    /** Creates UDP socket and connects it to multicast group and port
     *  for output, optionally setting multicast output interface.
     *  Always enables multicast loopback.
     */
    ConnectedSocket* multicastWriteSocket(const std::string& group, int port,
                                          const std::string& ifn="");

}
