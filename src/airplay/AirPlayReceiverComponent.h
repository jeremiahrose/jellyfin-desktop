#ifndef AIRPLAYRECEIVERCOMPONENT_H
#define AIRPLAYRECEIVERCOMPONENT_H

#include <QByteArray>
#include <atomic>
#include <memory>
#include <shairplay/raop.h>

#include "AirPlayNetworkAnnouncement.h"
#include "ComponentManager.h"

class AirPlayReceiverComponent : public ComponentBase
{
  Q_OBJECT
  DEFINE_SINGLETON(AirPlayReceiverComponent);

public:
  explicit AirPlayReceiverComponent(QObject* parent = nullptr) : ComponentBase(parent) {}
  ~AirPlayReceiverComponent() override;

  const char* componentName() override { return "airplay"; }
  bool componentExport() override { return true; }
  bool componentInitialize() override;
  void componentPostInitialize() override;

  void countAirPlayStreamStarted(int sampleRateHz);
  void countAirPlayStreamEnded();
  void announceAirPlayTrackMetadata(const QByteArray& dmapTaggedTrackMetadata);
  void announceAirPlayProgress(unsigned int trackStartRtpTimestamp,
                               unsigned int currentRtpTimestamp, unsigned int trackEndRtpTimestamp);
  void announceAirPlayCoverArt(const QByteArray& coverArtImage);
  void announceAirPlayAudioArrived();
  void announceAirPlayAudioStoppedFlowing();

Q_SIGNALS:
  void airPlayStreamStarted();
  void airPlayTrackChanged(const QString& title, const QString& artist, const QString& album);
  void airPlayCoverArtChanged(const QString& coverArtMimeType, const QString& coverArtBase64);
  void airPlayProgressChanged(double positionSeconds, double durationSeconds);
  void airPlayAudioStartedFlowing();
  void airPlayAudioStoppedFlowing();
  void airPlayStreamEnded();

public Q_SLOTS:
  void disconnectAirPlaySenders();

private:
  static QByteArray readAirportExpressPrivateKeyPem();
  static QByteArray firstNetworkInterfaceHardwareAddress();
  void createAirPlayAudioServer();
  void startListeningForAirPlaySenders();

  QByteArray m_receiverHardwareAddress;
  std::atomic<int> m_activeAirPlayStreamCount = 0;
  std::atomic<int> m_airPlaySampleRateHz = 44100;
  std::atomic<bool> m_airPlayAudioIsFlowing = false;
  raop_callbacks_t m_airPlayAudioServerCallbacks = {};
  raop_t* m_airPlayAudioServer = nullptr;
  unsigned short m_airPlayAudioPort = 5000;
  std::unique_ptr<AirPlayNetworkAnnouncement> m_networkAnnouncement;
};

#endif
