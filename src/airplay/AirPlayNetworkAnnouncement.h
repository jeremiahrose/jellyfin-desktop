#ifndef AIRPLAYNETWORKANNOUNCEMENT_H
#define AIRPLAYNETWORKANNOUNCEMENT_H

#include <QByteArray>
#include <QString>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/thread-watch.h>

class AirPlayNetworkAnnouncement
{
public:
  AirPlayNetworkAnnouncement(const QString& receiverName, const QByteArray& receiverHardwareAddress,
                             quint16 airPlayAudioPort);
  ~AirPlayNetworkAnnouncement();

private:
  static void onAvahiClientStateChanged(AvahiClient* avahiClient, AvahiClientState avahiClientState,
                                        void* airPlayNetworkAnnouncement);
  static void onServiceRegistrationStateChanged(AvahiEntryGroup* serviceRegistration,
                                                AvahiEntryGroupState serviceRegistrationState,
                                                void* airPlayNetworkAnnouncement);
  void registerAirPlayAudioService(AvahiClient* avahiClient);

  QByteArray m_airPlayServiceName;
  quint16 m_airPlayAudioPort;
  AvahiThreadedPoll* m_avahiEventLoop;
  AvahiClient* m_avahiClient = nullptr;
  AvahiEntryGroup* m_airPlayServiceRegistration = nullptr;
};

#endif
