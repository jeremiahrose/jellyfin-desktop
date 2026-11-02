#ifndef AIRPLAYAUDIOOUTPUT_H
#define AIRPLAYAUDIOOUTPUT_H

#include <pulse/simple.h>

class AirPlayAudioOutput
{
public:
  AirPlayAudioOutput(int channelCount, int sampleRateHz);
  ~AirPlayAudioOutput();

  void playSigned16BitPcmSamples(const void* pcmSamples, int pcmSamplesLengthBytes);
  void discardBufferedAudio();

private:
  pa_simple* m_pulseAudioPlaybackStream;
};

#endif
