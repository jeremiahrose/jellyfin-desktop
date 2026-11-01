#ifndef DEFAULTAUDIOOUTPUTVOLUME_H
#define DEFAULTAUDIOOUTPUTVOLUME_H

#include <pulse/pulseaudio.h>

class DefaultAudioOutputVolume
{
public:
  DefaultAudioOutputVolume();
  ~DefaultAudioOutputVolume();

  void raiseVolumeByOneStep();
  void lowerVolumeByOneStep();
  void toggleMute();
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

  void connectToPulseAudio();
  void disconnectFromPulseAudio();
  DefaultAudioOutputState readDefaultAudioOutputState();
  void setDefaultAudioOutputChannelVolumes(const pa_cvolume& channelVolumes);
  void waitForPulseAudioOperation(pa_operation* pulseAudioOperation,
                                  const char* operationDescription);

  pa_threaded_mainloop* m_pulseAudioThread = nullptr;
  pa_context* m_pulseAudioConnection = nullptr;
  DefaultAudioOutputState m_lastReadDefaultAudioOutputState = {};
  bool m_lastOperationSucceeded = false;
};

#endif
