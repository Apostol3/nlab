﻿#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <cstring>
#include <limits>

#include "remote_env.h"

using asio::ip::tcp;

class tcp_stream : public base_stream
{
	asio::io_service _io_service;
	tcp::socket _sock;
	tcp::acceptor _acceptor;
	void* _buf;
	size_t _buf_size;
	bool _server;
	bool _reopen;
	std::string _host;
	std::string _port;
	size_t static const max_internal_buffer = 1024;

public:
	explicit tcp_stream(size_t buf_size);
	tcp_stream(std::string host, std::string port, size_t buf_size);
	tcp_stream(const tcp_stream&) = delete;

	tcp_stream& operator =(const tcp_stream& a) = delete;
	~tcp_stream();
	void receive(void** ppd, size_t& sz) override;
	void send(const void* pd, size_t sz) override;
	bool is_connected() const override;
	void connect(std::string host, std::string port);
	void connect() override;
	void create(std::string port);
	void create() override;
	void disconnect() override;
	void close() override;
};

inline tcp_stream::tcp_stream(size_t buf_size) :
	_sock(_io_service),_acceptor(_io_service), _buf_size(buf_size), _server(false)
{
	_reopen = true;
	_buf = new char[buf_size];
}

inline tcp_stream::tcp_stream(std::string host, std::string port, size_t buf_size) :
	tcp_stream(buf_size)
{
	_port = port;
	_reopen = false;
}

inline tcp_stream::~tcp_stream()
{
	delete[] static_cast<char*>(_buf);
}

inline void tcp_stream::receive(void** ppd, size_t& sz)
{
	asio::error_code ec;
	sz = 0;
	size_t sz_part;
	
	if (_server&&_reopen)
		_sock = tcp::socket(_io_service);

	if (_reopen)
	{
		_acceptor.accept(_sock, ec);

		if (ec == asio::error::would_block)
		{
			sz = 0;
			return;
		}
		
		if (ec != asio::error_code())
			asio::detail::throw_error(ec, "receive_from");
	}
	
	sz_part = _sock.read_some(asio::buffer(_buf, max_internal_buffer), ec);
	
	if (ec == asio::error::would_block)
	{
		sz = 0;
		return;
	}
	
	if (ec != asio::error_code())
		asio::detail::throw_error(ec, "receive_from");
	
	sz = sz_part;

	if (static_cast<char*>(_buf)[sz_part-1] != '\0')
	{
		auto part_start = static_cast< char* >(_buf);
		try
		{
			do {
				part_start += sz_part;
				sz_part = _sock.read_some(asio::buffer(part_start, max_internal_buffer), ec);
				if (ec == asio::error::would_block)
				{
					//TODO: add some sleep here
					sz_part = 0;
					continue;
				}
				sz += sz_part;
				if (sz + max_internal_buffer > _buf_size)
					throw std::runtime_error("receive buffer overflow");
			} while (static_cast<char*>(part_start)[sz_part - 1] != '\0');
		}
		catch (std::exception& )
		{
			throw;
		}
		
	}

	*ppd = _buf;
}

inline void tcp_stream::send(const void* pd, size_t sz)
{
	asio::write(_sock, asio::buffer(static_cast< const char* >(pd), sz));
}

inline bool tcp_stream::is_connected() const
{
	return _sock.is_open();
}

inline void tcp_stream::connect()
{
	throw std::runtime_error("not implemented");
}

inline void tcp_stream::connect(std::string host, std::string port)
{
	throw std::runtime_error("not implemented");
}


inline void tcp_stream::create()
{
	if (_acceptor.is_open())
		_acceptor.close();
	int port_num;

	try
	{
		port_num = std::stoi(_port.c_str());
		
	}
	catch (std::invalid_argument&)
	{
		throw std::invalid_argument("tcp error: invalid port");
	}

	if (port_num < 0 || port_num > std::numeric_limits<unsigned short>::max())
		throw std::invalid_argument("tcp error: invalid port");

	_acceptor = tcp::acceptor(_io_service, tcp::endpoint(tcp::v4(),
		static_cast<unsigned short>(port_num)));

	_acceptor.non_blocking(true);
	if (!_reopen)
	{
		asio::error_code ec;
		_sock = tcp::socket(_io_service);
		while (true)
		{
			_acceptor.accept(_sock, ec);
			if (ec == asio::error::would_block)
			{
				continue;
			}
			if (ec != asio::error_code())
			{
				asio::detail::throw_error(ec, "receive_from");
			}
			break;
		}
	}
	_server = true;
}

inline void tcp_stream::create(std::string port)
{
	_port = port;
	create();
}

inline void tcp_stream::disconnect()
{
	throw std::runtime_error("not implemented");
}

inline void tcp_stream::close()
{
	if (_acceptor.is_open())
		_acceptor.close();
	_sock.close();
}

