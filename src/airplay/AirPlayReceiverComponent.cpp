#include "AirPlayReceiverComponent.h"

#include <QDebug>
#include <QFile>
#include <QHostInfo>
#include <QNetworkInterface>

#include "AirPlayAudioOutput.h"
#include "input/InputComponent.h"
#include "player/PlayerComponent.h"
#include "utils/Utils.h"

static const int MAXIMUM_SIMULTANEOUS_AIRPLAY_SENDERS = 1;

static void* openAudioOutputForNewAirPlayStream(void* receiverComponent, int bitsPerSample,
                                                int channelCount, int sampleRateHz)
{
  if (bitsPerSample != 16)
  {
    qCritical() << "AirPlay sender requested unsupported" << bitsPerSample << "bit audio";
    return nullptr;
  }

  qInfo() << "AirPlay stream started, stopping current playback";
  QMetaObject::invokeMethod(
  &InputComponent::Get(), [] { InputComponent::Get().sendAction("stop"); }, Qt::QueuedConnection);

  try
  {
    auto* airPlayAudioOutput = new AirPlayAudioOutput(channelCount, sampleRateHz);
    static_cast<AirPlayReceiverComponent*>(receiverComponent)->countAirPlayStreamStarted();
    return airPlayAudioOutput;
  }
  catch (FatalException& audioOutputError)
  {
    qCritical() << audioOutputError.message();
    return nullptr;
  }
}

static void playAirPlayAudio(void* receiverComponent, void* airPlayAudioOutput,
                             const void* pcmSamples, int pcmSamplesLengthBytes)
{
  if (airPlayAudioOutput)
    static_cast<AirPlayAudioOutput*>(airPlayAudioOutput)
    ->playSigned16BitPcmSamples(pcmSamples, pcmSamplesLengthBytes);
}

static void discardBufferedAirPlayAudio(void* receiverComponent, void* airPlayAudioOutput)
{
  if (airPlayAudioOutput)
    static_cast<AirPlayAudioOutput*>(airPlayAudioOutput)->discardBufferedAudio();
}

static void setAirPlayVolume(void* receiverComponent, void* airPlayAudioOutput,
                             float airPlayVolumeDecibels)
{
  if (airPlayAudioOutput)
    static_cast<AirPlayAudioOutput*>(airPlayAudioOutput)
    ->setVolumeFromAirPlayDecibels(airPlayVolumeDecibels);
}

static void closeAudioOutputForEndedAirPlayStream(void* receiverComponent, void* airPlayAudioOutput)
{
  qInfo() << "AirPlay stream ended";
  if (airPlayAudioOutput)
    static_cast<AirPlayReceiverComponent*>(receiverComponent)->countAirPlayStreamEnded();
  delete static_cast<AirPlayAudioOutput*>(airPlayAudioOutput);
}

AirPlayReceiverComponent::~AirPlayReceiverComponent()
{
  m_networkAnnouncement.reset();
  if (m_airPlayAudioServer)
  {
    raop_stop(m_airPlayAudioServer);
    raop_destroy(m_airPlayAudioServer);
  }
}

bool AirPlayReceiverComponent::componentInitialize()
{
  try
  {
    m_receiverHardwareAddress = firstNetworkInterfaceHardwareAddress();
    createAirPlayAudioServer();
    startListeningForAirPlaySenders();

    QString receiverName = QString("Jellyfin (%1)").arg(QHostInfo::localHostName());
    m_networkAnnouncement = std::make_unique<AirPlayNetworkAnnouncement>(
    receiverName, m_receiverHardwareAddress, m_airPlayAudioPort);
  }
  catch (FatalException& airPlayStartupError)
  {
    qCritical() << airPlayStartupError.message();
    return false;
  }

  return true;
}

void AirPlayReceiverComponent::componentPostInitialize()
{
  connect(&PlayerComponent::Get(), &PlayerComponent::playing, this,
          &AirPlayReceiverComponent::disconnectAirPlaySendersForJellyfinPlayback);
}

void AirPlayReceiverComponent::disconnectAirPlaySendersForJellyfinPlayback()
{
  if (m_activeAirPlayStreamCount == 0)
    return;

  qInfo() << "Jellyfin playback started, disconnecting AirPlay senders";
  raop_stop(m_airPlayAudioServer);
  try
  {
    startListeningForAirPlaySenders();
  }
  catch (FatalException& airPlayRestartError)
  {
    qCritical() << airPlayRestartError.message();
  }
}

void AirPlayReceiverComponent::createAirPlayAudioServer()
{
  m_airPlayAudioServerCallbacks.cls = this;
  m_airPlayAudioServerCallbacks.audio_init = openAudioOutputForNewAirPlayStream;
  m_airPlayAudioServerCallbacks.audio_process = playAirPlayAudio;
  m_airPlayAudioServerCallbacks.audio_flush = discardBufferedAirPlayAudio;
  m_airPlayAudioServerCallbacks.audio_set_volume = setAirPlayVolume;
  m_airPlayAudioServerCallbacks.audio_destroy = closeAudioOutputForEndedAirPlayStream;

  QByteArray airportExpressPrivateKeyPem = readAirportExpressPrivateKeyPem();
  int airPlayServerErrorCode = 0;
  m_airPlayAudioServer =
  raop_init(MAXIMUM_SIMULTANEOUS_AIRPLAY_SENDERS, &m_airPlayAudioServerCallbacks,
            airportExpressPrivateKeyPem.constData(), &airPlayServerErrorCode);
  if (!m_airPlayAudioServer)
    throw FatalException(
    QString("Could not create the AirPlay audio server, error %1").arg(airPlayServerErrorCode));
}

void AirPlayReceiverComponent::startListeningForAirPlaySenders()
{
  int startResult =
  raop_start(m_airPlayAudioServer, &m_airPlayAudioPort, m_receiverHardwareAddress.constData(),
             m_receiverHardwareAddress.size(), nullptr);
  if (startResult < 0)
    throw FatalException(
    QString("Could not listen for AirPlay audio on port %1").arg(m_airPlayAudioPort));

  qInfo() << "Listening for AirPlay audio on port" << m_airPlayAudioPort;
}

QByteArray AirPlayReceiverComponent::readAirportExpressPrivateKeyPem()
{
  QFile airportExpressPrivateKeyFile(":/airplay/airport-express-private-key.pem");
  if (!airportExpressPrivateKeyFile.open(QIODevice::ReadOnly))
    throw FatalException("Could not read the bundled AirPort Express private key");

  return airportExpressPrivateKeyFile.readAll();
}

QByteArray AirPlayReceiverComponent::firstNetworkInterfaceHardwareAddress()
{
  for (const QNetworkInterface& networkInterface : QNetworkInterface::allInterfaces())
  {
    bool interfaceIsConnected = networkInterface.flags().testFlag(QNetworkInterface::IsUp) &&
                                networkInterface.flags().testFlag(QNetworkInterface::IsRunning);
    bool interfaceIsLoopback = networkInterface.flags().testFlag(QNetworkInterface::IsLoopBack);
    QByteArray hardwareAddress =
    QByteArray::fromHex(networkInterface.hardwareAddress().remove(':').toLatin1());

    if (interfaceIsConnected && !interfaceIsLoopback && hardwareAddress.size() == 6)
      return hardwareAddress;
  }

  throw FatalException(
  "No connected network interface with a hardware address to announce AirPlay on");
}
