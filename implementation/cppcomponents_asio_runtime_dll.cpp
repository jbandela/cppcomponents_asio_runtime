#include "../cppcomponents_asio_runtime/cppcomponents_asio_runtime.hpp"
#define ASIO_STANDALONE 1
#include "external_dependencies/asio.hpp"
#include <cppcomponents/implementation/spinlock.hpp>

using namespace cppcomponents;
using namespace cppcomponents::asio_runtime;

struct IGetImplementation :define_interface<cppcomponents::uuid<0xd73b5876, 0x0d97, 0x4879, 0xb97e, 0x6dc43c258217>>
{
  void* GetImplementationRaw();
  const void* GetImplementationConstRaw();

  CPPCOMPONENTS_CONSTRUCT(IGetImplementation, GetImplementationRaw, GetImplementationConstRaw);

  CPPCOMPONENTS_INTERFACE_EXTRAS(IGetImplementation){
    template<class T>
    T& GetImplementation(){
      return *static_cast<T*>(this->get_interface().GetImplementationRaw());
    }
    template<class T>
    const T& GetImplementation()const{
      return *static_cast<const T*>(this->get_interface().GetImplementationConstRaw());
    }

  };
};

template<class T,class Interface>
T& get_implementation(use<Interface> i){
  return i.template QueryInterface<IGetImplementation>().template GetImplementation<T>();
}



typedef runtime_class<RuntimeId, factory_interface<NoConstructorFactoryInterface>,object_interfaces<IGetImplementation,IThreadPool>, static_interfaces<IRuntimeStatics> > RuntimeImp_t;



struct io_service_runner{
  std::atomic<bool> should_stop_ = { false };
  std::atomic<bool> stop_if_no_work_ = { false };
  asio::io_service* io_;
  std::thread t_;

  struct coarse_runner{
    static bool run(asio::io_service* io){
      return (0 != io->poll());
    }
  };
  struct fine_runner{
    static bool run(asio::io_service* io){
      return (0 != io->poll_one());
    }
  };
  template<class Runner>
  void thread_func(){
    while (should_stop_.load() == false){
      if (!Runner::run(io_)){
        if (stop_if_no_work_.load() == true){
          return;
        }
        else{
          std::this_thread::yield();
        }
      }
    }
  
  }

  std::thread get_thread(bool coarse){
    if (coarse){
      return std::thread{ std::bind(&io_service_runner::thread_func<coarse_runner>, this) };
    }
    else{
      return std::thread{ std::bind(&io_service_runner::thread_func<fine_runner>, this) };

    }
  }

  io_service_runner(asio::io_service* io,bool coarse = true)
    :io_{ io },t_{ get_thread(coarse) }
  {}

  void stop_when_no_work(){
    stop_if_no_work_.store(true);
  }

  void join_when_no_work(){
    stop_when_no_work();
    if(t_.joinable()) t_.join();
  }

    ~io_service_runner(){
    if (io_ == nullptr) return;
    should_stop_.store(true);

    // Wait for the thread to stop
    if(t_.joinable()) t_.join();
  }


};

struct ImplementRuntime :implement_runtime_class<ImplementRuntime, RuntimeImp_t>{
  std::atomic<bool> lock_ = { false };
  asio::io_service io_service_;
  std::vector<std::unique_ptr<io_service_runner>> threads_;
  std::int32_t min_threads_;
  std::int32_t max_threads_;
  std::int32_t initial_threads_;
  typedef cppcomponents::delegate < void() > ClosureType;

  asio::io_service& io(){ return io_service_; }

  void IExecutor_AddDelegate(use<ClosureType> f){
    io_service_.post(f);
  }
  std::size_t IExecutor_NumPendingClosures(){

    return 65536;
  }

  bool IThreadPool_AddThread(){
    spin_locker l{ lock_ };
    if (threads_.size() < max_threads_){
      if (threads_.size() < initial_threads_){
        std::unique_ptr<io_service_runner> ptr{ new io_service_runner{ &io_service_, true } };
        threads_.push_back(std::move(ptr));
      }
      else{
        std::unique_ptr<io_service_runner> ptr{ new io_service_runner{ &io_service_, false } };
        threads_.push_back(std::move(ptr));

      }
      return true;
    }
    else{
      return false;
    }
  }
  bool IThreadPool_RemoveThread(){
    spin_locker l{ lock_ };
    if (threads_.size() > min_threads_){
      threads_.pop_back();
      return true;
    }
    else{
      return false;
    }
  }

  void IThreadPool_Join(){
    spin_locker l{ lock_ };
    
    // Wait for tasks to finish
    for (auto& t : threads_){
      t->join_when_no_work();
    }

    threads_.clear();

    assert(io_service_.stopped());
  }

  ImplementRuntime(std::int32_t num_threads = -1, std::int32_t min_threads = 2, std::int32_t max_threads = 100)
    :min_threads_{ min_threads }, max_threads_{max_threads}
  {
    if (num_threads == -1){
      num_threads = std::thread::hardware_concurrency();
      if (num_threads == 0) num_threads = min_threads;
    }
    if (num_threads < min_threads_){ min_threads_ = num_threads; }
    if (num_threads > max_threads_){ max_threads_ = num_threads; }
    initial_threads_ = num_threads;
    for (int i = 0; i < num_threads; ++i){
      std::unique_ptr<io_service_runner> ptr{ new io_service_runner{&io_service_, true } };
      threads_.push_back(std::move(ptr));
    }
  }

  ~ImplementRuntime(){
    IThreadPool_Join();
  }

  struct TPInitializer{
    use<IThreadPool> pool_;
    TPInitializer(std::int32_t num_threads, std::int32_t min_threads, std::int32_t max_threads)
    {
      pool_ = ImplementRuntime::create(num_threads, min_threads, max_threads).QueryInterface<IThreadPool>();
    }

  };

  static   use<IThreadPool> GetThreadPoolRaw(std::int32_t num_threads, std::int32_t min_threads, std::int32_t max_threads){
    return cross_compiler_interface::detail::safe_static_init<TPInitializer, ImplementRuntime>::get(num_threads, min_threads, max_threads).pool_;
  }



  void* GetImplementationRaw(){ return this; }
  const void* GetImplementationConstRaw(){ return this; }
};

CPPCOMPONENTS_REGISTER(ImplementRuntime)


asio::io_service& get_io(){
  return get_implementation<ImplementRuntime>(Runtime::GetThreadPool()).io();

}


struct ImplementStrand :implement_runtime_class<ImplementStrand, Strand_t>
{
  typedef cppcomponents::delegate < void() > ClosureType;

  asio::io_service::strand strand_;

  ImplementStrand() :strand_{ get_io() }{}

  void AddDelegate(use<ClosureType> f){
    strand_.post(f);
  }
  std::size_t NumPendingClosures(){
    return 65536;
  }
};

CPPCOMPONENTS_REGISTER(ImplementStrand)

struct ImplementTimer :implement_runtime_class<ImplementTimer, Timer_t>
{
  asio::basic_waitable_timer<std::chrono::steady_clock> timer_;

  ImplementTimer() :timer_{ get_io() }{}

  Future<void> Wait(){
    auto p = make_promise<void>();
    timer_.async_wait([p](asio::error_code ec)mutable{
      if (ec){
        auto val = ec.value();
        if (val > 0) val = -val;
        if (val == 0){
          val = error_fail::ec;
        }
        p.SetError(val);
      }
      else{
        p.Set();
      }
    });

    return p.QueryInterface<IFuture<void>>();
  }
  void Cancel(){ timer_.cancel(); }
  void CancelOne(){ timer_.cancel_one(); }
  void ExpiresFromNowRaw(std::uint64_t microseconds){
    timer_.expires_from_now(std::chrono::microseconds{ microseconds });
  }

  static Future<void> WaitForRaw(std::uint64_t microseconds){
    Timer t;
    t.ExpiresFromNowRaw(microseconds);
    auto it = t.as<InterfaceUnknown>();
    return t.Wait().Then([it](Future<void> f)mutable{
      it = nullptr;
      f.Get();
    });
  }


    


};


CPPCOMPONENTS_REGISTER(ImplementTimer);

struct ImplementIPAddress :implement_runtime_class<ImplementIPAddress, IPAddress_t>
{
  asio::ip::address address_;
  bool IsV4(){ return address_.is_v4(); }
  bool IsV6(){ return address_.is_v6(); }
  std::string ToString(){ return address_.to_string(); }
  void ToBytes(simple_buffer bytes){
    if (IsV4()){

      auto v4 = address_.to_v4();
      auto b = v4.to_bytes();
      if (bytes.size() != b.size()){
        throw error_invalid_arg();
      }
      std::copy(b.begin(), b.end(), bytes.begin());
    }
    else if (IsV6()){
      auto v6 = address_.to_v6();
      auto b = v6.to_bytes();
      if (bytes.size() != b.size()){
        throw error_invalid_arg();
      }
      std::copy(b.begin(), b.end(), bytes.begin());
    }
    else{
      throw error_invalid_arg();
    }
  }

  ImplementIPAddress(){}
  ImplementIPAddress(asio::ip::address a) :address_{std::move(a)}{}

  static use<IIPAddress> V4FromString(cr_string str){
    return ImplementIPAddress::create(asio::ip::address_v4::from_string(std::string(str.begin(), str.end()))).QueryInterface<IIPAddress>();
  }
  static use<IIPAddress> V4FromBytes(simple_buffer bytes){
    asio::ip::address_v4::bytes_type b;
    if (bytes.size() != b.size()){
      throw error_invalid_arg();
    }
    std::copy(bytes.begin(), bytes.end(), b.begin());
    return ImplementIPAddress::create(asio::ip::address_v4(b)).QueryInterface<IIPAddress>();
  }
  static use<IIPAddress> V4Broadcast()   {
    return ImplementIPAddress::create(asio::ip::address_v4::broadcast()).QueryInterface<IIPAddress>();
  }

  static use<IIPAddress> V4Loopback() {
    return ImplementIPAddress::create(asio::ip::address_v4::loopback()).QueryInterface<IIPAddress>();
  }
  static use<IIPAddress> V4Any(){
    return ImplementIPAddress::create(asio::ip::address_v4::any()).QueryInterface<IIPAddress>();

  }

  static use<IIPAddress> V6FromString(cr_string str){
    return ImplementIPAddress::create(asio::ip::address_v6::from_string(std::string(str.begin(), str.end()))).QueryInterface<IIPAddress>();
  }
  static use<IIPAddress> V6FromBytes(simple_buffer bytes){
    asio::ip::address_v6::bytes_type b;
    if (bytes.size() != b.size()){
      throw error_invalid_arg();
    }
    std::copy(bytes.begin(), bytes.end(), b.begin());
    return ImplementIPAddress::create(asio::ip::address_v6(b)).QueryInterface<IIPAddress>();
  }
  static use<IIPAddress> V6Loopback(){
    return ImplementIPAddress::create(asio::ip::address_v6::loopback()).QueryInterface<IIPAddress>();
  }
  static use<IIPAddress> V6Any(){
    return ImplementIPAddress::create(asio::ip::address_v6::any()).QueryInterface<IIPAddress>();

  }
  static use<IIPAddress> V6V4Compatible(use<IIPAddress> v4)
  {
    if (!v4.IsV4()){
      throw error_invalid_arg();
    }
    asio::ip::address_v4::bytes_type b;
    simple_buffer buf{ b };
    auto v6 = asio::ip::address_v6::v4_compatible(asio::ip::address_v4(b));
    return ImplementIPAddress::create(v6).QueryInterface<IIPAddress>();
  }




};

asio::ip::address get_address(use < IIPAddress> addr){
  if (addr.IsV4()){
    asio::ip::address_v4::bytes_type b;
    addr.ToBytes(simple_buffer{ b });
    return asio::ip::address_v4{ b };
  }
  else{
    asio::ip::address_v6::bytes_type b;
    addr.ToBytes(simple_buffer{ b });
    return asio::ip::address_v6{ b };

  }
}


CPPCOMPONENTS_REGISTER(ImplementIPAddress)
template<class Type = void>
struct settable_option{
  int level_;
  int name_;
  Type* data_;
  std::size_t size_;

  template<class T>
  int level(const T&)const{
    return level_;
  }
  template<class T>
  int name(const T&)const{
    return name_;
  }
  template<class T>
  Type* data(const T&)const{
    return data_;
  }
  template<class T>
  std::size_t size(const T&)const{
    return size_;
  }

  template<class T>
  void resize(const T&, std::size_t){
    throw error_invalid_arg{};
  }
};


typedef runtime_class<TcpId, object_interfaces<ISocket, IAsyncStream, IGetImplementation>, static_interfaces<IQueryStatic>> Tcp_t1;

template<class Derived, class Socket, class UdpOrTcp> struct ImplementSocketHelper{
  typedef typename  UdpOrTcp::resolver::iterator RIter;

  Derived* derived(){ return static_cast<Derived*>(this); }

  Socket& socket(){ return derived()->socket_; }


  void AssignRaw(std::int32_t ip_type, std::uint64_t s){
    if (ip_type == 4){
      socket().assign(UdpOrTcp::v4(), s);
    }
    else if (ip_type = 6){
      socket().assign(UdpOrTcp::v6(), s);

    }

    throw error_invalid_arg();
  }

  Future<void> Connect(endpoint e){
    auto p = make_promise<void>();
    typename UdpOrTcp::endpoint ep(get_address(e.address_), e.port_);
    socket().async_connect(ep, [p](const asio::error_code& error)
    {
      if (!error)
      {
        p.Set();
      }
      else{
        p.SetError(error.value());
      }
    });

    return p.QueryInterface<IFuture<void>>();
  }

  static  Future<std::vector<endpoint>> Query(cr_string host, cr_string service, std::uint32_t flags){
    typename UdpOrTcp::resolver::query::flags f{};
    if (flags & ISocket::AddressConfigured){
      f &= UdpOrTcp::resolver::query::flags::address_configured;
    }
    if (flags & ISocket::AllMatching){
      f &= UdpOrTcp::resolver::query::flags::all_matching;
    }
    if (flags & ISocket::CanonicalName){
      f &= UdpOrTcp::resolver::query::flags::canonical_name;
    }
    if (flags & ISocket::NumericHost){
      f &= UdpOrTcp::resolver::query::flags::numeric_host;
    }
    if (flags & ISocket::NumericService){
      f &= UdpOrTcp::resolver::query::flags::numeric_service;
    }
    if (flags & ISocket::Passive){
      f &= UdpOrTcp::resolver::query::flags::passive;
    }
    if (flags & ISocket::V4Mapped){
      f &= UdpOrTcp::resolver::query::flags::v4_mapped;
    }
    auto sq = std::make_shared<typename UdpOrTcp::resolver::query>(host.to_string(), service.to_string(), f);
    auto sr = std::make_shared<typename UdpOrTcp::resolver>(get_io());

    auto p = make_promise<std::vector<endpoint>>();

    sr->async_resolve(*sq, [p, sq, sr](
      const asio::error_code& error, typename UdpOrTcp::resolver::iterator iterator)mutable {
      if (error){
        p.SetError(error.value());
      }
      else{
        std::vector<endpoint> endpoints;
        for (auto iter = iterator; iter != RIter{}; ++iter){
          auto ipaddr = ImplementIPAddress::create(iter->endpoint().address()).template QueryInterface<IIPAddress>();
          endpoints.push_back(endpoint(ipaddr,iter->endpoint().port()));
        }
        p.Set(endpoints);
      }
    });
    return p.QueryInterface<IFuture<std::vector<endpoint>>>();
  }

  Future<void> ConnectQueryRaw(cr_string host, cr_string service, std::uint32_t flags){
    typename UdpOrTcp::resolver::query::flags f{};
    if (flags & ISocket::AddressConfigured){
      f &= UdpOrTcp::resolver::query::flags::address_configured;
    }
    if (flags & ISocket::AllMatching){
      f &= UdpOrTcp::resolver::query::flags::all_matching;
    }
    if (flags & ISocket::CanonicalName){
      f &= UdpOrTcp::resolver::query::flags::canonical_name;
    }
    if (flags & ISocket::NumericHost){
      f &= UdpOrTcp::resolver::query::flags::numeric_host;
    }
    if (flags & ISocket::NumericService){
      f &= UdpOrTcp::resolver::query::flags::numeric_service;
    }
    if (flags & ISocket::Passive){
      f &= UdpOrTcp::resolver::query::flags::passive;
    }
    if (flags & ISocket::V4Mapped){
      f &= UdpOrTcp::resolver::query::flags::v4_mapped;
    }
    auto sq = std::make_shared<typename UdpOrTcp::resolver::query>(host.to_string(), service.to_string(), f);
    auto sr = std::make_shared<typename UdpOrTcp::resolver>(get_io());

    auto p = make_promise<void>();
    Socket* ps = &socket();
    auto iunk = derived()->template QueryInterface<InterfaceUnknown>();
    sr->async_resolve(*sq, [p, ps, iunk, sq, sr](
      const asio::error_code& error, typename UdpOrTcp::resolver::iterator iterator)mutable {
      if (error){
        auto msg = error.message();
        p.SetError(error.value());
      }
      else{
        asio::async_connect(*ps, iterator, [p, sq, sr](const asio::error_code& error, RIter iterator)
        {
          if (error)
          {
            auto msg = error.message();
            p.SetError(error.value());
          }
          else{
            p.Set();
          }
        });
      }
      iunk = nullptr;
    });
    return p.QueryInterface<IFuture<void>>();


  }
  bool AtMark(){
    return socket().at_mark();
  }
  std::size_t Available(){
    return socket().available();
  }
  void Bind(endpoint e){
    try{
      return socket().bind(typename UdpOrTcp::endpoint(get_address(e.address_), e.port_));
    }
    catch (asio::system_error& e){
      auto msg = e.what();
    }
  }
  void Cancel(){
    socket().cancel();
  }
  void Close(){
    socket().close();
  }
  void GetOptionRaw(int level, int option_name, void* option_value, std::size_t option_len){
    settable_option<> op;
    op.level_ = level;
    op.name_ = option_name;
    op.data_ = option_value;
    op.size_ = option_len;

    socket().get_option(op);


  }
  void SetOptionRaw(int level, int option_name, const void* option_value, std::size_t option_len){
    settable_option<const void> op;
    op.level_ = level;
    op.name_ = option_name;
    op.data_ = option_value;
    op.size_ = option_len;

    socket().set_option(op);
  }
  bool IsOpen(){
    return socket().is_open();
  }
  int NativeHandle(){
    return socket().native_handle();
  }
  bool GetNonBlocking(){
    return socket().non_blocking();
  }
  void SetNonBlocking(bool mode){
    socket().non_blocking(mode);
  }
  bool GetNativeNonBlocking(){
    return socket().native_non_blocking();
  }
  void SetNativeNonBlocking(bool mode){
    socket().native_non_blocking(mode);
  }
  void OpenRaw(std::int32_t ip_type){
    if (ip_type == 4){
      socket().open(UdpOrTcp::v4());
    }
    else if (ip_type == 6){
      socket().open(UdpOrTcp::v6());
    }
    else{
      throw error_invalid_arg{};
    }
  }

  endpoint RemoteEndpoint(){
    auto ep = socket().remote_endpoint();
    return endpoint{ImplementIPAddress::create(ep.address()).template QueryInterface<IIPAddress>(), ep.port() };
  }

  void Shutdown(std::int32_t type){
    asio::socket_base::shutdown_type s;
    if (type == asio::socket_base::shutdown_both){
      s = asio::socket_base::shutdown_both;
    }
    else if (type == asio::socket_base::shutdown_receive){
      s = asio::socket_base::shutdown_receive;
    }
    else if (type == asio::socket_base::shutdown_send){
      s = asio::socket_base::shutdown_send;
    }
    else{
      throw error_invalid_arg{};
    }
    socket().shutdown(s);
  }


};

template<class Derived, class Socket>
struct ImplementAsyncStreamHelper{
  Derived* derived(){ return static_cast<Derived*>(this); }

  Socket& socket(){ return derived()->socket_; }

  static void process_read(Promise<std::size_t> p,
  const asio::error_code& ec, std::size_t bytes_transferred){
    try{
      if (ec){
        p.SetError(ec.value());
      }
      else{
        p.Set(bytes_transferred);
      }
    }
    catch (std::exception& e){
      p.SetError(error_mapper::error_code_from_exception(e));
    }
  }

  Future<void> IAsyncStream_Poll(){
    auto p = make_promise<void>();
    socket().async_read_some(asio::null_buffers{},
      [p](const asio::error_code& ec, std::size_t bytes_transferred){
      try{
        if (ec && !(ec == asio::error::eof && bytes_transferred)){
          p.SetError(ec.value());
        }
        else{
          p.Set();
        }
      }
      catch (std::exception& e){
        p.SetError(error_mapper::error_code_from_exception(e));
      }
    });
    return p.QueryInterface<IFuture<void>>();

  }

  Future<std::size_t> IAsyncStream_Read(simple_buffer buf){
    auto p = make_promise<std::size_t>();
    socket().async_read_some(asio::buffer(buf.begin(), buf.size()),
      std::bind(&process_read, p, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<std::size_t>>();
  }
  Future<std::size_t> IAsyncStream_ReadAt(std::uint64_t offset, simple_buffer buf){
    return IAsyncStream_Read(buf);
  }

  static void process_streambuf_read(Promise<use<IBuffer>> p, std::shared_ptr<asio::streambuf> spbuf,
    const asio::error_code& ec, std::size_t bytes_transferred){
    if (ec){
      auto msg = ec.message();
      p.SetError(ec.value());
    }
    else{
      try{
        auto sz = asio::buffer_size(spbuf->data());
        auto ib = Buffer::Create(sz);
        ib.SetSize(sz);
        asio::buffer_copy(asio::buffer(ib.Begin(), sz), spbuf->data());
        p.Set(ib);
      }
      catch (std::exception& e){
        p.SetError(error_mapper::error_code_from_exception(e));
      }
    }
  }

  Future<use<IBuffer>> IAsyncStream_ReadBuffer(){
    auto p = make_promise<use<IBuffer>>();
    std::shared_ptr<asio::streambuf> spbuf = std::make_shared<asio::streambuf>();

    asio::async_read(socket(), *spbuf, asio::transfer_at_least(1),
      std::bind(&process_streambuf_read, p, spbuf, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<use<IBuffer>>>();
  }
  Future<use<IBuffer>> IAsyncStream_ReadBufferAt(std::uint64_t offset){
    return IAsyncStream_ReadBuffer();
  }
  Future<use<IBuffer>> IAsyncStream_ReadBufferUntilChar(char c){
    auto p = make_promise<use<IBuffer>>();
    std::shared_ptr<asio::streambuf> spbuf = std::make_shared<asio::streambuf>();

    asio::async_read_until(socket(), *spbuf, c, std::bind(&process_streambuf_read, p, spbuf, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<use<IBuffer>>>();

  }
  Future<use<IBuffer>> IAsyncStream_ReadBufferUntilString(cr_string delim){
    auto p = make_promise<use<IBuffer>>();
    std::shared_ptr<asio::streambuf> spbuf = std::make_shared<asio::streambuf>();

    asio::async_read_until(socket(), *spbuf, delim.to_string(), std::bind(&process_streambuf_read, p, spbuf, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<use<IBuffer>>>();

  }

  static void process_write(Promise<std::size_t> p, const asio::error_code& error, std::size_t bytes_written){
    try{
      if (error){
        p.SetError(error.value());
      }
      else{
        p.Set(bytes_written);
      }
    }
    catch (std::exception& e){
      p.SetError(error_mapper::error_code_from_exception(e));
    }
  }

  Future<std::size_t> IAsyncStream_Write(const_simple_buffer data){
    using namespace std::placeholders;
    auto p = make_promise<std::size_t>();
    asio::async_write(socket(), asio::buffer(data.begin(), data.size()), std::bind(&process_write, p, _1, _2));
    return p.QueryInterface<IFuture<std::size_t>>();
  }
  Future<std::size_t> IAsyncStream_WriteAt(std::uint64_t offset, const_simple_buffer data){
    return IAsyncStream_Write(data);
  }
};
// NB: IGetImplementation returns socket
struct ImplementTcp :implement_runtime_class<ImplementTcp, Tcp_t1>, 
  ImplementSocketHelper<ImplementTcp,asio::ip::tcp::socket,asio::ip::tcp>,
  ImplementAsyncStreamHelper<ImplementTcp,asio::ip::tcp::socket>{
  asio::ip::tcp::socket socket_;




  ImplementTcp() :socket_{ get_io() }{}
  void* IGetImplementation_GetImplementationRaw(){
    return &socket_;
  }
  const void* IGetImplementation_GetImplementationConstRaw(){
    return &socket_;
  }




};

CPPCOMPONENTS_REGISTER(ImplementTcp)

asio::ip::tcp::socket& get_tcp_socket(use<InterfaceUnknown> i){
  return i.QueryInterface<IGetImplementation>().GetImplementation<asio::ip::tcp::socket>();
}

struct ImplementTcpAcceptor :implement_runtime_class<ImplementTcpAcceptor, TcpAcceptor_t>
{
  asio::ip::tcp::acceptor acceptor_;
  Future<void> Accept(use<ISocket> i){
    auto p = make_promise<void>();
    auto& s = get_tcp_socket(i);
    acceptor_.async_accept(s, [i, p](const asio::error_code& error){
      try{
        if (error){
          p.SetError(error.value());
        }
        else{
          p.Set();
        }

      }
      catch (std::exception& e){
        p.SetError(error_mapper::error_code_from_exception(e));
      }
    });
    return p.QueryInterface<IFuture<void>>();
  }


  ImplementTcpAcceptor(endpoint e, bool reuse_addr) 
    :acceptor_{ get_io(), asio::ip::tcp::endpoint(get_address(e.address_), e.port_), reuse_addr }
  {}

};

CPPCOMPONENTS_REGISTER(ImplementTcpAcceptor)

struct ImplementUdp:implement_runtime_class<ImplementUdp,Udp_t>,
  ImplementSocketHelper<ImplementUdp,asio::ip::udp::socket,asio::ip::udp>
{
  asio::ip::udp::socket socket_;

  ImplementUdp() :socket_{ get_io() }{}
  static void process_receive(Promise<std::size_t> p,
    const asio::error_code& ec, std::size_t bytes_transferred){
    try{
      if (ec){
        p.SetError(ec.value());
      }
      else{
        p.Set(bytes_transferred);
      }
    }
    catch (std::exception& e){
      p.SetError(error_mapper::error_code_from_exception(e));
    }
  } 

  static void process_receive_from(Promise<std::pair<std::size_t, endpoint>> p, std::shared_ptr<asio::ip::udp::endpoint> uep,
    const asio::error_code& ec, std::size_t bytes_transferred){
    try{
      if (ec){
        p.SetError(ec.value());
      }
      else{
        auto address = ImplementIPAddress::create(uep->address()).QueryInterface<IIPAddress>();
        endpoint ep(address, uep->port());
        p.Set(std::make_pair(bytes_transferred,ep));
      }
    }
    catch (std::exception& e){
      p.SetError(error_mapper::error_code_from_exception(e));
    }
  }



  Future<std::size_t> IAsyncDatagram_ReceiveRaw(simple_buffer buf, std::uint32_t flags){
    auto p = make_promise<std::size_t>();
    socket().async_receive(asio::buffer(buf.begin(), buf.size()),flags,
      std::bind(&process_receive, p, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<std::size_t>>();
  }


  Future<std::pair<std::size_t, endpoint>> IAsyncDatagram_ReceiveFromRaw(simple_buffer buf, std::uint32_t flags){
    auto p = make_promise<std::pair<std::size_t, endpoint>>();
    auto uep = std::make_shared<asio::ip::udp::endpoint>();
    socket().async_receive_from(asio::buffer(buf.begin(), buf.size()),*uep , flags,
      std::bind(&process_receive_from, p,uep, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<std::pair<std::size_t, endpoint>>>();
  }

  static void process_buffer_receive(Promise<use<IBuffer>> p, use<IBuffer> ib,
    const asio::error_code& ec, std::size_t bytes_transferred){
    if (ec){
      auto msg = ec.message();
      p.SetError(ec.value());
    }
    else{
      try{
        ib.SetSize(bytes_transferred);
        p.Set(ib);
      }
      catch (std::exception& e){
        p.SetError(error_mapper::error_code_from_exception(e));
      }
    }
  }
  static void process_buffer_receive_from(Promise<std::pair<use<IBuffer>, endpoint>> p, use<IBuffer> ib, std::shared_ptr<asio::ip::udp::endpoint> uep,
    const asio::error_code& ec, std::size_t bytes_transferred){
    if (ec){
      auto msg = ec.message();
      p.SetError(ec.value());
    }
    else{
      try{
        ib.SetSize(bytes_transferred);
        auto address = ImplementIPAddress::create(uep->address()).QueryInterface<IIPAddress>();
        endpoint ep(address, uep->port());
        p.Set(std::make_pair(ib,ep));
      }
      catch (std::exception& e){
        p.SetError(error_mapper::error_code_from_exception(e));
      }
    }
  }

  static const int max_udp_size = 65507;

  Future<use<IBuffer>> IAsyncDatagram_ReceiveBufferRaw(std::uint32_t flags){
    auto p = make_promise<use<IBuffer>>();
    auto ib = Buffer::Create(max_udp_size);
    

    socket().async_receive(asio::buffer(ib.Begin(), ib.Size()), flags,
      std::bind(&process_buffer_receive, p, ib, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<use<IBuffer>>>();
  }

  Future<std::pair<use<IBuffer>, endpoint>> IAsyncDatagram_ReceiveFromBufferRaw(std::uint32_t flags){
    auto p = make_promise<std::pair<use<IBuffer>, endpoint>>();
    auto ib = Buffer::Create(max_udp_size);
    auto uep = std::make_shared<asio::ip::udp::endpoint>();

    socket().async_receive_from(asio::buffer(ib.Begin(), ib.Size()),*uep, flags,
      std::bind(&process_buffer_receive_from, p, ib,uep, std::placeholders::_1, std::placeholders::_2));
    return p.QueryInterface<IFuture<std::pair<use<IBuffer>, endpoint>>>();
  }


  static void process_send(Promise<std::size_t> p, const asio::error_code& error, std::size_t bytes_written){
    try{
      if (error){
        auto msg = error.message();
        p.SetError(error.value());
      }
      else{
        p.Set(bytes_written);
      }
    }
    catch (std::exception& e){
      p.SetError(error_mapper::error_code_from_exception(e));
    }
  }



  Future<std::size_t> IAsyncDatagram_SendRaw(const_simple_buffer data, std::uint32_t flags){
    using namespace std::placeholders;
    auto p = make_promise<std::size_t>();
    socket().async_send(asio::buffer(data.begin(), data.size()), flags,std::bind(&process_send, p, _1, _2));
    return p.QueryInterface<IFuture<std::size_t>>();
  }


  Future<std::size_t> IAsyncDatagram_SendToRaw(const_simple_buffer data, endpoint receiver, std::uint32_t flags){
    using namespace std::placeholders;
    auto p = make_promise<std::size_t>();
    socket().async_send_to(asio::buffer(data.begin(), data.size()), asio::ip::udp::endpoint(get_address(receiver.address_), receiver.port_),
      flags, std::bind(&process_send, p, _1, _2));
    return p.QueryInterface<IFuture<std::size_t>>();


  }

};

CPPCOMPONENTS_REGISTER(ImplementUdp)

CPPCOMPONENTS_DEFINE_FACTORY()

