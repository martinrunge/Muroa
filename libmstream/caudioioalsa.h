/***************************************************************************
 *   Copyright (C) 2004 by Martin Runge                                    *
 *   martin.runge@web.de                                                   *
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
#ifndef CAUDIOIOALSA_H
#define CAUDIOIOALSA_H



#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include "iaudioio.h"

/**
@author Martin Runge
*/


class CAudioIoAlsa : public IAudioIO
{
public:
    CAudioIoAlsa();

    ~CAudioIoAlsa();

    int start();
    int stop();

    int state();
    int setVolume(int volume);

    int close();
    int closeMixer();
    int open(std::string device, int samplerate, int channels);
    int openMixer(std::string device, std::string channel);

    int read(char* data, int buffersize);
    int write(char* data, int length);

    int getBytesPlayed();
    int getOutOverflows();

    int getDelay();
    int getSpace();

    int getWriteGranularity(void);
    int getActualSampleRate(void);


private:
    
    snd_pcm_t *m_playback_handle;
    snd_pcm_hw_params_t *m_hw_params;
    snd_pcm_sw_params_t *m_sw_params;
    snd_pcm_status_t *m_status_ptr;

    long  m_vol_min, m_vol_max;
    snd_mixer_t *m_mixer_handle;
    snd_mixer_selem_id_t *m_mixer_sid;
    snd_mixer_elem_t* m_mixer_elem;

    int m_write_granularity;     
    int m_sample_rate;
    bool m_open;
  
        
};

#endif
