#ifndef DEFAULTAUDIOOUTPUTVOLUMECOMPONENT_H
#define DEFAULTAUDIOOUTPUTVOLUMECOMPONENT_H

#include <functional>
#include <memory>

#include "ComponentManager.h"
#include "DefaultAudioOutputVolume.h"

class DefaultAudioOutputVolumeComponent : public ComponentBase
{
  Q_OBJECT
  DEFINE_SINGLETON(DefaultAudioOutputVolumeComponent);
  Q_PROPERTY(int volumePercent READ volumePercent NOTIFY volumeChanged)
  Q_PROPERTY(bool muted READ isMuted NOTIFY volumeChanged)

public:
  explicit DefaultAudioOutputVolumeComponent(QObject* parent = nullptr) : ComponentBase(parent) {}

  const char* componentName() override { return "defaultAudioOutputVolume"; }
  bool componentExport() override { return true; }
  bool componentInitialize() override;

  bool controlsDefaultAudioOutput() const { return m_defaultAudioOutputVolume != nullptr; }
  int volumePercent() const { return m_volumePercent; }
  bool isMuted() const { return m_isMuted; }

  Q_INVOKABLE void setVolumePercent(int volumePercent);
  Q_INVOKABLE void setMuted(bool shouldMute);
  Q_INVOKABLE void raiseVolumeByOneStep();
  Q_INVOKABLE void lowerVolumeByOneStep();
  Q_INVOKABLE void toggleMute();

  int currentVolumePercentOfDefaultAudioOutput();
  bool defaultAudioOutputIsMuted();

Q_SIGNALS:
  void volumeChanged();

private:
  static void logVolumeControlFailureOf(const std::function<void()>& volumeOperation);
  void refreshVolumeFromDefaultAudioOutput();

  std::unique_ptr<DefaultAudioOutputVolume> m_defaultAudioOutputVolume;
  int m_volumePercent = 0;
  bool m_isMuted = false;
};

#endif
