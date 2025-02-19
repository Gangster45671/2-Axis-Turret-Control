//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, synchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost\beast\core\buffers_to_string.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include "main.h"

#include "serial.h"
#include <iostream>
#include <sstream>
#include <string> // for string and to_string() 
using namespace std;

int pitch = 0;
int yaw = 0;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
string sport = "";
										//------------------------------------------------------------------------------
										// Echoes back all received WebSocket messages
void
do_session(tcp::socket& socket)
{
	try
	{
		// Construct the stream by moving in the socket
		websocket::stream<tcp::socket> ws{ std::move(socket) };

		// Set a decorator to change the Server of the handshake
		ws.set_option(websocket::stream_base::decorator(
			[](websocket::response_type& res)
		{
			res.set(http::field::server,
				std::string(BOOST_BEAST_VERSION_STRING) +
				" websocket-server-sync");
		}));

		// Accept the websocket handshake
		ws.accept();


		//open serial port
		serial myserial(sport, 115200);
		

		for (;;)
		{

			// This buffer will hold the incoming message
			beast::flat_buffer buffer;

			// Read a message
			ws.read(buffer);

			// Echo the message back
			ws.text(ws.got_text());
			ws.write(buffer.data());

			//send position to serial port
			string str = "";
			str = (string)boost::beast::buffers_to_string(buffer.data());

			const char* c = str.c_str();

			myserial.write(c, str.size());
			std::cout << c << std::endl;

			std::cout << (string)boost::beast::buffers_to_string(buffer.data()) << std::endl;
		}
	}
	catch (beast::system_error const& se)
	{
		// This indicates that the session was closed
		if (se.code() != websocket::error::closed)
			std::cerr << "Error: " << se.code().message() << std::endl;
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{



	try
	{
		// Check command line arguments.
		if (argc != 4)
		{
			std::cerr <<
				"Usage: websocket-server-sync <address> <port> <serialcom>\n" <<
				"Example:\n" <<
				"    websocket-server-sync 0.0.0.0 8080 COM1\n";
			return EXIT_FAILURE;
		}
		auto const address = net::ip::make_address(argv[1]);
		auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
		sport = string(argv[3]);
		
		// The io_context is required for all I/O
		net::io_context ioc{ 1 };

		// The acceptor receives incoming connections
		tcp::acceptor acceptor{ ioc,{ address, port } };
		for (;;)
		{
			// This will receive the new connection
			tcp::socket socket{ ioc };

			// Block until we get a connection
			acceptor.accept(socket);

			// Launch the session, transferring ownership of the socket
			std::thread{ std::bind(
				&do_session,
				std::move(socket)) }.detach();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
