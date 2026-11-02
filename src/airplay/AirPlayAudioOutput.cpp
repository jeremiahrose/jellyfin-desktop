#include "AirPlayAudioOutput.h"

#include <QDebug>
#include <cstdint>
#include <pulse/error.h>

#include "utils/Utils.h"

AirPlayAudioOutput::AirPlayAudioOutput(int channelCount, int sampleRateHz)
{
  pa_sample_spec signed16BitPcmFormat;
  signed16BitPcmFormat.format = PA_SAMPLE_S16NE;
  signed16BitPcmFormat.channels = static_cast<uint8_t>(channelCount);
  signed16BitPcmFormat.rate = static_cast<uint32_t>(sampleRateHz);

  int pulseAudioErrorCode = 0;
  m_pulseAudioPlaybackStream =
  pa_simple_new(nullptr, "Jellyfin Desktop", PA_STREAM_PLAYBACK, nullptr, "AirPlay",
                &signed16BitPcmFormat, nullptr, nullptr, &pulseAudioErrorCode);
  if (!m_pulseAudioPlaybackStream)
    throw FatalException(QString("Could not open a PulseAudio stream for AirPlay: %1")
                         .arg(pa_strerror(pulseAudioErrorCode)));
}

AirPlayAudioOutput::~AirPlayAudioOutput() { pa_simple_free(m_pulseAudioPlaybackStream); }

void AirPlayAudioOutput::playSigned16BitPcmSamples(const void* pcmSamples,
                                                   int pcmSamplesLengthBytes)
{
  int pulseAudioErrorCode = 0;
  if (pa_simple_write(m_pulseAudioPlaybackStream, pcmSamples,
                      static_cast<size_t>(pcmSamplesLengthBytes), &pulseAudioErrorCode) < 0)
    qCritical() << "Writing AirPlay audio to PulseAudio failed:"
                << pa_strerror(pulseAudioErrorCode);
}

void AirPlayAudioOutput::discardBufferedAudio()
{
  int pulseAudioErrorCode = 0;
  if (pa_simple_flush(m_pulseAudioPlaybackStream, &pulseAudioErrorCode) < 0)
    qCritical() << "Flushing AirPlay audio from PulseAudio failed:"
                << pa_strerror(pulseAudioErrorCode);
}
