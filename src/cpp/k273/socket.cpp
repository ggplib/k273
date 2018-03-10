
// local includes
#include "k273/socket.h"
#include "k273/logging.h"
#include "k273/exception.h"

// std includes
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;

///////////////////////////////////////////////////////////////////////////////

SocketError::SocketError(const std::string& msg) :
    SysException(msg, errno) {
}

SocketError::~SocketError() {
}

///////////////////////////////////////////////////////////////////////////////

static int inetSocket() {
    /* Create the socket file descriptor. */

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw SocketError("Could not create inet socket.");
    }

    return sockfd;
}

static int unixSocket() {
    /* Create the socket file descriptor. */

    int sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw SocketError("Could not create unix socket.");
    }

    return sockfd;
}

static int udpSocket() {
    /* Create the socket file descriptor. */

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw SocketError("Could not create udp socket.");
    }

    return sockfd;
}

static struct sockaddr_in inetAddress(const string& ipaddr, int port) {
    /* Create an inet address with our desired properties.  */

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    inet_aton(ipaddr.c_str(), &addr.sin_addr);
    addr.sin_port = htons(port);
    if (ipaddr == "0.0.0.0") {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    return addr;
}

// XXX where to get this?
#define UNIX_PATH_MAX    108

static struct sockaddr_un unixAddress(const string& path) {
    ASSERT (path.size() < UNIX_PATH_MAX + 1);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), UNIX_PATH_MAX);
    addr.sun_path[UNIX_PATH_MAX - 1] = '\0';

    return addr;
}

///////////////////////////////////////////////////////////////////////////////

Socket::Socket(int sockfd) :
    sockfd(sockfd) {
}

Socket::~Socket() {
}

int Socket::fileno() {
    return this->sockfd;
}

void Socket::close(bool throws) {
    int status = ::close(this->sockfd);
    if (throws && status != 0) {
        throw SocketError("In close");
    }
}

void Socket::shutdown(bool throws) {
    int status = ::shutdown(this->sockfd, SHUT_RDWR);
    if (throws && status != 0) {
        throw SocketError("In shutdown");
    }
}

void Socket::setBlocking(bool block) {
    K273::l_verbose("setting O_NONBLOCK(%s) on %d", block ? "false" : "true", this->sockfd);
    int delay_flag = fcntl(this->sockfd, F_GETFL, 0);
    if (block) {
        delay_flag &= ~O_NONBLOCK;
    } else {
        delay_flag |= O_NONBLOCK;
    }

    fcntl(this->sockfd, F_SETFL, delay_flag);
}

void Socket::setTcpNoDelay(bool enable) {
    int delay_flag = (int) enable;
    K273::l_debug("setting setTcpNoDelay(%s) on %d", enable ? "true" : "false", this->sockfd);
    if (setsockopt(this->sockfd, IPPROTO_TCP, TCP_NODELAY, &delay_flag, sizeof(int)) != 0) {
        throw SocketError("Socket::setTCPNoDelay");
    }
}

void Socket::setMaxReceiveBuffer() {
    // TODO: grab max from proc
    int max_buffer_size = 16777216;
    K273::l_debug("Trying to set setMaxReceiveBuffer() to %d on %d", max_buffer_size, this->sockfd);
    if (setsockopt(this->sockfd, SOL_SOCKET, SO_RCVBUF, &max_buffer_size, sizeof(max_buffer_size)) != 0) {
        throw SocketError("Socket::setMaxReceiveBuffer");
    }

    int current_size = 0;
    int size = 0;
    size = sizeof(int);
    if (getsockopt(this->sockfd, SOL_SOCKET, SO_RCVBUF, &current_size, (socklen_t*)&size) != 0) {
        throw SocketError("Socket::setMaxReceiveBuffer getsockopt");
     }

    K273::l_debug("Actually set setMaxReceiveBuffer() to %d on %d", current_size, this->sockfd);
}

void Socket::setMaxSendBuffer() {
    // TODO: grab max from proc
    int max_buffer_size = 16777216;
    K273::l_debug("Trying to set setMaxSendBuffer() to %d on %d", max_buffer_size, this->sockfd);
    if (setsockopt(this->sockfd, SOL_SOCKET, SO_SNDBUF, &max_buffer_size, sizeof(max_buffer_size)) != 0) {
        throw SocketError("Socket::setMaxSendBuffer");
    }

    int current_size = 0;
    int size = 0;
    size = sizeof(int);
    if (getsockopt(this->sockfd, SOL_SOCKET, SO_SNDBUF, &current_size, (socklen_t*)&size) != 0) {
        throw SocketError("Socket::setMaxSendBuffer getsockopt");
     }

    K273::l_debug("Actually set setMaxSendBuffer() to %d on %d", current_size, this->sockfd);
}

///////////////////////////////////////////////////////////////////////////////

UnconnectedSocket::UnconnectedSocket(int fd) :
    Socket(fd) {
    K273::l_debug("creating UnconnectedSocket with fd %d", fd);
}

UnconnectedSocket::~UnconnectedSocket() {
}

int UnconnectedSocket::recv(char *buf, int size, int flags) {
    this->fromlen = sizeof(struct sockaddr_un);
    int bytes = ::recvfrom(this->sockfd, buf, size, flags, (sockaddr *) &this->from, &fromlen);
    if (bytes < 0) {
        if (errno == EAGAIN) {
            return -1;
        }

        throw SocketError("in recvFrom");
    }

    return bytes;
}

///////////////////////////////////////////////////////////////////////////////

string UnconnectedSocket::getWhoFrom() {
    // get the peer
    char peerbuf[NI_MAXHOST];
    int error = getnameinfo((sockaddr *) &this->from, this->fromlen, peerbuf, sizeof(peerbuf), nullptr, 0, NI_NUMERICHOST);
    if (error) {
        throw SocketError("UnconnectedSocket::getWhoFrom()");
    }

    string host = peerbuf;
    return host;
}

///////////////////////////////////////////////////////////////////////////////

ConnectedSocket::ConnectedSocket(int fd) :
    Socket(fd),
    default_send_flags(0),
    mmheaders(),
    iov(),
    recvmmsg_buffer(),
    recvmmsg_results() {

    K273::l_debug("creating ConnectedSocket with fd %d", fd);
}

void ConnectedSocket::enableRecvMultiple() {

    // Ensure we didn't do this already
    ASSERT(!this->recvmmsg_buffer);

    // Allocate the buffer to store the recvmmsg results
    this->recvmmsg_buffer = std::unique_ptr<char[]>(new char[MaxBufferCount*MaxBufferSize]);

    // Initialize mmheaders for use with recvMultiple
    for (int ii=0; ii<MaxBufferCount; ii++) {

        // Initialize the iovecs with the buffers
        // All the buffers are in a continguous piece of memory so do pointer
        // arithmetic to get the correct buffer
        this->iov[ii].iov_base = this->recvmmsg_buffer.get() + MaxBufferSize * ii;
        this->iov[ii].iov_len = MaxBufferSize;

        this->mmheaders[ii].msg_hdr.msg_name = nullptr;
        this->mmheaders[ii].msg_hdr.msg_namelen = 0;

        // Give it the corresponding iov
        this->mmheaders[ii].msg_hdr.msg_iov = this->iov + ii;
        this->mmheaders[ii].msg_hdr.msg_iovlen = 1;

        this->mmheaders[ii].msg_hdr.msg_control = nullptr;
        this->mmheaders[ii].msg_hdr.msg_controllen = 0;
        this->mmheaders[ii].msg_hdr.msg_flags = 0;
    }

    // Pre-allocalte MaxBufferCount items
    this->recvmmsg_results.reserve(MaxBufferCount);
}

int ConnectedSocket::send(const char *buf, int size, int flags) {
    int send_flags = this->default_send_flags | flags;
    int bytes = ::send(this->sockfd, buf, size, send_flags);
    if (bytes < 0) {
        if (errno == EAGAIN) {
            return -1;
        }

        throw SocketError("in send");
    }

    return bytes;
}

int ConnectedSocket::recv(char *buf, int size, int flags) {
    int bytes = ::recv(this->sockfd, buf, size, flags);
    if (bytes < 0) {
        if (errno == EAGAIN) {
            return -1;
        }

        throw SocketError("in recv");
    }

    return bytes;
}

int ConnectedSocket::recvScatter(struct iovec *msg_iov, int msg_iovlen, int flags) {
    this->msg_hdr.msg_iov = msg_iov;
    this->msg_hdr.msg_iovlen = msg_iovlen;

    int bytes = ::recvmsg(this->sockfd, &this->msg_hdr, flags);

    if (bytes < 0) {
        if (errno == EAGAIN) {
            return -1;
        }

        throw SocketError("in recvScatter");
    }

    return bytes;
}

const std::vector<iovec>& ConnectedSocket::recvMultiple(int flags) {

    // Test that we have the buffers
    ASSERT(this->recvmmsg_buffer);

    // Make the system call to recvmmsg
    int count = ::recvmmsg(this->sockfd, this->mmheaders, MaxBufferCount, flags, nullptr);

    // Erase any counts from previous calls
    this->recvmmsg_results.clear();

    // Check to see if we were given any messages
    if (count == -1) {
        // If this wasn't a normal case then throw an exception
        if (errno != EAGAIN && errno != EINTR) {
            K273::l_warning("recvmmsg error: %s", ::strerror(errno));
            throw SocketError("recvmmsg");
        }

        return this->recvmmsg_results;
    }

    // Add the lengths of all the buffers to counts so the client knows how much was received
    // We checked count == -1 so safe to cast to size_t
    for (size_t i = 0; i < static_cast<size_t>(count); ++i) {

        // All the buffers are in a continguous piece of memory so do pointer
        // arithmetic to get the correct buffer
        this->recvmmsg_results.push_back(
                        iovec{this->recvmmsg_buffer.get() + MaxBufferSize*i,
                              this->mmheaders[i].msg_len});
    }

    return this->recvmmsg_results;
}

///////////////////////////////////////////////////////////////////////////////

ConnectingSocket::ConnectingSocket(int sockfd) :
    ConnectedSocket(sockfd),
    connected(false) {
}

ConnectingSocket::~ConnectingSocket() {
}

bool ConnectingSocket::connect() {

    if (this->connected) {
        return true;
    }

    /* as we connected non-blocking, we have to call this when our selector
       thinks we are ready (OP_CONNECT).  Will return true when we are
       connected (and false if we in progress for non blocking sockets). */

    if (::connect(this->sockfd, this->getAddr(), this->getAddrSize()) == -1) {
        if (errno == EINPROGRESS || errno == EALREADY) {
            return false;
        } else {
            throw SocketError("ConnectingSocket::connect()");
        }
    }

    this->connected = true;
    return true;
}

struct sockaddr *ConnectingSocket::getAddr() {
    return nullptr;
}

int ConnectingSocket::getAddrSize() {
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

TcpConnectingSocket::TcpConnectingSocket(const string& ipaddr, int port) :
    ConnectingSocket(inetSocket()) {
    this->addr = inetAddress(ipaddr, port);
}

TcpConnectingSocket::~TcpConnectingSocket() {
}

struct sockaddr *TcpConnectingSocket::getAddr() {
    return (struct sockaddr *) &this->addr;
}

int TcpConnectingSocket::getAddrSize() {
    return sizeof(struct sockaddr_in);
}

///////////////////////////////////////////////////////////////////////////////

UnixConnectingSocket::UnixConnectingSocket(const string& path) :
    ConnectingSocket(unixSocket()) {
    this->addr = unixAddress(path);
    this->default_send_flags = MSG_NOSIGNAL;
}

UnixConnectingSocket::~UnixConnectingSocket() {
}

struct sockaddr *UnixConnectingSocket::getAddr() {
    return (struct sockaddr *) &this->addr;
}

int UnixConnectingSocket::getAddrSize() {
    return strlen(this->addr.sun_path) + sizeof(this->addr.sun_family);
}

///////////////////////////////////////////////////////////////////////////////

AcceptingSocket::AcceptingSocket(int sockfd) :
    Socket(sockfd),
    listening(false) {
}

AcceptingSocket::~AcceptingSocket() {
}

bool AcceptingSocket::isListening() {
    return this->listening;
}

void AcceptingSocket::listen(int backlog) {
    ASSERT (backlog > 0);
    int res = ::listen(this->sockfd, backlog);
    if (res == -1) {
        throw SocketError("AcceptingSocket::listen()");
    }

    this->listening = true;
}

ConnectedSocket *AcceptingSocket::accept() {
    ASSERT (this->listening);

    socklen_t addrlen = this->getAddrSize();
    int res_fd = ::accept(this->sockfd, this->getAddr(), &addrlen);
    if (res_fd == -1) {
        if (errno == EAGAIN) {
            return nullptr;
        }

        throw SocketError("AcceptingSocket::accept()");
    }

    return this->createSocket(res_fd);
}

struct sockaddr *AcceptingSocket::getAddr() {
    return nullptr;
}

int AcceptingSocket::getAddrSize() {
    return 0;
}

ConnectedSocket *AcceptingSocket::createSocket(int fd) {
    return new ConnectedSocket(fd);
}

///////////////////////////////////////////////////////////////////////////////

TcpAcceptingSocket::TcpAcceptingSocket(const string& ipaddr, int port) :
    AcceptingSocket(inetSocket()) {

    this->addr = inetAddress(ipaddr, port);

    int on = 1;
    int status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (status == -1) {
        throw SocketError("TCP socket TIME_WAIT.");
    }

    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
        throw SocketError("Bind on socket.");
    }

    K273::l_info("Created listening TCP socket on interface %s port %d", ipaddr.c_str(), port);
}

TcpAcceptingSocket::~TcpAcceptingSocket() {
}

struct sockaddr *TcpAcceptingSocket::getAddr() {
    return (struct sockaddr *) &this->addr;
}

int TcpAcceptingSocket::getAddrSize() {
    return sizeof(struct sockaddr_in);
}

///////////////////////////////////////////////////////////////////////////////

UnixAcceptingSocket::UnixAcceptingSocket(const string& path) :
    AcceptingSocket(unixSocket()),
    path(path) {

    this->addr = unixAddress(path);

    int on = 1;
    int status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (status == -1) {
        throw SocketError("UNIX socket SO_REUSEADDR");
    }

    unlink(this->path.c_str());

    if (bind(sockfd, this->getAddr(), this->getAddrSize()) == -1) {
        throw SocketError("Bind on socket.");
    }

    K273::l_info("Created listening UNIX socket at path %s", path.c_str());
}

UnixAcceptingSocket::~UnixAcceptingSocket() {
    unlink(this->path.c_str());
}

struct sockaddr *UnixAcceptingSocket::getAddr() {
    return (struct sockaddr *) &this->addr;
}

int UnixAcceptingSocket::getAddrSize() {
    return strlen(this->addr.sun_path) + sizeof(this->addr.sun_family);
}

ConnectedSocket *UnixAcceptingSocket::createSocket(int fd) {
    ConnectedSocket *s = new ConnectedSocket(fd);
    s->default_send_flags = MSG_NOSIGNAL;
    return s;
}

///////////////////////////////////////////////////////////////////////////////

ConnectedSocket* K273::udpReadSocket(const string& ipaddr, int port, const string& ifn) {
    /* Given a ip address and port, create a bound udp socket for reading. */

    K273::l_debug("binding to %s:%d on %s", ipaddr.c_str(), port, ifn.size() == 0 ? "any" : ifn.c_str());

    int sockfd = udpSocket();
    int on = 1;

    // Allow more than one binding
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        throw SocketError("Could not set broadcast permissions.");
    }

    sockaddr_in addr = inetAddress(ipaddr, port);

    if (ifn.size() != 0) {
        ifreq ifreq;
        strncpy(ifreq.ifr_name, ifn.c_str(), IFNAMSIZ);

        if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0) {
            throw SocketError("invalid network interface");
        }

        addr.sin_addr = ((sockaddr_in*)&ifreq.ifr_addr)->sin_addr;
    }

    if (bind(sockfd, (sockaddr*) &addr, sizeof(sockaddr)) == -1) {
        throw SocketError("Bind on socket.");
    }

    ConnectedSocket* s = new ConnectedSocket(sockfd);
    s->setBlocking(false);
    return s;
}

ConnectedSocket* K273::udpWriteSocket(const string& ipaddr, int port, bool broadcast) {
    /* Given a ip address and port, create a connected udp socket for writing. */

    // Create the address and socket
    K273::l_debug("connecting to %s:%d", ipaddr.c_str(), port);

    int sockfd = udpSocket();
    if (broadcast) {
        // Enable broadcast permissions
        int on = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) == -1) {
            throw SocketError("Could not set broadcast permissions.");
        }
    }

    sockaddr_in addr = inetAddress(ipaddr, port);

    // Connect the socket with a single address so that we don't have to
    // specify it every time in sendto().
    if (connect(sockfd, (sockaddr*) &addr, sizeof(sockaddr)) == -1) {
        throw SocketError("Connect on socket.");
    }

    ConnectedSocket* s = new ConnectedSocket(sockfd);
    s->setBlocking(false);
    return s;
}

ConnectedSocket* K273::multicastSocket(const string& group, int port, const string& ifn) {

    K273::l_debug("joining multicast group %s:%d on %s",
                  group.c_str(), port, ifn.size() == 0 ? "any" : ifn.c_str());

    int fd = udpSocket();
    int on = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        throw SocketError("Could not set SO_REUSEADDR");
    }

    sockaddr_in in_addr = inetAddress(group, port);
    if (bind(fd, (sockaddr*) &in_addr, sizeof(sockaddr)) == -1) {
        throw SocketError("Could not bind socket port");
    }

    ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(group.c_str());

    if (ifn.size() == 0) {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    } else {
        ifreq ifreq;
        std::strncpy(ifreq.ifr_name, ifn.c_str(), IFNAMSIZ);

        if (ioctl(fd, SIOCGIFADDR, &ifreq) < 0) {
            throw SocketError("invalid multicast interface");
        }

        const sockaddr_in& sin = *(sockaddr_in*)&ifreq.ifr_addr;
        mreq.imr_interface.s_addr = sin.sin_addr.s_addr;
    }

    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
        throw SocketError("Could not join multicast group");
    }

    ConnectedSocket *s = new ConnectedSocket(fd);
    s->setBlocking(false);
    return s;
}

ConnectedSocket* K273::multicastWriteSocket(const string& group, int port, const string& ifn) {

    K273::l_debug("connecting to multicast %s:%d via %s",
                  group.c_str(), port, ifn.size() == 0 ? "any" : ifn.c_str());

    int fd = udpSocket();
    int on = 1;

    K273::l_debug("enabling multicast loopback on %d", fd);

    if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on)) == -1) {
        throw SocketError("Could not enable multicast loopback");
    }

    if (ifn.size() != 0) {
        in_addr inaddr;
        ifreq   ifreq;

        strncpy(ifreq.ifr_name, ifn.c_str(), IFNAMSIZ);

        if (ioctl(fd, SIOCGIFADDR, &ifreq) < 0) {
            throw SocketError("invalid multicast interafce");
        }

        const sockaddr_in& sin = *(sockaddr_in*)&ifreq.ifr_addr;
        inaddr.s_addr = sin.sin_addr.s_addr;

        if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &inaddr,
                       sizeof(in_addr)) < 0) {
            throw SocketError("Could not set multicast output interface");
        }
    }

    sockaddr_in addr = inetAddress(group, port);

    if (connect(fd, (sockaddr*)&addr, sizeof(sockaddr)) == -1) {
        throw SocketError("Connect on multicast socket.");
    }

    ConnectedSocket* s = new ConnectedSocket(fd);
    s->setBlocking(false);
    return s;
}

