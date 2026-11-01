#include "DefaultAudioOutputVolume.h"

#include <QString>
#include <cmath>

#include "utils/Utils.h"

static const char* DEFAULT_AUDIO_OUTPUT_NAME = "@DEFAULT_SINK@";
static const pa_volume_t ONE_VOLUME_STEP = PA_VOLUME_NORM / 20;

class PulseAudioThreadLock
{
public:
  explicit PulseAudioThreadLock(pa_threaded_mainloop* pulseAudioThread)
    : m_pulseAudioThread(pulseAudioThread)
  {
    pa_threaded_mainloop_lock(m_pulseAudioThread);
  }
  ~PulseAudioThreadLock() { pa_threaded_mainloop_unlock(m_pulseAudioThread); }

private:
  pa_threaded_mainloop* m_pulseAudioThread;
};

DefaultAudioOutputVolume::DefaultAudioOutputVolume()
{
  try
  {
    connectToPulseAudio();
  }
  catch (FatalException&)
  {
    disconnectFromPulseAudio();
    throw;
  }
}

DefaultAudioOutputVolume::~DefaultAudioOutputVolume() { disconnectFromPulseAudio(); }

void DefaultAudioOutputVolume::raiseVolumeByOneStep()
{
  PulseAudioThreadLock pulseAudioThreadLock(m_pulseAudioThread);
  pa_cvolume channelVolumes = readDefaultAudioOutputState().channelVolumes;
  pa_cvolume_inc_clamp(&channelVolumes, ONE_VOLUME_STEP, PA_VOLUME_NORM);
  setDefaultAudioOutputChannelVolumes(channelVolumes);
}

void DefaultAudioOutputVolume::lowerVolumeByOneStep()
{
  PulseAudioThreadLock pulseAudioThreadLock(m_pulseAudioThread);
  pa_cvolume channelVolumes = readDefaultAudioOutputState().channelVolumes;
  pa_cvolume_dec(&channelVolumes, ONE_VOLUME_STEP);
  setDefaultAudioOutputChannelVolumes(channelVolumes);
}

void DefaultAudioOutputVolume::toggleMute()
{
  PulseAudioThreadLock pulseAudioThreadLock(m_pulseAudioThread);
  bool shouldMute = !readDefaultAudioOutputState().isMuted;
  m_lastOperationSucceeded = false;
  waitForPulseAudioOperation(pa_context_set_sink_mute_by_name(m_pulseAudioConnection,
                                                              DEFAULT_AUDIO_OUTPUT_NAME, shouldMute,
                                                              storeOperationSuccess, this),
                             "mute the default audio output");
}

int DefaultAudioOutputVolume::volumePercent()
{
  PulseAudioThreadLock pulseAudioThreadLock(m_pulseAudioThread);
  pa_cvolume channelVolumes = readDefaultAudioOutputState().channelVolumes;
  pa_volume_t loudestChannelVolume = pa_cvolume_max(&channelVolumes);
  return static_cast<int>(std::lround(100.0 * loudestChannelVolume / PA_VOLUME_NORM));
}

bool DefaultAudioOutputVolume::isMuted()
{
  PulseAudioThreadLock pulseAudioThreadLock(m_pulseAudioThread);
  return readDefaultAudioOutputState().isMuted;
}

void DefaultAudioOutputVolume::connectToPulseAudio()
{
  m_pulseAudioThread = pa_threaded_mainloop_new();
  m_pulseAudioConnection =
  pa_context_new(pa_threaded_mainloop_get_api(m_pulseAudioThread), "Jellyfin Desktop");
  pa_context_set_state_callback(m_pulseAudioConnection, signalPulseAudioThread, this);

  PulseAudioThreadLock pulseAudioThreadLock(m_pulseAudioThread);
  if (pa_threaded_mainloop_start(m_pulseAudioThread) < 0)
    throw FatalException("Could not start the PulseAudio volume control thread");

  if (pa_context_connect(m_pulseAudioConnection, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
    throw FatalException(QString("Could not connect to PulseAudio for volume control: %1")
                         .arg(pa_strerror(pa_context_errno(m_pulseAudioConnection))));

  while (pa_context_get_state(m_pulseAudioConnection) != PA_CONTEXT_READY)
  {
    if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(m_pulseAudioConnection)))
      throw FatalException(QString("PulseAudio volume control connection failed: %1")
                           .arg(pa_strerror(pa_context_errno(m_pulseAudioConnection))));
    pa_threaded_mainloop_wait(m_pulseAudioThread);
  }
}

void DefaultAudioOutputVolume::disconnectFromPulseAudio()
{
  if (m_pulseAudioConnection)
  {
    {
      PulseAudioThreadLock pulseAudioThreadLock(m_pulseAudioThread);
      pa_context_disconnect(m_pulseAudioConnection);
      pa_context_unref(m_pulseAudioConnection);
    }
    m_pulseAudioConnection = nullptr;
  }

  if (m_pulseAudioThread)
  {
    pa_threaded_mainloop_stop(m_pulseAudioThread);
    pa_threaded_mainloop_free(m_pulseAudioThread);
    m_pulseAudioThread = nullptr;
  }
}

DefaultAudioOutputVolume::DefaultAudioOutputState DefaultAudioOutputVolume::
readDefaultAudioOutputState()
{
  m_lastOperationSucceeded = false;
  waitForPulseAudioOperation(pa_context_get_sink_info_by_name(m_pulseAudioConnection,
                                                              DEFAULT_AUDIO_OUTPUT_NAME,
                                                              storeDefaultAudioOutputState, this),
                             "read the default audio output volume");
  return m_lastReadDefaultAudioOutputState;
}

void DefaultAudioOutputVolume::setDefaultAudioOutputChannelVolumes(const pa_cvolume& channelVolumes)
{
  m_lastOperationSucceeded = false;
  waitForPulseAudioOperation(
  pa_context_set_sink_volume_by_name(m_pulseAudioConnection, DEFAULT_AUDIO_OUTPUT_NAME,
                                     &channelVolumes, storeOperationSuccess, this),
  "set the default audio output volume");
}

void DefaultAudioOutputVolume::waitForPulseAudioOperation(pa_operation* pulseAudioOperation,
                                                          const char* operationDescription)
{
  if (!pulseAudioOperation)
    throw FatalException(QString("Could not %1: %2")
                         .arg(operationDescription)
                         .arg(pa_strerror(pa_context_errno(m_pulseAudioConnection))));

  while (pa_operation_get_state(pulseAudioOperation) == PA_OPERATION_RUNNING)
    pa_threaded_mainloop_wait(m_pulseAudioThread);
  pa_operation_unref(pulseAudioOperation);

  if (!m_lastOperationSucceeded)
    throw FatalException(QString("PulseAudio failed to %1: %2")
                         .arg(operationDescription)
                         .arg(pa_strerror(pa_context_errno(m_pulseAudioConnection))));
}

void DefaultAudioOutputVolume::signalPulseAudioThread(pa_context* pulseAudioConnection,
                                                      void* defaultAudioOutputVolume)
{
  auto* volume = static_cast<DefaultAudioOutputVolume*>(defaultAudioOutputVolume);
  pa_threaded_mainloop_signal(volume->m_pulseAudioThread, 0);
}

void DefaultAudioOutputVolume::storeDefaultAudioOutputState(
pa_context* pulseAudioConnection, const pa_sink_info* defaultAudioOutputInfo, int reachedEndOfList,
void* defaultAudioOutputVolume)
{
  auto* volume = static_cast<DefaultAudioOutputVolume*>(defaultAudioOutputVolume);
  if (defaultAudioOutputInfo)
  {
    volume->m_lastReadDefaultAudioOutputState.channelVolumes = defaultAudioOutputInfo->volume;
    volume->m_lastReadDefaultAudioOutputState.isMuted = defaultAudioOutputInfo->mute;
    volume->m_lastOperationSucceeded = true;
  }
  if (reachedEndOfList)
    pa_threaded_mainloop_signal(volume->m_pulseAudioThread, 0);
}

void DefaultAudioOutputVolume::storeOperationSuccess(pa_context* pulseAudioConnection,
                                                     int operationSucceeded,
                                                     void* defaultAudioOutputVolume)
{
  auto* volume = static_cast<DefaultAudioOutputVolume*>(defaultAudioOutputVolume);
  volume->m_lastOperationSucceeded = operationSucceeded;
  pa_threaded_mainloop_signal(volume->m_pulseAudioThread, 0);
}
