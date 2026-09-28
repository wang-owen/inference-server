#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <iostream>
#include <string>

int main() {
  static constexpr std::string kBindAddress = "127.0.0.1";
  static constexpr std::uint16_t kPort = 9091;

  int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    std::cerr << "Failed to create socket.\n";
    return -1;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(kPort);
  if (::inet_pton(AF_INET, kBindAddress.c_str(), &addr.sin_addr) != 1) {
    std::cerr << "Error converting IPv4 address.\n";
    return -1;
  }

  if (::connect(client_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
      -1) {
    std::cerr << "Connection failed. Is the server running?" << std::endl;
    ::close(client_fd);
    return -1;
  }

  std::string message = "INFER 0 1.0 1.1 1.2 1.3 1.4 1.5 1.55\n";
  ::send(client_fd, message.data(), message.size(), 0);

  std::array<char, 1024> buffer;
  std::string response;
  ssize_t bytes_received;
  while (response.find('\n') == std::string::npos &&
         (bytes_received = ::read(client_fd, buffer.data(), buffer.size())) >
             0) {
    response.append(buffer.data(), static_cast<std::size_t>(bytes_received));
  }

  std::cout << response;

  ::close(client_fd);
}
