#include <cppcomponents/cppcomponents.hpp>
#include <cppcomponents/future.hpp>
#include <cppcomponents/buffer.hpp>
#include <cppcomponents/channel.hpp>

namespace cppcomponents{
  namespace asio_runtime{

 

#pragma pack(push, 1)
    template<class Char = char>
    struct basic_simple_buffer{
      typedef typename std::make_unsigned<Char>::type UChar;
      Char* data_;
      std::size_t len_;

      basic_simple_buffer(std::vector<Char>& v)
        :data_{ v.size() ? &v[0] : nullptr },
        len_{ v.size() }

      {
      }
      template<int sz>
      basic_simple_buffer(Char(&ar)[sz])
        : data_{ &ar[0] },
        len_{ sz }
      {}
      template<int sz>
      basic_simple_buffer(UChar(&ar)[sz])
        : data_{ reinterpret_cast<Char*>(&ar[0]) },
        len_{ sz }
      {}
      template<int sz>
      basic_simple_buffer(std::array<Char, sz>&ar)
        : data_{ &ar[0] },
        len_{ size }
      {}
      template<int sz>
      basic_simple_buffer(std::array<UChar, sz>&ar)
        : data_{ reinterpret_cast<Char*>(&ar[0]) },
        len_{ sz }
      {}

      basic_simple_buffer(Char* d, std::size_t sz) :data_{ d }, len_{ sz }{}
      basic_simple_buffer(UChar* d, std::size_t sz) :data_{ reinterpret_cast<Char*>(d) }, len_{ sz }{}
      basic_simple_buffer() :data_{ nullptr }, len_{ 0 }{}

      Char* begin(){ return data_; }
      Char* end(){ return begin() + len_; }
      const Char* cbegin(){ return data_; }
      const Char* cend(){ return begin() + len_; }
      std::size_t size(){ return len_; }
      Char* data(){ return data_; }



    };
#pragma pack(pop)

    typedef basic_simple_buffer<char> simple_buffer;

    typedef basic_simple_buffer<const char> const_simple_buffer;
  }
}

namespace cross_compiler_interface{

  template<class Char>
  struct cross_conversion<cppcomponents::asio_runtime::basic_simple_buffer<Char>>
    :trivial_conversion<cppcomponents::asio_runtime::basic_simple_buffer<Char>>{};
  template<class Char>
  struct type_name_getter<cppcomponents::asio_runtime::basic_simple_buffer<Char>>
  {
    static std::string get_type_name(){ return "cppcomponents::asio_runtime::basic_simple_buffer"; }
  };

}

namespace cppcomponents{
  namespace asio_runtime{




    //struct IIOService :define_interface<cppcomponents::uuid<0x5b2da622, 0x990e, 0x45b4, 0x8ab2, 0xc7ef18a1d6a6>>
    //{
    //	typedef use<delegate<void()>> Handler;

    //	void Dispatch(Handler h);
    //	void Poll();
    //	void PollOne();
    //	void Post(Handler h);
    //	void Reset();
    //	void Stop();
    //	void Stopped();

    //	CPPCOMPONENTS_CONSTRUCT(IIOService, Dispatch, Poll, PollOne, Post, Reset, Stop, Stopped);

    //};

    struct IThreadPool :define_interface<cppcomponents::uuid<0x3ad5b16e, 0xa68f, 0x48d1, 0x9343, 0x6a5f01c5717a>, IExecutor>
    {
      bool AddThread();
      bool RemoveThread();
      void Join();

      CPPCOMPONENTS_CONSTRUCT(IThreadPool, AddThread, RemoveThread, Join);
    };



    struct IRuntimeStatics :define_interface<cppcomponents::uuid<0x64d84abd, 0x7333, 0x420b, 0x80ce, 0x9462c191afe7>>
    {
      use<IThreadPool> GetThreadPoolRaw(std::int32_t num_threads, std::int32_t min_threads, std::int32_t max_threads);
      //use <IIOService> GetIOService();

      CPPCOMPONENTS_CONSTRUCT(IRuntimeStatics, GetThreadPoolRaw);

      CPPCOMPONENTS_STATIC_INTERFACE_EXTRAS(IRuntimeStatics){
        static use<IThreadPool> GetThreadPool(std::int32_t num_threads = -1, std::int32_t min_threads = 2, std::int32_t max_threads = 100){
          return Class::GetThreadPoolRaw(num_threads, min_threads, max_threads);
        }
      };
    };

    inline std::string RuntimeId(){ return "cppcomponents_asio_dll!Runtime"; }

    typedef runtime_class<RuntimeId, factory_interface<NoConstructorFactoryInterface>, static_interfaces<IRuntimeStatics> > Runtime_t;
    typedef use_runtime_class<Runtime_t> Runtime;

    struct ThreadPoolBoost{
    private:
      bool added_ = false;

      ThreadPoolBoost(){
        added_ = Runtime::GetThreadPool().AddThread();
      }

      ~ThreadPoolBoost(){
        if (added_){
          Runtime::GetThreadPool().RemoveThread();
        }
      }
    };

    struct IAsyncStream :define_interface<cppcomponents::uuid<0x891a8183, 0xe68d, 0x4265, 0xa108, 0x6b4e0519d254>>{
      Future<std::size_t> Read(simple_buffer buf);
      Future<std::size_t> ReadAt(std::uint64_t offset, simple_buffer buf);

      Future<use<IBuffer>> ReadBuffer();
      Future<use<IBuffer>> ReadBufferAt(std::uint64_t offset);
      Future<use<IBuffer>> ReadBufferUntilChar(char c);
      Future<use<IBuffer>> ReadBufferUntilString(cr_string delim);

      Future<void> Poll();

      Future<std::size_t> Write(const_simple_buffer data);
      Future<std::size_t> WriteAt(std::uint64_t offset, const_simple_buffer data);

      CPPCOMPONENTS_CONSTRUCT(IAsyncStream, Read, ReadAt, 
        ReadBuffer, ReadBufferAt, ReadBufferUntilChar, ReadBufferUntilString,Poll,Write,WriteAt);

    };



    struct IIPAddress :define_interface<cppcomponents::uuid<0x4cfca553, 0xa840, 0x49e6, 0xa3ce, 0x2b989b4f703d>>
    {
      bool IsV4();
      bool IsV6();
      std::string ToString();
      void ToBytes(simple_buffer bytes);

      CPPCOMPONENTS_CONSTRUCT(IIPAddress, IsV4, IsV6, ToString, ToBytes);
    };

    struct IIPAddressStatic :define_interface<cppcomponents::uuid<0xa4969be1, 0xb3cb, 0x41b3, 0xacea, 0x2074dbe8dfde>>{
      use<IIPAddress> V4FromString(cr_string str);
      use<IIPAddress> V4FromBytes(simple_buffer bytes);
      use<IIPAddress> V4Broadcast();
      use<IIPAddress> V4Loopback();
      use<IIPAddress> V4Any();

      use<IIPAddress> V6FromString(cr_string str);
      use<IIPAddress> V6FromBytes(simple_buffer bytes);
      use<IIPAddress> V6Loopback();
      use<IIPAddress> V6Any();
      use<IIPAddress> V6V4Compatible(use<IIPAddress> v4);

      CPPCOMPONENTS_CONSTRUCT(IIPAddressStatic, V4FromString, V4FromBytes, V4Broadcast, V4Loopback, V4Any,
        V6FromString, V6FromBytes, V6Loopback, V6Any, V6V4Compatible);

    };

    inline std::string IPAddressId(){ return "cppcomponents_asio_dll!IPAddress"; }
    typedef runtime_class < IPAddressId, object_interfaces<IIPAddress>, static_interfaces<IIPAddressStatic>,
      factory_interface < NoConstructorFactoryInterface >> IPAddress_t;
    typedef use_runtime_class<IPAddress_t> IPAddress;

    struct endpoint{
      use<IIPAddress> address_;
      std::int32_t port_;
    };

    namespace detail{
#pragma pack(push, 1)
      struct end_point_trivial{
        cross_compiler_interface::cross_conversion<use<IIPAddress>>::converted_type caddress_;
        std::int32_t port_;
      };
#pragma pack(pop)
    }

  }
}

namespace cross_compiler_interface{


  template<>
  struct type_name_getter<cppcomponents::asio_runtime::endpoint>
  {
    static std::string get_type_name(){ return "cppcomponents::asio_runtime::endpoint"; }
  };

  template<>
  struct cross_conversion<cppcomponents::asio_runtime::endpoint>{
    // Required typedefs
    typedef cppcomponents::asio_runtime::endpoint original_type;
    typedef cppcomponents::asio_runtime::detail::end_point_trivial converted_type;

    // static functions to do the conversion

    static converted_type to_converted_type(const original_type& o){
      converted_type ret;
      ret.caddress_ = cross_conversion<cppcomponents::use<cppcomponents::asio_runtime::IIPAddress>>::to_converted_type(o.address_);
      ret.port_ = o.port_;
      return ret;
    }
    static original_type to_original_type(const converted_type& c){
      original_type ret;
      ret.address_ = cross_conversion<cppcomponents::use<cppcomponents::asio_runtime::IIPAddress>>::to_original_type(c.caddress_);
      ret.port_ = c.port_;
      return ret;
    }
  };
}

namespace cppcomponents{
  namespace asio_runtime{
    struct IAsyncDatagram :define_interface<cppcomponents::uuid<0x5e1f8df2, 0x3485, 0x416a, 0x8024, 0x9f18f7ecad02>>
    {
      Future<void> ReceiveRaw(simple_buffer buf, std::uint32_t flags);
      Future<void> ReceiveFromRaw(simple_buffer buf, endpoint sender, std::uint32_t flags);
      Future<use<IBuffer>> ReceiveBufferRaw(std::uint32_t flags);
      Future<use<IBuffer>> ReceiveFromBufferRaw(endpoint sender, std::uint32_t flags);
      void SendRaw(const_simple_buffer buffer, std::uint32_t flags);
      void SendToRaw(const_simple_buffer buffer, endpoint receiver, std::uint32_t flags);

      CPPCOMPONENTS_CONSTRUCT(IAsyncDatagram, ReceiveRaw, ReceiveFromRaw,
        SendRaw, SendToRaw);
    };


    struct ISocket :define_interface<cppcomponents::uuid<0xbb639a67, 0x6442, 0x45b6, 0x8085, 0xdbc3082912a2>>
    {
      enum{
        AddressConfigured = 1,
        AllMatching = 2,
        CanonicalName = 4,
        NumericHost = 8,
        NumericService = 16,
        Passive = 32,
        V4Mapped = 64
      };

      enum{
        IPV4 = 4,
        IPV6 = 6
      };

      enum{
        ShutdownBoth = 2,
        ShutdownSend = 0,
        ShutdownReceive = 1
      };
      void AssignRaw(std::int32_t ip_type,std::uint64_t);
      Future<void> Connect(endpoint e);
      Future<void> ConnectQueryRaw(cr_string host, cr_string service, std::uint32_t flags);
      bool AtMark();
      std::size_t Available();
      void Bind(endpoint e);
      void Cancel();
      void Close();
      void GetOptionRaw(int level, int option_name, void* option_value, std::size_t option_len);
      void SetOptionRaw(int level, int option_name, const void* option_value, std::size_t option_len);
      bool IsOpen();
      int NativeHandle();
      bool GetNonBlocking();
      void SetNonBlocking(bool);
      bool GetNativeNonBlocking();
      void SetNativeNonBlocking(bool);
      void OpenRaw(std::int32_t ip_type);
      endpoint RemoteEndpoint();
      void Shutdown(std::int32_t type);

      CPPCOMPONENTS_CONSTRUCT(ISocket, AssignRaw, Connect, ConnectQueryRaw, AtMark,
        Available, Bind, Cancel, Close, GetOptionRaw, SetOptionRaw,
        IsOpen, NativeHandle, GetNonBlocking, SetNonBlocking, GetNativeNonBlocking,
        SetNativeNonBlocking, OpenRaw, RemoteEndpoint, Shutdown);



    };

    struct ITimer :define_interface<cppcomponents::uuid<0xb9618857, 0x9b3e, 0x452e, 0x92c4, 0x352f9e0334e8>>
    {
      Future<void> Wait();
      void Cancel();
      void CancelOne();
      void ExpiresFromNowRaw(std::uint64_t microseconds);

      CPPCOMPONENTS_CONSTRUCT(ITimer, Wait, Cancel, CancelOne, ExpiresFromNowRaw);

      CPPCOMPONENTS_INTERFACE_EXTRAS(ITimer){
        template<class Duration>
        void ExpiresFromNow(Duration d){
          this->get_interface().ExpiresFromNowRaw(std::chrono::duration_cast<std::chrono::microseconds>(d).count());
        }
      };

    };

    struct ITimerStatics :define_interface<cppcomponents::uuid<0xb9618857, 0x9b3e, 0x452e, 0x92c4, 0x352f9e0334e8>>
    {
      Future<void> WaitForRaw(std::uint64_t microseconds);


      CPPCOMPONENTS_CONSTRUCT(ITimerStatics, WaitForRaw);

      CPPCOMPONENTS_STATIC_INTERFACE_EXTRAS(ITimerStatics){
        template<class Duration>
        static Future<void> WaitFor(Duration d){
          return Class::WaitForRaw(std::chrono::duration_cast<std::chrono::microseconds>(d).count());
        }
      };
    };

    inline std::string TimerId(){ return "cppcomponents_asio_dll!Timer"; }

    typedef runtime_class<TimerId, object_interfaces<ITimer>, static_interfaces<ITimerStatics>> Timer_t;
    typedef use_runtime_class<Timer_t> Timer;

    struct IAcceptor :define_interface<cppcomponents::uuid<0x6269086f, 0xaaa7, 0x4ea5, 0xa65c, 0xbbf0b8394ffe>>
    {
      Future<void> Accept(use<ISocket>);

      CPPCOMPONENTS_CONSTRUCT(IAcceptor, Accept);
    };

    struct IAcceptorCreator :define_interface<cppcomponents::uuid<0x0f2f5f06, 0xabbd, 0x4a76, 0xb054, 0x5cdab96ea8f4>>
    {
      use<InterfaceUnknown> Create(endpoint e, bool reuse_addr);

      CPPCOMPONENTS_CONSTRUCT(IAcceptorCreator, Create);

      CPPCOMPONENTS_INTERFACE_EXTRAS(IAcceptorCreator){
        use<InterfaceUnknown> TemplatedConstructor(endpoint e, bool reuse_address = true){
          return this->get_interface().Create(e, reuse_address);
        }
      };
    };

    inline std::string TcpAcceptorId(){ return "cppcomponents_asio_dll!TcpAcceptor"; }
    typedef runtime_class<TcpAcceptorId, object_interfaces<IAcceptor>, factory_interface<IAcceptorCreator>> TcpAcceptor_t;
    typedef use_runtime_class<TcpAcceptor_t> TcpAcceptor;

    inline std::string TcpId(){ return "cppcomponents_asio_dll!Tcp"; }
    inline std::string UdpId(){ return "cppcomponents_asio_dll!Udp"; }

    typedef runtime_class<TcpId, object_interfaces<ISocket, IAsyncStream>> Tcp_t;
    typedef use_runtime_class<Tcp_t> Tcp;

    typedef runtime_class<UdpId, object_interfaces<ISocket, IAsyncDatagram>> Udp_t;
    typedef use_runtime_class<Udp_t> Udp;

  }

  template<>
  struct uuid_of<asio_runtime::simple_buffer>{
    typedef cppcomponents::uuid<0xa198811e, 0x199f, 0x4b4a, 0xbac0, 0x9b3944d566ad> uuid_type;
  };
  template<>
  struct uuid_of<asio_runtime::const_simple_buffer>{
    typedef cppcomponents::uuid<0x6d3336ce, 0x24e3, 0x4922, 0xad21, 0xfb56d19bbef0> uuid_type;
  };
  template<>
  struct uuid_of<asio_runtime::endpoint>{
    typedef cppcomponents::uuid<0x6ac33669, 0x0ea0, 0x4380, 0x8d20, 0x51d479e56711> uuid_type;
  };




}

