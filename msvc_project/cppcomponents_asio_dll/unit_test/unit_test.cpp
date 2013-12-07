#include "../../../cppcomponents_asio_runtime/cppcomponents_asio_runtime.hpp"
#include <iostream>
#include <cppcomponents_async_coroutine_wrapper/cppcomponents_resumable_await.hpp>


void print_connection(cppcomponents::use<cppcomponents::asio_runtime::IAsyncStream> is,
  cppcomponents::awaiter await){

  while (true){
    std::vector<char>  vec(100);
    auto fut = await.as_future(is.Read(vec));
    if (fut.ErrorCode()){
      auto e = fut.ErrorCode();
    }
    else{

    }
    auto buf = fut.Get();
    std::string str{vec.begin(),vec.end()};
    std::cout << str;
  }
}

void main_async(int i, cppcomponents::awaiter await){
  using namespace cppcomponents;
  using namespace asio_runtime;
  Runtime::GetThreadPool();
  //auto start = std::chrono::steady_clock::now();
  //auto f = await.as_future(Timer::WaitFor(std::chrono::milliseconds{ i }));
  //auto end = std::chrono::steady_clock::now();
  //std::cout << "Timer finished with error code " << f.ErrorCode() << " after " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "milliseconds \n";

  Tcp socket;
  
  await(socket.ConnectQueryRaw("www.google.com", "http", ISocket::Passive | ISocket::AddressConfigured));
  std::string resource = "/";
  std::string server = "www.google.com";
  std::string request = "GET " + resource + " HTTP/1.0\r\nHost: " + server + "\r\n\r\n";
  await(socket.Write(const_simple_buffer(&request[0], request.size())));
  while (true){
   
    auto fut = await.as_future(socket.ReadBuffer());
    if (fut.ErrorCode()){ break; }
    auto buf = fut.Get();
    std::string str{ buf.Begin(), buf.End() };
    std::cout << str;
  }

  TcpAcceptor acceptor{ endpoint{ IPAddress::V4Loopback(), 7777 } };

  while (true){
    auto is = await(acceptor.Accept());
    cppcomponents::resumable(print_connection)(is);
  }
}

int main(){
  int i = 0;
  //std::cout << "Enter wait time in milliseconds\n";
  //std::cin >> i;
  cppcomponents::resumable(main_async)(500).Then([](cppcomponents::Future<void> f){
    std::cout << "Finished with " << f.ErrorCode() << "\n";
  });

  std::cin >> i;

  cppcomponents::asio_runtime::Runtime::GetThreadPool().Join();

}