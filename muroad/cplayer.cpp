/***************************************************************************
 *   Copyright (C) 2005 by Martin Runge   *
 *   martin.runge@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "cplayer.h"

#include <iostream>

#include <cstdlib>
#include <string.h>

#include <sys/stat.h> 

#include "cpthread.h"
#include "libsock++.h"
#include "libdsaudio.h"

#include "caudioframe.h"
#include "csocket.h"

#include "cplayloop.h"
#include "crecvloop.h"
#include "cpacketringbuffer.h"

#include "CApp.h"
#include "CSettings.h"
#include "avahi/CDnsSdAvahi.h"


using namespace std;
using namespace boost::posix_time;
using namespace muroa;

CPlayer::CPlayer(CApp* app, boost::asio::io_service& io_service) : m_app(app), m_settings(app->settings()), m_io_service(io_service)
{
  
  int num;
  cout << "dsclient" << endl;

  m_settings.setServiceType("_muroad._udp");
  m_settings.setServiceName("Muroa Streaming Client");
  m_settings.setPort(44400);
   
  m_packet_ringbuffer = new CPacketRingBuffer(10);

  m_recvloop = new CRecvloop(this, m_app, m_packet_ringbuffer);
  m_playloop = new CPlayloop(this, m_app, m_packet_ringbuffer);

  m_recvloop_thread = new CPThread(m_recvloop);
  m_playloop_thread = new CPThread(m_playloop);

  m_sync_requested_for_stream_id = -1;
  m_sync_requested_at = microsec_clock::universal_time();

  m_dnssd = new CDnsSdAvahi(io_service, app->settings().serviceName(), app->settings().port(), app->settings().serviceType(), vector<string>());
  m_dnssd->setServiceChangedHandler(boost::bind( &muroa::CApp::serviceChanged, app));

  m_idle_time = 0;

  //CSync syncobj;
}


CPlayer::~CPlayer()
{
  delete m_recvloop_thread;
  delete m_playloop_thread;

  delete m_recvloop;
  delete m_playloop;

  delete m_packet_ringbuffer;

}


/*!
    \fn CPlayer::start()
 */
void CPlayer::start()
{
  m_recvloop_thread->StartThread();
  m_playloop_thread->StartThread(true);
}

/*!
    \fn CPlayer::stop()
 */
void CPlayer::stop()
{
  m_recvloop_thread->StopThread();
  m_playloop_thread->StopThread();
}


/*!
    \fn CPlayer::sendSyncReq(CSync* sync_req)
 */
void CPlayer::sendRTPPacket(CRTPPacket* packet)
{
    m_recvloop->sendRTPPacket(packet);
}


/*!
    \fn CPlayer::sync()
 */
void CPlayer::sync()
{
  m_playloop->sync();    
}

void CPlayer::setSyncObj(CRTPPacket* rtp_packet) {
  m_sync_obj.deserialize( rtp_packet);
  m_playloop->setSync(&m_sync_obj);
}


/*!
    \fn CPlayer::setRequestedSyncObj(CRTPPacket* rtp_packet)
 */
void CPlayer::setRequestedSyncObj(CRTPPacket* rtp_packet)
{
    m_sync_requested_for_stream_id = -1;
    setSyncObj(rtp_packet);
}


/*!
    \fn CPlayer::requestSync(int stream_id)
 */
void CPlayer::requestSync(int session_id, int stream_id)
{
  if( m_sync_requested_for_stream_id == -1) {  // no request is underway yet  
    CSync *sync_req = new CSync(SYNC_REQ_STREAM);
    sync_req->sessionId(session_id);
    sync_req->streamId(stream_id);
    sync_req->serialize();

    cerr << "sending request for sync obj." << endl;

    CRTPPacket* tmp_packet = new CRTPPacket(session_id, stream_id, sync_req->getSerialisationBufferSize(), true);

    tmp_packet->copyInPayload(sync_req->getSerialisationBufferPtr(), sync_req->getSerialisationBufferSize());
      
    tmp_packet->payloadType(PAYLOAD_SYNC_OBJ);
    sendRTPPacket(tmp_packet);

    m_sync_requested_for_stream_id = stream_id;
    m_sync_requested_at = microsec_clock::universal_time();

    delete sync_req;
    delete tmp_packet;     
  }
}

void CPlayer::onResetStream(const CmdStreamReset& cmd_rst) {
    cerr << "CPlayer::onResetStream: sessionID: " << cmd_rst.getOldSessionId()
         << " streamID: " << cmd_rst.getOldStreamId() << endl
         << " newSessionID: " << cmd_rst.getNewSessionId()
         << " newStreamID: " << cmd_rst.getNewStreamId() << endl;
	m_packet_ringbuffer->clearContent(cmd_rst.getOldSessionId(), cmd_rst.getOldStreamId());
	m_playloop->reset(cmd_rst.getOldSessionId(), cmd_rst.getOldStreamId());
}




