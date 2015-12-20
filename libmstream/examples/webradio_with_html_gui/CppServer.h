/*
Copyright (c) 2002-2014 "Martin Runge"

server.h is part of muroa, the  Multi Room Audio Player [http://www.muroa.org]

Muroa is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef LIBMSTREAM_EXAMPLES_SERVER_H_
#define LIBMSTREAM_EXAMPLES_SERVER_H_

#include <iostream>
#include <cstdio>
#include <getopt.h>
#include <vector>
#include <string>

#include <boost/asio.hpp>
#include <CStreamServer.h>
#include "CStreamDecoder.h"

namespace bip=boost::asio::ip;


class CppServer :public CStreamServer {
public:
	CppServer(boost::asio::io_service& io_service, std::vector<bip::tcp::endpoint> clients, int timeServerPort, int sessionID );
	~CppServer();

	void playStream(std::string url);
	void stopStream();

	void reportClientState(muroa::CStreamCtrlConnection* conn, const muroa::CmdStreamBase* evt);
	void reportProgress( int posInSecs, int durationInSecs);

private:
	boost::asio::io_service& m_io_service;
	CStreamDecoder *m_decoder;

};




#endif /* LIBMSTREAM_EXAMPLES_SERVER_H_ */
