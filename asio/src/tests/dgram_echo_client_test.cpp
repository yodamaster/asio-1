#include "asio.hpp"
#include <boost/bind.hpp>
#include <iostream>
#include <string.h>

using namespace asio;

class dgram_handler
{
public:
  dgram_handler(demuxer& d)
    : demuxer_(d),
      timer_(d, timer::from_now, timeout),
      socket_(d, inet_address_v4(32123)),
      count_(0),
      stop_(false)
  {
    timer_.async_wait(boost::bind(&dgram_handler::handle_timeout, this));

    inet_address_v4 destination(32124, "localhost");
    char msg[] = "Hello, World!\n";
    memcpy(data_, msg, sizeof(msg));
    socket_.async_sendto(data_, sizeof(msg) - 1, destination,
        boost::bind(&dgram_handler::handle_sendto, this, _1, _2));
  }

  ~dgram_handler()
  {
    std::cout << count_ << " round trips\n";
    std::cout << (count_ / timeout) << " round trips per second\n";
  }

  void handle_timeout()
  {
    stop_ = true;
    socket_.close();
  }

  void handle_sendto(const socket_error& error, size_t length)
  {
    if (!error && !stop_)
    {
      socket_.async_recvfrom(data_, max_length, sender_address_,
          boost::bind(&dgram_handler::handle_recvfrom, this, _1, _2));
    }
  }

  void handle_recvfrom(const socket_error& error, size_t length)
  {
    if (!error && length > 0 && !stop_)
    {
      ++count_;
      socket_.async_sendto(data_, length, sender_address_,
          boost::bind(&dgram_handler::handle_sendto, this, _1, _2));
    }
  }

private:
  demuxer& demuxer_;
  enum { timeout = 10 };
  timer timer_;
  dgram_socket socket_;
  inet_address_v4 sender_address_;
  enum { max_length = 512 };
  char data_[max_length];
  int count_;
  bool stop_;
};

int main()
{
  try
  {
    demuxer d;
    dgram_handler dh(d);
    d.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
