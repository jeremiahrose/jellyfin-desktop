#ifndef DEFAULTAUDIOOUTPUTVOLUME_H
#define DEFAULTAUDIOOUTPUTVOLUME_H

#include <functional>
#include <mutex>
#include <pulse/pulseaudio.h>

class DefaultAudioOutputVolume
{
public:
  explicit DefaultAudioOutputVolume(std::function<void()> announceDefaultAudioOutputChanged);
  ~DefaultAudioOutputVolume();

  void raiseVolumeByOneStep();
  void lowerVolumeByOneStep();
  void setVolumePercent(int volumePercent);
  void toggleMute();
  void setMuted(bool shouldMute);
  int volumePercent();
  bool isMuted();

private:
  struct DefaultAudioOutputState
  {
    pa_cvolume channelVolumes;
    bool isMuted;
  };

  static void signalPulseAudioThread(pa_context* pulseAudioConnection,
                                     void* defaultAudioOutputVolume);
  static void storeDefaultAudioOutputState(pa_context* pulseAudioConnection,
                                           const pa_sink_info* defaultAudioOutputInfo,
                                           int reachedEndOfList, void* defaultAudioOutputVolume);
  static void storeOperationSuccess(pa_context* pulseAudioConnection, int operationSucceeded,
                                    void* defaultAudioOutputVolume);
  static void announceAudioOutputChange(pa_context* pulseAudioConnection,
                                        pa_subscription_event_type_t changeType,
                                        uint32_t changedAudioOutputIndex,
                                        void* defaultAudioOutputVolume);

  void connectToPulseAudio();
  void disconnectFromPulseAudio();
  DefaultAudioOutputState readDefaultAudioOutputState();
  void setDefaultAudioOutputChannelVolumes(const pa_cvolume& channelVolumes);
  void setDefaultAudioOutputMuted(bool shouldMute);
  void subscribeToAudioOutputChanges();
  void waitForPulseAudioOperation(pa_operation* pulseAudioOperation,
                                  const char* operationDescription);

  std::function<void()> m_announceDefaultAudioOutputChanged;
  std::mutex m_oneVolumeOperationAtATime;
  pa_threaded_mainloop* m_pulseAudioThread = nullptr;
  pa_context* m_pulseAudioConnection = nullptr;
  DefaultAudioOutputState m_lastReadDefaultAudioOutputState = {};
  bool m_lastOperationSucceeded = false;
};

#endif
