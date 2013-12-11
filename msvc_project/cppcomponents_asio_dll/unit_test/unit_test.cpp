#include "../../../cppcomponents_asio_runtime/cppcomponents_asio_runtime.hpp"
#include <iostream>
#include <cppcomponents_async_coroutine_wrapper/cppcomponents_resumable_await.hpp>
#include <sstream>


void print_connection(cppcomponents::use<cppcomponents::asio_runtime::IAsyncStream> is,
  cppcomponents::awaiter await){
  int loop = 0;
  while (true){
  std::string str;
    loop++;
    auto fut = await.as_future(is.ReadBufferUntilString("\r\n\r\n"));
    if (fut.ErrorCode()){
      auto e = fut.ErrorCode();
      break;
    }
    auto buf = fut.Get();
    str.append(buf.Begin(),buf.End());
    std::cout << str << "\n";
    std::stringstream strstream;
    strstream <<
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: Close\r\n"
      "Content-Length: " << str.size() << "\r\n"
      "\r\n"
      << str;;


    strstream << "\r\n\r\n";

    auto s = strstream.str();
    await(is.WriteRaw(cppcomponents::asio_runtime::const_simple_buffer{ &s[0], s.size() }));
  }


  // Generate the http response
  // In this case we are echoing back using http

}

  void udp_client(cppcomponents::awaiter await) {


    using namespace cppcomponents::asio_runtime;
    Udp u;
    u.OpenRaw(4);
    await(u.ConnectQueryRaw(cppcomponents::cr_string("127.0.0.1"), "7000",0));
    await(u.SendRaw("World", 0));
    auto buf = await(u.ReceiveBufferRaw(0));
    std::string s(buf.Begin(), buf.End());
    auto b = (s == std::string("Hello World"));
    //assert(true);

    
  }

  void udp_server(cppcomponents::awaiter await){
    using namespace cppcomponents::asio_runtime;
      Udp u;
      u.OpenRaw(4);
      auto sip = IPAddress::V4Loopback().ToString();
      u.Bind(endpoint(IPAddress::V4FromString("0.0.0.0"), 7000));

      endpoint ep;
      cppcomponents::use<cppcomponents::IBuffer> ib;
      std::tie(ib, ep) = await(u.ReceiveFromBufferRaw(0));
      std::string s(ib.Begin(), ib.End());
      s = "Hello " + s;
      await(u.SendToRaw(const_simple_buffer(&s[0], s.size()), ep, 0));
      auto b = (s == "Hello World");
      assert(s == "Hello World");

  }
  void main_async(int i, cppcomponents::awaiter await){

    using namespace cppcomponents;
    using namespace asio_runtime;


    Runtime::GetThreadPool();
    Runtime::GetBlockingThreadPool();
    

    resumable(udp_server)();
    resumable(udp_client)();

    auto eps = await(Tcp::Query("www.google.com", "https", ISocket::Passive | ISocket::AddressConfigured));
    std::vector<std::string> ipstrings;
    for (auto& e : eps){
      ipstrings.push_back(e.address_.ToString());
    }
    auto start = std::chrono::steady_clock::now();
    auto f = await.as_future(Timer::WaitFor(std::chrono::milliseconds{ 50 }));
    auto end = std::chrono::steady_clock::now();
    std::cout << "Timer finished with error code " << f.ErrorCode() << " after " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "milliseconds \n";
#if 1
    TlsContext context{ TlsConstants::Method::Tlsv1 };
    context.LoadVerifyFile("C:\\Users\\jrb\\Desktop\\cacert.pem");
    context.SetVerifyMode(TlsConstants::VerifyPeer);
    context.EnableRfc2818Verification("www.google.com");
    TlsStream socket{ context.as<ITlsContext>() };
  

  await(socket.LowestLayer().ConnectQueryRaw("www.google.com", "https", ISocket::Passive | ISocket::AddressConfigured));
  await(socket.Handshake(TlsConstants::HandshakeType::Client));
  std::string resource = "/";
  std::string server = "www.google.com";
  std::string request = "GET " + resource + " HTTP/1.0\r\nHost: " + server + "\r\n\r\n";
  await(socket.WriteRaw(const_simple_buffer(&request[0], request.size())));
  while (true){
   
    auto fut = await.as_future(socket.ReadBuffer());
    if (fut.ErrorCode()){ break; }
    auto buf = fut.Get();
    std::string str{ buf.Begin(), buf.End() };
    std::cout << str;
  }
#endif 
  TcpAcceptor acceptor{ endpoint{ IPAddress::V4Loopback(), 7777 } };

  while (true){
    Tcp t;
    auto is = t.as<ISocket>();
    await(acceptor.Accept(is));
    std::cout << "Received connect\n";
    cppcomponents::async(cppcomponents::asio_runtime::Runtime::GetThreadPool(),std::bind(cppcomponents::resumable(print_connection),is.QueryInterface<IAsyncStream>()));
  }
}
#include <signal.h>
int main(){
  int i = 0;
  //std::cout << "Enter wait time in milliseconds\n";
  //std::cin >> i;
  cppcomponents::resumable(main_async)(500).Then([](cppcomponents::Future<void> f){
    std::cout << "Finished with " << f.ErrorCode() << "\n";
  });

  using namespace cppcomponents;
  using namespace cppcomponents::asio_runtime;
  SignalSet s;
  s.Add(SIGINT);
  std::atomic<bool> done{ false };
  s.Wait().Then([&](Future<int> f){
    if (f.ErrorCode()){
      std::cout << "Signal error code " <<  f.ErrorCode() << std::endl;
    }
    else{
      std::cout << "Signal received " << f.Get() << std::endl;
    }
    done.store(true);
  });
  while (!done.load()){
    std::this_thread::yield();
  }
  std::cin >> i;

  //cppcomponents::asio_runtime::Runtime::GetThreadPool().Join();

}