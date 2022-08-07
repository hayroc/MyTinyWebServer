
#include <iostream>
#include <functional>
#include <string>
#include <unistd.h>

#include "../thread/threadpool.h"

using namespace hayroc;

int main() {
  ThreadPool eventloop;
  for(int i = 0; i < 100; i++) {
    eventloop.addJod([i]() {
      std::string str = std::to_string(i) + "test lambda";
      std::cout << str << std::endl;
      sleep(2);
      std::cout << str << std::endl;
    });
  }

  std::function<void(int)> func = [](int i) {
    std::string str = std::to_string(i) + "test function";
    std::cout << str << std::endl;
    sleep(2);
    std::cout << str << std::endl;
  };
  for(int i = 0; i < 100; i++) {
    eventloop.addJod(std::bind(func, i));
  }
  while(true) {}
}
