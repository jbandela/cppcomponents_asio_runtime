#include "../../../cppcomponents_asio_runtime/cppcomponents_asio_runtime.hpp"
#include <iostream>

int main(){
  using namespace cppcomponents;
  using namespace asio_runtime;
  Runtime::GetThreadPool();
  int i = 0;
  std::cout << "Enter wait time in milliseconds\n";
  std::cin >> i;

  auto start = std::chrono::steady_clock::now();
  Timer::WaitFor(std::chrono::milliseconds{ i }).Then([start](cppcomponents::Future<void> f){
    auto end = std::chrono::steady_clock::now();
    std::cout << "Timer finished with error code " << f.ErrorCode() << "after " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "milliseconds \n";
  });

  std::cin >> i;

}