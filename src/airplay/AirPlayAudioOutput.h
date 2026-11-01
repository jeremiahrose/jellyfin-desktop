#ifndef AIRPLAYAUDIOOUTPUT_H
#define AIRPLAYAUDIOOUTPUT_H

#include <atomic>
#include <pulse/simple.h>

class AirPlayAudioOutput
{
public:
  AirPlayAudioOutput(int channelCount, int sampleRateHz);
  ~AirPlayAudioOutput();

  void playSigned16BitPcmSamples(const void* pcmSamples, int pcmSamplesLengthBytes);
  void discardBufferedAudio();
  void setVolumeFromAirPlayDecibels(float airPlayVolumeDecibels);

private:
  pa_simple* m_pulseAudioPlaybackStream;
  std::atomic<float> m_linearVolumeMultiplier;
};

#endif
