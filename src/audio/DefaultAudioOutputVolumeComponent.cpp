#include "DefaultAudioOutputVolumeComponent.h"

#include <QDebug>

#include "utils/Utils.h"

bool DefaultAudioOutputVolumeComponent::componentInitialize()
{
  try
  {
    m_defaultAudioOutputVolume = std::make_unique<DefaultAudioOutputVolume>(
    [this]
    {
      QMetaObject::invokeMethod(
      this, [this] { logVolumeControlFailureOf([this] { refreshVolumeFromDefaultAudioOutput(); }); },
      Qt::QueuedConnection);
    });
    refreshVolumeFromDefaultAudioOutput();
    return true;
  }
  catch (FatalException& volumeControlError)
  {
    qCritical() << volumeControlError.message();
    return false;
  }
}

void DefaultAudioOutputVolumeComponent::setVolumePercent(int volumePercent)
{
  logVolumeControlFailureOf([&] { m_defaultAudioOutputVolume->setVolumePercent(volumePercent); });
}

void DefaultAudioOutputVolumeComponent::setMuted(bool shouldMute)
{
  logVolumeControlFailureOf([&] { m_defaultAudioOutputVolume->setMuted(shouldMute); });
}

void DefaultAudioOutputVolumeComponent::raiseVolumeByOneStep()
{
  logVolumeControlFailureOf([&] { m_defaultAudioOutputVolume->raiseVolumeByOneStep(); });
}

void DefaultAudioOutputVolumeComponent::lowerVolumeByOneStep()
{
  logVolumeControlFailureOf([&] { m_defaultAudioOutputVolume->lowerVolumeByOneStep(); });
}

void DefaultAudioOutputVolumeComponent::toggleMute()
{
  logVolumeControlFailureOf([&] { m_defaultAudioOutputVolume->toggleMute(); });
}

int DefaultAudioOutputVolumeComponent::currentVolumePercentOfDefaultAudioOutput()
{
  return m_defaultAudioOutputVolume->volumePercent();
}

bool DefaultAudioOutputVolumeComponent::defaultAudioOutputIsMuted()
{
  return m_defaultAudioOutputVolume->isMuted();
}

void DefaultAudioOutputVolumeComponent::refreshVolumeFromDefaultAudioOutput()
{
  int newVolumePercent = m_defaultAudioOutputVolume->volumePercent();
  bool newIsMuted = m_defaultAudioOutputVolume->isMuted();
  bool volumeHasChanged = newVolumePercent != m_volumePercent || newIsMuted != m_isMuted;
  m_volumePercent = newVolumePercent;
  m_isMuted = newIsMuted;
  if (volumeHasChanged)
    emit volumeChanged();
}

void DefaultAudioOutputVolumeComponent::logVolumeControlFailureOf(
const std::function<void()>& volumeOperation)
{
  try
  {
    volumeOperation();
  }
  catch (FatalException& volumeControlError)
  {
    qCritical() << volumeControlError.message();
  }
}
