/* C libraries */
#include <sys/wait.h>
#include <unistd.h> // usleep
/* C++ libraries */
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
/* boost libraries */
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp> // replace_all
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>

using namespace std;
/* boost namespace */
using namespace boost::asio;
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
using Resolver  = boost::asio::ip::tcp::resolver; // resolve host to endpoints
using ErrCode   = boost::system::error_code;
using Socket    = boost::asio::ip::tcp::socket;
using SignalSet = boost::asio::signal_set;
using Acceptor  = boost::asio::ip::tcp::acceptor;
using TCP       = boost::asio::ip::tcp;
using Endpoint  = boost::asio::ip::tcp::endpoint;

#define ERR(str, ec) cerr << str << " err: " << ec.message() << "\n";
