#ifndef AIRPLAYRECEIVERCOMPONENT_H
#define AIRPLAYRECEIVERCOMPONENT_H

#include <QByteArray>
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
  bool componentExport() override { return false; }
  bool componentInitialize() override;

private:
  static QByteArray readAirportExpressPrivateKeyPem();
  static QByteArray firstNetworkInterfaceHardwareAddress();
  void startAirPlayAudioServer(const QByteArray& receiverHardwareAddress);

  raop_callbacks_t m_airPlayAudioServerCallbacks = {};
  raop_t* m_airPlayAudioServer = nullptr;
  unsigned short m_airPlayAudioPort = 5000;
  std::unique_ptr<AirPlayNetworkAnnouncement> m_networkAnnouncement;
};

#endif
