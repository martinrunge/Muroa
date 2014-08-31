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
#include <iostream>
#include <assert.h>

#include "cfixpointresampler.h"
// #include "caudioframe.h"
// #include "cringbuffer.h"

/*
 * The configuration constants below govern
 * the number of bits in the input sample and filter coefficients, the
 * number of bits to the right of the binary-point for fixed-point math, etc.
 */

/* Conversion constants */
#define Nhc       8
#define Na        7
#define Np       (Nhc+Na)
#define Npc      (1<<Nhc)
#define Amask    ((1<<Na)-1)
#define Pmask    ((1<<Np)-1)
#define Nh       16
#define Nb       16
#define Nhxn     14
#define Nhg      (Nh-Nhxn)
#define NLpScl   13
/* Description of constants:
 *
 * Npc - is the number of look-up values available for the lowpass filter
 *    between the beginning of its impulse response and the "cutoff time"
 *    of the filter.  The cutoff time is defined as the reciprocal of the
 *    lowpass-filter cut off frequence in Hz.  For example, if the
 *    lowpass filter were a sinc function, Npc would be the index of the
 *    impulse-response lookup-table corresponding to the first zero-
 *    crossing of the sinc function.  (The inverse first zero-crossing
 *    time of a sinc function equals its nominal cutoff frequency in Hz.)
 *    Npc must be a power of 2 due to the details of the current
 *    implementation. The default value of 512 is sufficiently high that
 *    using linear interpolation to fill in between the table entries
 *    gives approximately 16-bit accuracy in filter coefficients.
 *
 * Nhc - is log base 2 of Npc.
 *
 * Na - is the number of bits devoted to linear interpolation of the
 *    filter coefficients.
 *
 * Np - is Na + Nhc, the number of bits to the right of the binary point
 *    in the integer "time" variable. To the left of the point, it indexes
 *    the input array (X), and to the right, it is interpreted as a number
 *    between 0 and 1 sample of the input X.  Np must be less than 16 in
 *    this implementation.
 *
 * Nh - is the number of bits in the filter coefficients. The sum of Nh and
 *    the number of bits in the input data (typically 16) cannot exceed 32.
 *    Thus Nh should be 16.  The largest filter coefficient should nearly
 *    fill 16 bits (32767).
 *
 * Nb - is the number of bits in the input data. The sum of Nb and Nh cannot
 *    exceed 32.
 *
 * Nhxn - is the number of bits to right shift after multiplying each input
 *    sample times a filter coefficient. It can be as great as Nh and as
 *    small as 0. Nhxn = Nh-2 gives 2 guard bits in the multiply-add
 *    accumulation.  If Nhxn=0, the accumulation will soon overflow 32 bits.
 *
 * Nhg - is the number of guard bits in mpy-add accumulation (equal to Nh-Nhxn)
 *
 * NLpScl - is the number of bits allocated to the unity-gain normalization
 *    factor.  The output of the lowpass filter is multiplied by LpScl and
 *    then right-shifted NLpScl bits. To avoid overflow, we must have
 *    Nb+Nhg+NLpScl < 32.
 */

// two sets of filters coefficients:
#include "CFixPointResamplerLargeFilter.h"
#include "CFixPointResamplerSmallFilter.h"

#include <cstring>
#include <climits>


using namespace std;

CFixPointResampler::CFixPointResampler(int16_t *outbuffer_i16,
                                       int out_buffer_size,
                                       enum converter_quality quality,
                                       int num_channels) :
  m_large_filter(LARGE_FILTER_NMULT, LARGE_FILTER_SCALE, LARGE_FILTER_NWING, LARGE_FILTER_IMP, LARGE_FILTER_IMPD),
  m_small_filter(SMALL_FILTER_NMULT, SMALL_FILTER_SCALE, SMALL_FILTER_NWING, SMALL_FILTER_IMP, SMALL_FILTER_IMPD)
{
  int error;
  
  m_outbuffer = outbuffer_i16;
  m_outbuffer_size = out_buffer_size;

  m_quality = quality;

  m_X = 0;  // used to see if it can be deleted
  m_X_read = 0;
  m_X_write = 0;
  m_Y_read = 0;
  m_Y_write = 0;


  m_max_factor = 1.2;
  m_min_factor = 0.8;

  m_X_size = 4*8192 + 256;
  // m_X_size = 882000;

  m_volume = 1.0;
  m_interp_filter = true;

  m_num_channels = num_channels;

//  m_size_of_short_multichannel_sample = m_num_channels * sizeof(short);
  m_size_of_input_singlechannel_sample = sizeof(short);
  m_size_of_input_multichannel_sample = m_size_of_input_singlechannel_sample * m_num_channels;

  init(m_num_channels, m_volume);
 
  //m_debug_fd1 = fopen("resampler_input.raw", "w");
  //m_debug_fd2 = fopen("resampler_output.raw", "w");
  /* m_data_ptr = new char[4096];*/

  // m_resampled_frame = new CAudioFrame(PAYLOAD_PCM, 3 * 1024);

}


CFixPointResampler::~CFixPointResampler()
{
  // delete m_resampled_frame;

  //fclose(m_debug_fd1);
  //fclose(m_debug_fd2);
  /* delete [] m_data_ptr;*/

  deleteBuffers();
}

void CFixPointResampler::init(int num_channels, double volume) {

  int i;

   // Delete all previous allocated input and output buffer memory
  deleteBuffers();

  m_num_channels = num_channels;
  m_initial = true;
  m_volume = volume;

   /// TODO: make filter size configurable
  //m_filter = &m_small_filter;
  m_filter = &m_large_filter;

   // set initial m_X_read:
   // at the beginning of each m_X, there must be at least as many 
   // silence samples before the first input sample, that the windowfilter can
   // iterate through past samples.

  m_max_windowfilter_range = m_filter->nWing() / Npc + 16;   // 


   // Allocate all new memory
  m_X = new int16_t * [m_num_channels];
  m_Y = new int16_t * [m_num_channels];

  m_X_read = new int[m_num_channels]; 
  m_X_write = new int[m_num_channels]; 
  m_Y_read = new int[m_num_channels]; 
  m_Y_write = new int[m_num_channels]; 

  for (int ch = 0; ch < m_num_channels; ch++) {
      // Add extra to allow of offset of input data (Xoff in main routine)
    m_X[ch] = new int16_t[m_X_size];
    m_Y[ch] = new int16_t[(int)(((double)m_X_size) * m_max_factor)];
    memset(m_X[ch], 0, m_X_size * sizeof(int16_t));    
    
    m_X_read[ch] = m_max_windowfilter_range;
    m_X_write[ch] = m_X_read[ch];
  
    m_Y_read[ch] = 0;
    m_Y_write[ch] = 0;

  }

  m_time = m_X_read[0] << Np;

  cerr << "CFixPointResampler::init: m_max_windowfilter_range=" << m_max_windowfilter_range << endl;

}

void CFixPointResampler::deleteBuffers() {
  
  // Delete memory for the input and output arrays
  if (m_X != 0) {
    for (int ch = 0; ch < m_num_channels; ch++) {
      delete [] m_X[ch];
      m_X[ch] = 0;

      delete [] m_Y[ch];
      m_Y[ch] = 0;
    }
    delete [] m_X;
    m_X = 0;
    delete [] m_Y;
    m_Y = 0;
  }

  if(m_X_read != 0) {
    delete [] m_X_read;
    m_X_read = 0;
  }

  if(m_X_write != 0) {
    delete [] m_X_write;
    m_X_write = 0;
  }

  if(m_Y_read != 0) {
    delete [] m_Y_read;
    m_Y_read = 0;
  }

  if(m_Y_write != 0) {
    delete [] m_Y_write;
    m_Y_write = 0;
  }

  
}

int CFixPointResampler::resampleFrame(int16_t *inbuffer_i16, int num_frames, int16_t* outbuffer, double factor)
{
  addSamplesInX(inbuffer_i16, num_frames);
  // now the samples are in the m_X[] arrays devided by channels
  
  // now do the actual resampling
  resample(factor);
  // resampled samples are in m_Y buffers, beginning at index m_Y_read[ch]
  // until m_Y_write[ch] (not included)
  
  
  int num_frames_ret = copyResampledFramesToOutBuffer(outbuffer);
  // cerr << "CFixPointResampler::resampleFrame copied " << num_frames << " frames to ringbuffer." << endl;
  
  return num_frames_ret;
}

/**
\brief copy input instream into buffers m_X and deinterlace them

n-channel input stream is expected to be interlaced. Order in input array is:
1. sample of 1. channel, 
1. sample of 2. channel ... 
1. sample of n-th channel, 
2. sample of 1. channel, 
2. sample of 2. channel ... 
2. sample of n-th channel
.... and so on. For further use in resamper it is deinterlaces by channels so that
m_X[0] holds all samples of channel 1, 
m_X[1] holds all samples of channel 2 ....

Also, there is always an amount of old samples kept at the beginning of the buffers, 
so when resampling the windowfilter can access at least m_max_windowfilter_range 
older samples.
*/

void CFixPointResampler::addSamplesInX(int16_t* inbuffer, int num_frames) {
  
  assert(1 == m_num_channels);  // do performance and quality tests with just one channel
  //assert(in_frame->sizeofSample() == sizeof(int16_t));
  
  // int16_t* inbuffer = reinterpret_cast<int16_t*>(in_frame->dataPtr());
  int inbuffer_size_in_frames = num_frames;

  int tmp = m_X_write[0];
  
  //fwrite(inbuffer, sizeof(int16_t), inbuffer_size_in_frames * m_num_channels, m_debug_fd1);
  
  for (int ch = 0; ch < m_num_channels; ch++) {  // for each channel
    // make sure that there is still enough space left in buffer m_X[ch]
    if(!(inbuffer_size_in_frames <= (m_X_size - m_X_write[ch]))) {
      cerr << "assertation failed!" << endl;
    } 
    
    // deinterlace channels in input data into different m_X buffers
    for(int i = 0; i < inbuffer_size_in_frames; i++) {
      m_X[ch][m_X_write[ch]] = inbuffer[i * m_num_channels + ch];  	
      m_X_write[ch]++;
      //if(ch==0) fwrite(&inbuffer[i * m_num_channels + ch], sizeof(int16_t), 1, m_debug_fd1);	

    }
  }
  // cerr << " added " << inbuffer_size_in_frames << "samples from " << tmp << " to " << m_X_write[0] << endl;
  
}


/**
\brief copy output data into output ringbuffer, interlace them again

resampled data from the per channel buffers m_Y[ch] is copied
into the ringbuffer, interlacing them again.

The index variables m_Y_read and m_Y_write are updated.

\return number of samples per channel copied. Should always be equal to m_Y[0].
*/

int CFixPointResampler::copyResampledFramesToOutBuffer(int16_t* outbuffer)
{
  
  int frames_copied = m_Y_write[0] - m_Y_read[0];
  memcpy(outbuffer, m_Y[0] + m_Y_read[0], frames_copied * sizeof(int16_t) );
  // int frames_copied = m_ringbuffer->write(m_Y, m_Y_read[0], m_Y_write[0]);
    
  //for(int i= m_Y_read[0]; i <  m_Y_write[0]; i++) {
  //  fwrite(&m_Y[0][i], sizeof(int16_t), 1, m_debug_fd2); 
  //}
  
  
  if(frames_copied != m_Y_write[0] - m_Y_read[0] ) {
    cerr << "CFixPointResampler::copyResampledFramesToRingbuffer(): not all samples copied into ringbuffer!" << endl;  
  }
  
  for(int ch = 0; ch < m_num_channels; ch++) {
    m_Y_write[ch] = 0;
    m_Y_read[ch] = 0;
  }
  
  /*fwrite(m_data_ptr, 2, num_single_channel_samples, m_debug_fd1);*/
  /* m_resampled_frame->dataSizeAdded(num_single_channel_samples * sizeof(short));*/

  return frames_copied;
}






int CFixPointResampler::reset() {
  return 0;
}





int CFixPointResampler::sizeInFrames() {
  return 0;
}



/*!
    \fn CFixPointResampler::resample()
 */
void CFixPointResampler::resample(float factor)
{
  long time;
  switch(m_quality) {

    case poor:
      for(int ch = 0; ch < m_num_channels; ch++) {
        time = resampleChannelLinear(factor, ch, m_time);
      }
      m_time = time;
      break;

    case medium:
      for(int ch = 0; ch < m_num_channels; ch++) {
        //resampleChannelFilter(factor, ch);
      }
      break;

    case high:
      for(int ch = 0; ch < m_num_channels; ch++) {
        // resampleChannel(factor, ch);
      }
      break;

    case best:
      for(int ch = 0; ch < m_num_channels; ch++) {
        time = resampleChannelWithFilter(factor, ch, m_time);
      }
      m_time = time;
      break;

    default:
      break;
  }
}


/*!
    \fn CFixPointResampler::resampleChannelLinear(float factor, int channel)
 */
long CFixPointResampler::resampleChannelLinear(float factor, int ch, long time)
{
  double dt = 1.0/factor;                     /* Step through input signal */ 
  long timestep = (long)(dt*(1<<Np) + 0.5);   /* Fixed-point version of Dt */
  long end_time;                              /* When m_time reaches end_time, all 
                                                 samples in inpuit buffers are used. Return
                                                 to main loop to get new samples.*/

  long fact_part;
  long output_sample;

  long x1, x2;

  int samples_produces = 0;

  int16_t* input_sample_ptr;
  int16_t* output_sample_ptr = &m_Y[ch][m_Y_write[ch]];
 
  int num_input_samples = m_X_write[ch] - m_X_read[ch] - 1;  // -1 because we need two samples to interpolate.
                                                             // In the loop below, the actual input sample 
                                                             // and the following sample are used.
  end_time = time + (1<<Np) * num_input_samples;

  while (time < end_time) {

        fact_part = time & Pmask;
        input_sample_ptr = m_X[ch] + (time >> Np);    /* ptr to current input sample */
        x1 = *input_sample_ptr;
        input_sample_ptr++;
        x2 = *input_sample_ptr;

        x1 *= ((1<<Np) - fact_part);
        x2 *= fact_part;

        output_sample = x1 + x2;
        *output_sample_ptr++ = int32toint16(output_sample, Np);     /* Deposit output */
        time += timestep;           /* Move to next sample by time increment */
        samples_produces++;

  }

  long last_time_used = time - timestep;
  long last_input_sample_used = (last_time_used >> Np) + 1;  // this sample was the second 
                                                             // used to interpolate the last output 
                                                             // sample generated. It will be needed again 
                                                             // for interpolating the next output sample.


  m_Y_read[ch] += samples_produces;
  m_Y_write[ch] += samples_produces;

  
  // copy samples to reuse to beginning of buffer and adjust time variable accordingly
  // cerr << "[" << ch << "]: last input sample used: " << last_input_sample_used << " m_X_write " << m_X_write[ch] << endl;
  int i, j;
  for(i = last_input_sample_used, j = 0; i < m_X_write[ch]; i++, j++) {
    m_X[ch][j] = m_X[ch][i];
  }
  m_X_read[ch] = 1;
  m_X_write[ch] = j;
  time -= (last_input_sample_used) << Np;
 
  // cerr << "[" << ch << "]: m_X_read: " << m_X_read[ch] << " m_X_write " << m_X_write[ch] << endl;

  return time;
}

#ifdef DEBUG
static int pof = 0;             /* positive overflow count */
static int nof = 0;             /* negative overflow count */
#endif


/*!
    \fn CFixPointResampler::resampleChannelWithFilter(float factor, int ch, long time, int filter)
 */
long CFixPointResampler::resampleChannelWithFilter(float factor, int ch, long time)
{
  double dt = 1.0/factor;                     /* Step through input signal */ 
  long timestep = (long)(dt*(1<<Np) + 0.5);   /* Fixed-point version of Dt */
  long end_time;                              /* When m_time reaches end_time, all 
                                                 samples in inpuit buffers are used. Return
                                                 to main loop to get new samples.*/
  long start_time = time;
  
  
  long fract_part;
  long output_sample;

  int v;

  int samples_produced = 0;
  int time_shifted = 0;

  
  int16_t* input_sample_ptr;
  int16_t* output_sample_ptr = &m_Y[ch][m_Y_write[ch]];
 
  int num_input_samples = m_X_write[ch] - m_X_read[ch] - m_max_windowfilter_range;  // lea because we need two samples to interpolate.
                                                             // In the loop below, the actual input sample 
                                                             // and the following sample are used.
  end_time = time + (1<<Np) * num_input_samples;

  double dh = MIN(Npc, factor*Npc);  /* Filter sampling period */
  unsigned dhb = (unsigned)(dh*(1<<Na) + 0.5);       /* Fixed-point representation */

  
  while (time < end_time) {

//        fract_part = time & Pmask;
        time_shifted = (time >> Np);
        input_sample_ptr = m_X[ch] + time_shifted;    /* ptr to current input sample */
        
//        if(ch==0) fwrite(input_sample_ptr, sizeof(int16_t), 1, m_debug_fd1);
        
        if (factor >= 1) { 
          v = FilterUp( input_sample_ptr, (int)(time&Pmask),-1);
          /* Perform right-wing inner product */
          v += FilterUp( input_sample_ptr + 1, (int)((((time)^Pmask)+1)&Pmask),1);

        }
        else {
          
          v = FilterDown(input_sample_ptr, (int)(time&Pmask) ,-1 , dhb);
          /* Perform right-wing inner product */
          v += FilterDown( input_sample_ptr + 1, (int)((((time)^Pmask)+1)&Pmask) ,1 , dhb);
        
        }

        v >>= Nhg;              /* Make guard bits */
        //v *= LpScl;             /* Normalize for unity filter gain */

        //*output_sample_ptr++ = int32toint16(v,NLpScl);   /* strip guard bits, deposit output */
        *output_sample_ptr++ = v;   /* strip guard bits, deposit output */
        time += timestep;           /* Move to next sample by time increment */
        samples_produced++;

  }

  long last_time_used = time; // - timestep;
  long last_input_sample_used = (last_time_used >> Np);      // this sample was the second 
                                                             // used to interpolate the last output 
                                                             // sample generated. It will be needed again 
                                                             // for interpolating the next output sample.

  long num_input_samples_used = last_input_sample_used - (start_time >> Np);
  
  m_Y_write[ch] += samples_produced;

  
  // copy samples to reuse to beginning of buffer and adjust time variable accordingly
  // if(ch==0)
  //  cerr << "[" << ch << "]: num_input_samples = " << num_input_samples << " m_X_read = " << m_X_read[ch] << " last input sample used = " << last_input_sample_used << " m_X_write = " << m_X_write[ch] << endl;

  int i, j;

  assert(last_input_sample_used >= m_max_windowfilter_range);
  assert(last_input_sample_used - m_max_windowfilter_range < m_X_write[ch]);
  
                                                         // -1
  for(i = last_input_sample_used - m_max_windowfilter_range, j = 0; i < m_X_write[ch]; i++, j++) {
    m_X[ch][j] = m_X[ch][i];
  }
  m_X_read[ch] = m_max_windowfilter_range;
  m_X_write[ch] = j;
  time -= (num_input_samples_used) << Np;
 
  // if(ch==0)
  //  cerr << "[" << ch << "]: m_X_read: " << m_X_read[ch] << " m_X_write " << m_X_write[ch] << endl;

  return time;
}


/*!
    \fn CFixPointResampler::int32toint16(int32_t int32var, int n_fix_point_bits)
 */
int16_t CFixPointResampler::int32toint16(int32_t int32var, int n_fix_point_bits)
{
    int16_t out;
    int32_t llsb = (1<<(n_fix_point_bits - 1));
    int32var += llsb;          /* round */
    int32var >>= n_fix_point_bits;
    if (int32var>SHRT_MAX) {
#ifdef DEBUG
        if (pof == 0)
          fprintf(stderr, "*** resample: sound sample overflow\n");
        else if ((pof % 10000) == 0)
          fprintf(stderr, "*** resample: another ten thousand overflows\n");
        pof++;
#endif
        int32var = SHRT_MAX;
    } else if (int32var < SHRT_MIN) {
#ifdef DEBUG
        if (nof == 0)
          fprintf(stderr, "*** resample: sound sample (-) overflow\n");
        else if ((nof % 1000) == 0)
          fprintf(stderr, "*** resample: another thousand (-) overflows\n");
        nof++;
#endif
        int32var = SHRT_MIN;
    }   
    out = (int16_t) int32var;
    return out;
}




int CFixPointResampler::FilterUp(int16_t *Xp, int16_t Ph, int Inc)
{
  const int16_t *Hp, *Hdp = 0, *End;
  int16_t a = 0;
  int v, t;
    
  v=0;
  Hp = &(m_filter->m_imp[Ph>>Na]);
  End = &(m_filter->m_imp[m_filter->nWing()]);
  if (m_interp_filter == true) 
  {
    Hdp = &(m_filter->m_imp_d[Ph>>Na]);
    a = Ph & Amask;
  }
  if (Inc == 1)		/* If doing right wing...              */
  {				/* ...drop extra coeff, so when Ph is  */
    End--;			/*    0.5, we don't do too many mult's */
    if (Ph == 0)		/* If the phase is zero...           */
    {			/* ...then we've already skipped the */
      Hp += Npc;		/*    first sample, so we must also  */
      Hdp += Npc;		/*    skip ahead in Imp[] and ImpD[] */
    }
  }
  if (m_interp_filter == true)
  {
    while (Hp < End) 
    {
      t = *Hp;		/* Get filter coeff */
      t += (((int)*Hdp)*a)>>Na; /* t is now interp'd filter coeff */
      Hdp += Npc;		/* Filter coeff differences step */
      t *= *Xp;		/* Mult coeff by input sample */
      if (t & (1<<(Nhxn-1)))  /* Round, if needed */
      t += (1<<(Nhxn-1));
      t >>= Nhxn;		/* Leave some guard bits, but come back some */
      v += t;			/* The filter output */
      Hp += Npc;		/* Filter coeff step */
      Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
    }
  } 
  else
  { 
    while (Hp < End) 
    {
      t = *Hp;		/* Get filter coeff */
      t *= *Xp;		/* Mult coeff by input sample */
      if (t & (1<<(Nhxn-1)))   /* Round, if needed */
      {
        t += (1<<(Nhxn-1));
      }
      t >>= Nhxn;		/* Leave some guard bits, but come back some */
      v += t;			/* The filter output */
      Hp += Npc;		/* Filter coeff step */
      Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
    }
  }
  
  return(v);
}

int CFixPointResampler::FilterDown(int16_t *Xp, int16_t Ph, int Inc, uint16_t dhb)
{
  int16_t a;
  const int16_t *Hp, *Hdp, *End;
  int v, t;
  unsigned Ho;
    
  v=0;
  Ho = (Ph*(unsigned)dhb)>>Np;
  End = &(m_filter->m_imp[m_filter->nWing()]);
  if (Inc == 1)		        /* If doing right wing...              */
  {				/* ...drop extra coeff, so when Ph is  */
    End--;			/*    0.5, we don't do too many mult's */
    if (Ph == 0)		/* If the phase is zero...           */
      Ho += dhb;		/* ...then we've already skipped the */
  }				/*    first sample, so we must also  */
                                /*    skip ahead in Imp[] and ImpD[] */
   
  if (m_interp_filter == true)
  {
    while ((Hp = &(m_filter->m_imp[Ho>>Na])) < End) 
    {
      t = *Hp;		/* Get IR sample */
      Hdp = &(m_filter->m_imp_d[Ho>>Na]);  /* get interp (lower Na) bits from diff table*/
      a = Ho & Amask;	/* a is logically between 0 and 1 */
      t += (((int)*Hdp)*a)>>Na; /* t is now interp'd filter coeff */
      t *= *Xp;		/* Mult coeff by input sample */
      if (t & 1<<(Nhxn-1))	/* Round, if needed */
        t += 1<<(Nhxn-1);
      t >>= Nhxn;		/* Leave some guard bits, but come back some */
      v += t;			/* The filter output */
      Ho += dhb;		/* IR step */
      Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
    }
  }
  else
  { 
    while ((Hp = &(m_filter->m_imp[Ho>>Na])) < End)
    {
      t = *Hp;		/* Get IR sample */
      t *= *Xp;		/* Mult coeff by input sample */
      if (t & 1<<(Nhxn-1))	/* Round, if needed */
        t += 1<<(Nhxn-1);
      t >>= Nhxn;		/* Leave some guard bits, but come back some */
      v += t;			/* The filter output */
      Ho += dhb;		/* IR step */
      Xp += Inc;		/* Input signal step. NO CHECK ON BOUNDS */
    }
  }
  return(v);
}

