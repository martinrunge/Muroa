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
#include "csync.h"
#include "crtppacket.h"

#include <netinet/in.h>

using namespace std;
using namespace boost::posix_time;


CSync::CSync(enum sync_type_t sync_type)
{
  cerr << "CSync::CSync(enum)" << endl;
  m_sync_type = sync_type;
  m_local_time = new ptime(microsec_clock::universal_time());
  print();

}

CSync::CSync(CRTPPacket* rtp_packet) 
{
  if(rtp_packet->payloadType() != PAYLOAD_SYNC_OBJ) return;

  cerr << "CSync::CSync(CRTPPacket*) (test) " ;
  m_local_time = new ptime(from_iso_string("19700101T000000"));  // just a dummy to feed the c-tor. The value is never used!
  memcpy(m_serialization_buffer.raw_buffer, rtp_packet->payloadBufferPtr(), sizeof(m_serialization_buffer));
  deserialize();
  print();
}

CSync::~CSync()
{
  delete m_local_time;
}


/*!
    \fn CSync::serialize()
 */
char* CSync::serialize()
{

  uint32_t tmp = m_sync_type;
  m_serialization_buffer.serialisation_vars.sync_type = htonl(tmp);
  m_serialization_buffer.serialisation_vars.stream_id = htonl(m_stream_id);
  m_serialization_buffer.serialisation_vars.session_id = htonl(m_session_id);

  m_serialization_buffer.serialisation_vars.frame_nr = htonl(m_frame_nr);

  strcpy((char*)m_serialization_buffer.serialisation_vars.timestamp, to_simple_string(*m_local_time).c_str() );

  return m_serialization_buffer.raw_buffer;    
}


/*!
    \fn CSync::deserialize()
 */
void CSync::deserialize(void)
{
  uint32_t tmp = m_serialization_buffer.serialisation_vars.sync_type;
  m_sync_type = static_cast<sync_type_t>(ntohl(tmp));
  
  m_stream_id = ntohl( m_serialization_buffer.serialisation_vars.stream_id );
  m_session_id = ntohl( m_serialization_buffer.serialisation_vars.session_id );

  m_frame_nr = ntohl( m_serialization_buffer.serialisation_vars.frame_nr );

  *m_local_time = time_from_string( m_serialization_buffer.serialisation_vars.timestamp );
}


/*!
    \fn CSync::serialisationBufferSize()
 */
int CSync::getSerialisationBufferSize()
{
    return sizeof(m_serialization_buffer);
}


/*!
    \fn CSync::getSerialisationBufferPtr
 */
char* CSync::getSerialisationBufferPtr() 
{
    return m_serialization_buffer.raw_buffer;
}


/*!
    \fn CSync::print()
 */
void CSync::print()
{
  cerr << "session/stream ID (" << sessionId() << "/" << streamId() << ") frame nr " 
       << frameNr() << " at " << *m_local_time << endl;
}


/*!
    \fn CSync::addDuration(boost::posix_time::time_duration duration);
 */
void CSync::addDuration(boost::posix_time::time_duration duration)
{
  *m_local_time += duration;
}


/*!
    \fn CSync::setTimeToNow()
 */
void CSync::setTimeToNow()
{
  *m_local_time = microsec_clock::universal_time();
}


/*!
    \fn CSync::deserialize(RTPPacket* sync_packet)
 */
void CSync::deserialize(CRTPPacket* sync_packet)
{
  if(sync_packet->payloadType() != PAYLOAD_SYNC_OBJ) return;
  /* if(sync_packet->payloadBufferSize() != sizeof(m_serialization_buffer)) {
    cerr << "CSync::deserialize(CRTPPacket*): Warning: payload size does not fit. Ignoring packet!" << endl;
    return;
  }*/

  memcpy(m_serialization_buffer.raw_buffer, sync_packet->payloadBufferPtr(), sizeof(m_serialization_buffer));
  deserialize();
  // print();
}

ostream& operator<< (ostream &out, CSync &so)
{
    out << "session/stream ID (" << so.m_session_id << "/" << so.m_stream_id << ") frame nr "
         << so.frameNr() << " at " << *(so.m_local_time);

    return out;
}
