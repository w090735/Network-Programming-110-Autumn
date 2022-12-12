/* C libraries */
#include <pthread.h>
#include <stdint.h> // uint8_t to int
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
/* C++ libraries */
#include <iostream>
#include <cstdlib>
#include <memory>
#include <utility>
#include <map>
#include <vector>
/* boost libraries */
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>


using namespace std;
/* boost namespace */
using namespace boost; 
using namespace boost::asio;
using namespace boost::asio::ip::tcp;
using namespace boost::algorithm;

/*
    IOService{
        boost::asio::ip::tcp::socket
        boost::asio::ip::tcp::acceptor
        boost::asio::ip::udp::socket
        deadline_timer
    }
*/
using IOService = boost::asio::io_service;
using SignalSet = boost::asio::signal_set;
using Acceptor  = boost::asio::ip::tcp::acceptor;
using Socket    = boost::asio::ip::tcp::socket;
using Resolver  = boost::asio::ip::tcp::resolver;
using ErrCode   = boost::system::error_code;
using Endpoint  = boost::asio::ip::tcp::endpoint;
using TCP       = boost::asio::ip::tcp;

#define ERR(str, ec) cerr << str << " err: " << ec.message() << "\n";
