#ifndef AIRPLAYTRACKMETADATA_H
#define AIRPLAYTRACKMETADATA_H

#include <QByteArray>
#include <QString>

struct AirPlayTrackMetadata
{
  QString title;
  QString artist;
  QString album;

  static AirPlayTrackMetadata fromDmapTaggedData(const QByteArray& dmapTaggedData);
};

#endif
