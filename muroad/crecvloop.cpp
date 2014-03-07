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
#include "crecvloop.h"
#include "caudioframe.h"
#include "csocket.h"
#include "cpacketringbuffer.h"
#include "cplayer.h"

#include "CApp.h"

using namespace std;
using namespace muroa;

CRecvloop::CRecvloop(CPlayer* parent, CApp* app, CPacketRingBuffer* packet_ringbuffer):
		CThreadSlave(),
		m_app(app),
		m_settings(app->settings()),
		m_rtp_packet(0)
{

  m_player = parent;
  
  m_packet_ringbuffer = packet_ringbuffer;

  m_max_idle = m_settings.getProptery("MaxIdle", 10);


  m_socket = new CSocket(SOCK_DGRAM, m_settings.port(), true);
  m_settings.setPort( m_socket->getPort() );

  m_socket->recordSenderWithRecv(true);

}


CRecvloop::~CRecvloop()
{
  delete m_socket;
  if(m_rtp_packet) {
	  delete m_rtp_packet;
  }
  
}


void CRecvloop::DoLoop()
{
  m_rtp_packet = new CRTPPacket();
  int num = m_socket->read(m_rtp_packet->bufferPtr(), m_rtp_packet->bufferSize()); 
  m_rtp_packet->commit(num);

  if(num <= 0 ) {
    // m_rtp_packet->usedPayloadBufferSize(0);
    usleep(200);
  }
  else {

    switch( m_rtp_packet->payloadType() ) {
      case PAYLOAD_SYNC_OBJ:
        cerr << "CRecvloop::DoLoop got SyncObj:";
        m_tmp_sync_obj.deserialize(m_rtp_packet);
        if(m_tmp_sync_obj.streamId() == m_player->syncRequestedForStreamID()) {
          // this sync object has been requested by the client. Use it immediately
          m_player->setRequestedSyncObj(m_rtp_packet);
        }
        else {
          // beginning of next stream. put sync object into the packet ringbuffer
          m_packet_ringbuffer->appendRTPPacket(m_rtp_packet);
        }

        // wake up playback thread
        if(m_player->idleTime() > m_max_idle && m_max_idle != 0) {
           m_player->m_traffic_cond.Signal();
        }
        
        break;

      case PAYLOAD_RESET_STREAM:
      {
    	  CmdStreamReset* cmd_rst = new CmdStreamReset(m_rtp_packet);
    	  delete m_rtp_packet;
    	  m_player->onResetStream(*cmd_rst);
    	  delete cmd_rst;
    	  break;
      }
      case PAYLOAD_PCM:
      case PAYLOAD_MP3:
      case PAYLOAD_VORBIS:
      case PAYLOAD_FLAC:
        // m_rtp_packet->BufferSize(num);
        m_packet_ringbuffer->appendRTPPacket(m_rtp_packet);
        // cerr << "Sender was: " << m_socket->latestSender()->ipAddress() << " port " << m_socket->latestSender()->port() << endl;
        
        // wake up playback thread
        if(m_player->idleTime() > m_max_idle && m_max_idle != 0) {
           m_player->m_traffic_cond.Signal();
        }

        break;
      default:
        cerr << "CRecvloop::DoLoop(): unknown payload type: " << m_rtp_packet->payloadType() << endl; 
        
    }
  }

}


/*!
    \fn CRecvloop::sendPacket(CRTPPacket* packet)
 */
void CRecvloop::sendRTPPacket(CRTPPacket* packet)
{
    m_socket->sendTo(m_socket->latestSender(), packet->bufferPtr(), packet->usedBufferSize());
}
