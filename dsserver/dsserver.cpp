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

#include <iostream>
#include <sys/time.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "libsock++.h"
#include "libdsaudio.h"

#include "csocket.h"
#include "csync.h"
#include "crtppacket.h"

using namespace std;
using namespace boost::posix_time;


int main(int argc, char *argv[]) {
  
  unsigned long num;
  int payload_size = 1024;

  /** the actual system time when the last packet was sent */
  struct timeval m_send_time;

  /** system time when the first packet was sent */
  unsigned long long m_first_send_time = 0;
  unsigned long long send_time = 0;
  unsigned long long m_payload_duration_sum = 0;
  unsigned long long total_play_time = 0;

  ptime m_last_send_time = microsec_clock::local_time();
  ptime now = m_last_send_time;

  time_duration interval;

  CRTPPacket rtp_packet(payload_size);
  
  cout << "testserver!!!" << endl;

  CSocket socket(SOCK_DGRAM);  

  socket.connect("localhost", 4001);

  unsigned long payload_duration_in_us = 0;
  long time_since_last_packet_in_us;
  long audio_bytes_per_second = 44100 * 2 * 2;

  int m_last_payload_duration_in_us = 0;
        

  FILE* in_fd = fopen("infile.raw", "r");
  
  int transport_buffer_size_in_ms = 500;

  CSync syncobj;
  time_duration buffersize = millisec(transport_buffer_size_in_ms);

  syncobj.addDuration(buffersize);
  syncobj.sampleNr(0);
  syncobj.streamId(1);  
  syncobj.sessionId(1);
  syncobj.serialize();

  rtp_packet.payloadType(PAYLOAD_SYNC_OBJ);

  rtp_packet.copyInPayload(syncobj.getSerialisationBufferPtr(), syncobj.getSerialisationBufferSize());

  socket.write(rtp_packet.bufferPtr(), rtp_packet.usedBufferSize());

  do {
    num = fread((void*)rtp_packet.payloadBufferPtr(), rtp_packet.payloadBufferSize(), 1, in_fd); 
    num *= rtp_packet.payloadBufferSize();
    rtp_packet.usedPayloadBufferSize(num);

    payload_duration_in_us = (num * 1000000) / audio_bytes_per_second;

    rtp_packet.seqNum( rtp_packet.seqNum() + 1 ); 
    rtp_packet.timestamp( rtp_packet.timestamp() + num / 4);
    rtp_packet.payloadType(PAYLOAD_PCM);

//    measure the quality of the below usleep calculation
//    now = microsec_clock::local_time();
//    interval = now - m_last_send_time;
//    cerr << "interval: " << interval << endl;

    socket.write(rtp_packet.bufferPtr(), rtp_packet.usedBufferSize());

//    m_last_send_time = now;    

    if(m_last_payload_duration_in_us == 0) {
      // if this is the first packet, assume the "last" with same duration 
      // for sleep calculation below. 
      
      cerr << " CStreamSource::readAndSend: ============ STREAM RESET! =============" << endl;
      m_last_payload_duration_in_us = payload_duration_in_us;
      time_since_last_packet_in_us = m_last_payload_duration_in_us;

      gettimeofday(&m_send_time, NULL);
      m_first_send_time = m_send_time.tv_sec * 1000000 + m_send_time.tv_usec;
      send_time = m_first_send_time;
      
      m_payload_duration_sum = payload_duration_in_us;
      // cerr << " payload_duration sum = " << m_payload_duration_sum;

    }
    else {
      gettimeofday(&m_send_time, NULL);
      send_time = m_send_time.tv_sec * 1000000 + m_send_time.tv_usec;
                       
      m_payload_duration_sum += payload_duration_in_us;                  
      
      // cerr << " payload_duration sum=" << m_payload_duration_sum;
                       
    }

    total_play_time = send_time - m_first_send_time;
    // cerr << " total play time=" << total_play_time;

    long sleep_duration_in_us = m_payload_duration_sum - total_play_time;
    
    // cerr << " -> sleeping " << sleep_duration_in_us << " us before sending next packet." << endl;
    
    // only sleep, if there is time to sleep. especially on negative values, we are much too late, so don't sleep at all.
    if(sleep_duration_in_us > 0)
      usleep(sleep_duration_in_us);
    else {
      cerr << " Sleep duration was < 0!" << endl;
    }

    m_last_payload_duration_in_us = payload_duration_in_us;
  
  } while(num != 0);
  
  
  
  fclose(in_fd);
  return EXIT_SUCCESS;

}

