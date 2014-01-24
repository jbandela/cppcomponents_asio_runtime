#include "../../../cppcomponents_asio_runtime/cppcomponents_asio_runtime.hpp"
#include <iostream>
#include <cppcomponents_async_coroutine_wrapper/cppcomponents_resumable_await.hpp>
#include <sstream>
#include <cppcomponents/loop_executor.hpp>

#include<gtest/gtest.h>


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


  void main_async(int i, cppcomponents::awaiter await){

    using namespace cppcomponents;
    using namespace asio_runtime;


    Runtime::GetThreadPool();
    Runtime::GetBlockingThreadPool();
    


    auto eps = await(Tcp::Query("www.google.com", "https", ISocket::Passive | ISocket::AddressConfigured));
    std::vector<std::string> ipstrings;
    for (auto& e : eps){
      ipstrings.push_back(e.address_.ToString());
    }
    auto start = std::chrono::steady_clock::now();
    auto f = await.as_future(Timer::WaitFor(std::chrono::milliseconds{ 50 }));
    auto end = std::chrono::steady_clock::now();
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

  
  void udp_client(cppcomponents::awaiter await) {


    using namespace cppcomponents::asio_runtime;
    Udp u;
    u.OpenRaw(4);
    await(u.ConnectQueryRaw(cppcomponents::cr_string("127.0.0.1"), "7000", 0));
    await(u.SendRaw("World", 0));
    auto buf = await(u.ReceiveBufferRaw(0));
    std::string s(buf.Begin(), buf.End());
    EXPECT_EQ("Hello World", s);


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
    EXPECT_EQ("Hello World", s);

  }

  TEST(udp, udp){

	auto e = cppcomponents::asio_runtime::Runtime::GetThreadPool();
    auto server_future = cppcomponents::async(e,cppcomponents::resumable(udp_server));
    auto client_future = cppcomponents::async(e,cppcomponents::resumable(udp_client));

    while (!(server_future.Ready() && client_future.Ready())){
      std::this_thread::yield();
    }

  }

  void timer_test(cppcomponents::awaiter await){

    using cppcomponents::asio_runtime::Timer;
    auto start = std::chrono::steady_clock::now();
    auto f = await.as_future(Timer::WaitFor(std::chrono::milliseconds{ 50 }));
    auto end = std::chrono::steady_clock::now();
    EXPECT_EQ(0, f.ErrorCode());
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LE(ms, 70);
    EXPECT_GE(ms, 30);

  }

  TEST(timer, timer){
	auto e = cppcomponents::asio_runtime::Runtime::GetThreadPool();

    auto f = cppcomponents::async(e,cppcomponents::resumable(timer_test));
    while (!f.Ready()){
      std::this_thread::yield();
    }
  }


  TEST(longrunningexecutor, longrunningexecutor){
	  using namespace cppcomponents;
	  using namespace asio_runtime;

	  auto e = Runtime::GetThreadPool();
	  auto lr = Runtime::GetLongRunningExecutor();
	  std::vector<Future<void>> v;
	  Future<void> sf;
	  std::atomic<bool> bfirst_lr{ false };
	  for (int i = 0; i < 100; i++){
		  v.push_back(cppcomponents::async(lr, [&bfirst_lr](){std::this_thread::sleep_for(std::chrono::milliseconds{ 100 }); bfirst_lr.store(true); }));
	  }
	  sf = cppcomponents::async(e, [](){ });
	  auto f = cppcomponents::when_all(v);

	  // wait until the short task finishes
	  while (!sf.Ready()){
		  std::this_thread::yield();
	  }

	  // Our short task should have finished before the first long running task
	  EXPECT_FALSE(bfirst_lr.load());

	  // Wait for long running to finish
	  while (!f.Ready()){
		  std::this_thread::yield();
	  }

  } 
  TEST(blockingthreadpool, blockingthreadpool){
	  using namespace cppcomponents;
	  using namespace asio_runtime;

	  cppcomponents::LoopExecutor loop;

	  auto e = Runtime::GetBlockingThreadPool();
	  std::vector<Future<void>> v;
	  Future<void> sf;
	  auto tc = e.GetThreadCount();
	  auto start = std::chrono::steady_clock::now();
	  for (int i = 0; i < tc; i++){
		  v.push_back(cppcomponents::async(e, [](){std::this_thread::sleep_for(std::chrono::milliseconds{ 1000 });  }));
	  }
	  auto f = cppcomponents::when_all(v);

	  f.Then([&loop](Future<void> f){loop.MakeLoopExit(); });

	  loop.Loop(); 

	  auto stop = std::chrono::steady_clock::now();

	  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	  EXPECT_LT(duration.count(), 2 * 1000);

	   



  } 