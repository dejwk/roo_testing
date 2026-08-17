#include <array>
#include <chrono>
#include <cstring>
#include <future>
#include <string>
#include <thread>

#include "NetworkClient.h"
#include "NetworkServer.h"
#include "NetworkUdp.h"
#include "gtest/gtest.h"
#include "lwip/sockets.h"

namespace {

using namespace std::chrono_literals;

class Socket {
public:
  explicit Socket(int descriptor = -1) : descriptor_(descriptor) {}
  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;
  Socket(Socket &&other) noexcept : descriptor_(other.release()) {}
  Socket &operator=(Socket &&other) noexcept {
    if (descriptor_ >= 0) {
      close(descriptor_);
    }
    descriptor_ = other.release();
    return *this;
  }
  ~Socket() {
    if (descriptor_ >= 0) {
      close(descriptor_);
    }
  }

  int get() const { return descriptor_; }
  int release() {
    int descriptor = descriptor_;
    descriptor_ = -1;
    return descriptor;
  }

private:
  int descriptor_;
};

struct BoundSocket {
  Socket socket;
  uint16_t port;
};

BoundSocket BindLoopback(int type) {
  Socket socket(::socket(AF_INET, type, 0));
  if (socket.get() < 0) {
    return {Socket(), 0};
  }

  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = 0;
  if (bind(socket.get(), reinterpret_cast<sockaddr *>(&address),
           sizeof(address)) != 0) {
    return {Socket(), 0};
  }

  socklen_t address_size = sizeof(address);
  if (getsockname(socket.get(), reinterpret_cast<sockaddr *>(&address),
                  &address_size) != 0) {
    return {Socket(), 0};
  }
  return {Socket(socket.release()), ntohs(address.sin_port)};
}

bool WaitForReadable(int descriptor, std::chrono::milliseconds timeout) {
  pollfd event = {descriptor, POLLIN, 0};
  return poll(&event, 1, static_cast<int>(timeout.count())) == 1 &&
         (event.revents & POLLIN) != 0;
}

TEST(HostNetworkTest, NetworkClientUsesLinuxTcpAndResolver) {
  BoundSocket listener = BindLoopback(SOCK_STREAM);
  ASSERT_GE(listener.socket.get(), 0);
  ASSERT_NE(listener.port, 0);
  ASSERT_EQ(listen(listener.socket.get(), 1), 0);

  auto server = std::async(std::launch::async, [&listener]() {
    if (!WaitForReadable(listener.socket.get(), 3s)) {
      return false;
    }
    Socket connection(accept(listener.socket.get(), nullptr, nullptr));
    if (connection.get() < 0 || !WaitForReadable(connection.get(), 3s)) {
      return false;
    }
    std::array<char, 4> request = {};
    if (recv(connection.get(), request.data(), request.size(), MSG_WAITALL) !=
            static_cast<ssize_t>(request.size()) ||
        std::string(request.data(), request.size()) != "ping") {
      return false;
    }
    constexpr char response[] = "pong";
    return send(connection.get(), response, sizeof(response) - 1, 0) ==
           sizeof(response) - 1;
  });

  NetworkClient client;
  ASSERT_EQ(client.connect("localhost", listener.port, 2000), 1);
  constexpr char request[] = "ping";
  ASSERT_EQ(client.write(reinterpret_cast<const uint8_t *>(request),
                         sizeof(request) - 1),
            sizeof(request) - 1);

  const auto deadline = std::chrono::steady_clock::now() + 3s;
  while (client.available() < 4 && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(1ms);
  }
  std::array<uint8_t, 4> response = {};
  EXPECT_EQ(client.read(response.data(), response.size()), response.size());
  EXPECT_EQ(std::string(response.begin(), response.end()), "pong");
  EXPECT_TRUE(server.get());
  client.stop();
}

TEST(HostNetworkTest, NetworkServerAcceptsLinuxTcpConnections) {
  BoundSocket reservation = BindLoopback(SOCK_STREAM);
  ASSERT_NE(reservation.port, 0);
  const uint16_t port = reservation.port;
  reservation.socket = Socket();

  NetworkServer server(IPAddress(127, 0, 0, 1), port);
  server.begin();
  ASSERT_TRUE(server);

  Socket peer(::socket(AF_INET, SOCK_STREAM, 0));
  ASSERT_GE(peer.get(), 0);
  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(port);
  ASSERT_EQ(connect(peer.get(), reinterpret_cast<sockaddr *>(&address),
                    sizeof(address)),
            0);

  NetworkClient accepted;
  const auto deadline = std::chrono::steady_clock::now() + 3s;
  while (!accepted && std::chrono::steady_clock::now() < deadline) {
    accepted = server.accept();
    if (!accepted) {
      std::this_thread::sleep_for(1ms);
    }
  }
  ASSERT_TRUE(accepted);
  constexpr char response[] = "accepted";
  ASSERT_EQ(accepted.write(reinterpret_cast<const uint8_t *>(response),
                           sizeof(response) - 1),
            sizeof(response) - 1);

  ASSERT_TRUE(WaitForReadable(peer.get(), 3s));
  std::array<char, sizeof(response) - 1> actual = {};
  ASSERT_EQ(recv(peer.get(), actual.data(), actual.size(), MSG_WAITALL),
            actual.size());
  EXPECT_EQ(std::string(actual.begin(), actual.end()), "accepted");
  server.end();
}

TEST(HostNetworkTest, NetworkUdpSendsAndReceivesLinuxDatagrams) {
  BoundSocket sink = BindLoopback(SOCK_DGRAM);
  ASSERT_GE(sink.socket.get(), 0);

  NetworkUDP sender;
  ASSERT_EQ(sender.beginPacket("localhost", sink.port), 1);
  constexpr char outgoing[] = "udp-out";
  ASSERT_EQ(sender.write(reinterpret_cast<const uint8_t *>(outgoing),
                         sizeof(outgoing) - 1),
            sizeof(outgoing) - 1);
  ASSERT_EQ(sender.endPacket(), 1);
  ASSERT_TRUE(WaitForReadable(sink.socket.get(), 3s));
  std::array<char, sizeof(outgoing) - 1> received = {};
  ASSERT_EQ(recv(sink.socket.get(), received.data(), received.size(), 0),
            received.size());
  EXPECT_EQ(std::string(received.begin(), received.end()), "udp-out");

  BoundSocket reservation = BindLoopback(SOCK_DGRAM);
  ASSERT_NE(reservation.port, 0);
  const uint16_t receiver_port = reservation.port;
  reservation.socket = Socket();

  NetworkUDP receiver;
  ASSERT_EQ(receiver.begin(IPAddress(127, 0, 0, 1), receiver_port), 1);
  Socket source(::socket(AF_INET, SOCK_DGRAM, 0));
  ASSERT_GE(source.get(), 0);
  sockaddr_in destination = {};
  destination.sin_family = AF_INET;
  destination.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  destination.sin_port = htons(receiver_port);
  constexpr char incoming[] = "udp-in";
  ASSERT_EQ(sendto(source.get(), incoming, sizeof(incoming) - 1, 0,
                   reinterpret_cast<sockaddr *>(&destination),
                   sizeof(destination)),
            sizeof(incoming) - 1);

  int packet_size = 0;
  const auto deadline = std::chrono::steady_clock::now() + 3s;
  while (packet_size == 0 && std::chrono::steady_clock::now() < deadline) {
    packet_size = receiver.parsePacket();
    if (packet_size == 0) {
      std::this_thread::sleep_for(1ms);
    }
  }
  ASSERT_EQ(packet_size, sizeof(incoming) - 1);
  std::array<char, sizeof(incoming) - 1> actual = {};
  ASSERT_EQ(receiver.read(actual.data(), actual.size()), actual.size());
  EXPECT_EQ(std::string(actual.begin(), actual.end()), "udp-in");
  receiver.stop();
}

}  // namespace
