#define CATCH_CONFIG_MAIN
#include <FdEvent.hpp>
#include <catch2/catch.hpp>
#include <object_pool.hpp>
#include <reactor.hpp>
#include <slot.hpp>
#include <thread>
#include <threadUnsafeSharedPtr.hpp>
#include <timer.hpp>

class MyClass {
public:
  MyClass(TinyNet::Reactor *r) : m_reactor(r) {
    std::cout << "MyClass\n";
    for (int i = 0; i < 16; ++i) {
      vec.push_back(i);
    }
    // 10s
    m_event = std::make_shared<TinyNet::TimerEvent>(
        10 * 1000, true, [this] { this->callback(); });
    m_reactor->getTimer().addTimerEvent(m_event);
  }
  ~MyClass() { std::cout << "~MyClass\n"; }

  void callback() {
    for (int i = 16; i < 32; ++i) {
      vec.push_back(i);
    }
    std::cout << "vec.size() = " << vec.size() << ", and vec.back() = " << vec.back() << '\n';
  }

private:
  TinyNet::Reactor *m_reactor;
  std::shared_ptr<TinyNet::TimerEvent> m_event;
  std::vector<int> vec;
};

TEST_CASE("Test Reactor", "[Reactor]") {
  TinyNet::Reactor r;
  MyClass t(&r);
  r.loop();
}