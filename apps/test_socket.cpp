#include <cassert>
#include <epollfd.hpp>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <tcp/ip_v4_address.hpp>
#include <thread>
#include <unistd.h>

int main() {
  TinyNet::IPv4Address listenAddr("127.0.0.1", 5555);
  int fd = ::socket(listenAddr.getFamily(),
                    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  assert(fd > 0);

  int optval = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval,
               static_cast<socklen_t>(sizeof(optval)));
  // reuse port
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval,
               static_cast<socklen_t>(sizeof(optval)));

  int ret = ::bind(fd, listenAddr.getSockaddr_in(), sizeof(sockaddr_in));
  assert(ret == 0);

  int epollfd = ::epoll_create1(EPOLL_CLOEXEC);
  epoll_event ev{.events = EPOLLIN, .data = {.fd = fd}};
  ::epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);

  ::listen(fd, SOMAXCONN);

  for (;;) {
    epoll_event events[16];
    int num = ::epoll_wait(epollfd, events, 16, 10000);
    for (int i = 0; i < num; ++i) {
      if (events[i].data.fd == fd) {
        std::cout << "xxxx\n";
        sockaddr_in peeraddr;
        socklen_t len = sizeof(peeraddr);
        int acceptFd = ::accept4(fd, reinterpret_cast<sockaddr *>(&peeraddr),
                                 &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        epoll_event tmp{.events = EPOLLIN /* | EPOLLET */ | EPOLLRDHUP,
                        .data = {.fd = acceptFd}};
        ::epoll_ctl(epollfd, EPOLL_CTL_ADD, acceptFd, &tmp);
      } else {
        // 检查error
        if ((events[i].events & EPOLLIN) && (events[i].events & EPOLLRDHUP)) {
          std::cout << "double triggle\n";
        }

        if (events[i].events & EPOLLRDHUP) {
          std::cout << "EPOLLRDHUP\n";
          close(events[i].data.fd);
        } else if (events[i].events & EPOLLHUP) {
          std::cout << "EPOLLHUP\n";
          close(events[i].data.fd);
        } else if (events[i].events & EPOLLIN) {
          char buf[32];
          int n = read(events[i].data.fd, buf, 32);
          std::cout << "recv " << n << " bytes\n";
          if (n == 0) {
            std::cout << "close\n";
            close(events[i].data.fd);
          }
          shutdown(events[i].data.fd, SHUT_RDWR);
        }
      }
    }
  }
}