#include "AirPlayAudioOutput.h"

#include <QByteArray>
#include <QDebug>
#include <cmath>
#include <cstdint>
#include <pulse/error.h>

#include "utils/Utils.h"

static const float AIRPLAY_MUTED_VOLUME_DECIBELS = -144.0f;

AirPlayAudioOutput::AirPlayAudioOutput(int channelCount, int sampleRateHz)
  : m_linearVolumeMultiplier(1.0f)
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
  QByteArray volumeAdjustedSamples(static_cast<const char*>(pcmSamples), pcmSamplesLengthBytes);
  auto* samples = reinterpret_cast<int16_t*>(volumeAdjustedSamples.data());
  int sampleCount = pcmSamplesLengthBytes / static_cast<int>(sizeof(int16_t));
  float volumeMultiplier = m_linearVolumeMultiplier.load();

  for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
    samples[sampleIndex] = static_cast<int16_t>(samples[sampleIndex] * volumeMultiplier);

  int pulseAudioErrorCode = 0;
  if (pa_simple_write(m_pulseAudioPlaybackStream, volumeAdjustedSamples.constData(),
                      static_cast<size_t>(volumeAdjustedSamples.size()), &pulseAudioErrorCode) < 0)
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

void AirPlayAudioOutput::setVolumeFromAirPlayDecibels(float airPlayVolumeDecibels)
{
  bool senderIsMuted = airPlayVolumeDecibels <= AIRPLAY_MUTED_VOLUME_DECIBELS;
  float linearVolumeMultiplier =
  senderIsMuted ? 0.0f : std::pow(10.0f, airPlayVolumeDecibels / 20.0f);
  m_linearVolumeMultiplier.store(linearVolumeMultiplier);
}
