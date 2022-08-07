#include <iostream>
#include <thread>
#include <unistd.h>
#include <iostream>

#include "log.h"

using namespace std;

void thread_function(){
  auto log = Singleton<Log>::GetInstance();
  int i=0;
  while(1){
    std::string message("this is message ");
    message+=std::to_string(i++);
    DEBUG(message);
    sleep(1);
  }
}

int main() {
  std::thread thd1(thread_function);
  std::thread thd2(thread_function);
  std::thread thd3(thread_function);

  FATAL("new thread");
  thd1.detach();
  thd2.detach();
  thd3.detach();

  while (true) {
    INFO("hh");
    sleep(1);
  }
}
