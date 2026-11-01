#include "AirPlayNetworkAnnouncement.h"

#include <QDebug>
#include <avahi-common/error.h>
#include <avahi-common/strlst.h>

#include "utils/Utils.h"

AirPlayNetworkAnnouncement::AirPlayNetworkAnnouncement(const QString& receiverName,
                                                       const QByteArray& receiverHardwareAddress,
                                                       quint16 airPlayAudioPort)
  : m_airPlayServiceName(receiverHardwareAddress.toHex().toUpper() + "@" + receiverName.toUtf8()),
    m_airPlayAudioPort(airPlayAudioPort),
    m_avahiEventLoop(avahi_threaded_poll_new())
{
  int avahiErrorCode = 0;
  m_avahiClient =
  avahi_client_new(avahi_threaded_poll_get(m_avahiEventLoop), AVAHI_CLIENT_NO_FAIL,
                   &AirPlayNetworkAnnouncement::onAvahiClientStateChanged, this, &avahiErrorCode);
  if (!m_avahiClient)
    throw FatalException(QString("Could not connect to Avahi to announce AirPlay: %1")
                         .arg(avahi_strerror(avahiErrorCode)));

  avahi_threaded_poll_start(m_avahiEventLoop);
}

AirPlayNetworkAnnouncement::~AirPlayNetworkAnnouncement()
{
  avahi_threaded_poll_stop(m_avahiEventLoop);
  avahi_client_free(m_avahiClient);
  avahi_threaded_poll_free(m_avahiEventLoop);
}

void AirPlayNetworkAnnouncement::onAvahiClientStateChanged(AvahiClient* avahiClient,
                                                           AvahiClientState avahiClientState,
                                                           void* airPlayNetworkAnnouncement)
{
  auto* announcement = static_cast<AirPlayNetworkAnnouncement*>(airPlayNetworkAnnouncement);

  if (avahiClientState == AVAHI_CLIENT_S_RUNNING && !announcement->m_airPlayServiceRegistration)
    announcement->registerAirPlayAudioService(avahiClient);
  else if (avahiClientState == AVAHI_CLIENT_FAILURE)
    qCritical() << "Avahi stopped announcing AirPlay:"
                << avahi_strerror(avahi_client_errno(avahiClient));
}

void AirPlayNetworkAnnouncement::onServiceRegistrationStateChanged(
AvahiEntryGroup* serviceRegistration, AvahiEntryGroupState serviceRegistrationState,
void* airPlayNetworkAnnouncement)
{
  auto* announcement = static_cast<AirPlayNetworkAnnouncement*>(airPlayNetworkAnnouncement);

  if (serviceRegistrationState == AVAHI_ENTRY_GROUP_ESTABLISHED)
    qInfo() << "Announcing AirPlay receiver as" << announcement->m_airPlayServiceName;
  else if (serviceRegistrationState == AVAHI_ENTRY_GROUP_COLLISION)
    qCritical() << "Another AirPlay receiver on the network already uses the name"
                << announcement->m_airPlayServiceName;
  else if (serviceRegistrationState == AVAHI_ENTRY_GROUP_FAILURE)
    qCritical() << "Avahi could not announce AirPlay:"
                << avahi_strerror(
                   avahi_client_errno(avahi_entry_group_get_client(serviceRegistration)));
}

void AirPlayNetworkAnnouncement::registerAirPlayAudioService(AvahiClient* avahiClient)
{
  m_airPlayServiceRegistration = avahi_entry_group_new(
  avahiClient, &AirPlayNetworkAnnouncement::onServiceRegistrationStateChanged, this);

  AvahiStringList* airPlayAudioCapabilities = avahi_string_list_new(
  "txtvers=1", "ch=2", "cn=0,1", "et=0,1", "sv=false", "da=true", "sr=44100", "ss=16", "pw=false",
  "vn=3", "tp=TCP,UDP", "md=0,1,2", "vs=130.14", "sm=false", "ek=1", nullptr);

  int avahiErrorCode = avahi_entry_group_add_service_strlst(
  m_airPlayServiceRegistration, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
  static_cast<AvahiPublishFlags>(0), m_airPlayServiceName.constData(), "_raop._tcp", nullptr,
  nullptr, m_airPlayAudioPort, airPlayAudioCapabilities);
  avahi_string_list_free(airPlayAudioCapabilities);

  if (avahiErrorCode < 0)
  {
    qCritical() << "Avahi could not add the AirPlay service:" << avahi_strerror(avahiErrorCode);
    return;
  }

  avahiErrorCode = avahi_entry_group_commit(m_airPlayServiceRegistration);
  if (avahiErrorCode < 0)
    qCritical() << "Avahi could not publish the AirPlay service:" << avahi_strerror(avahiErrorCode);
}
