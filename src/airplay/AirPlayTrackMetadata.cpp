#include "AirPlayTrackMetadata.h"

#include <QtEndian>

static const qint64 DMAP_ITEM_HEADER_LENGTH_BYTES = 8;
static const QByteArray DMAP_LIST_ITEM_CONTAINER_TAG = "mlit";
static const QByteArray DMAP_TRACK_TITLE_TAG = "minm";
static const QByteArray DMAP_TRACK_ARTIST_TAG = "asar";
static const QByteArray DMAP_TRACK_ALBUM_TAG = "asal";

static void copyDmapItemsIntoTrackMetadata(const QByteArray& dmapItems,
                                           AirPlayTrackMetadata& trackMetadata)
{
  qint64 itemStartOffset = 0;
  while (itemStartOffset + DMAP_ITEM_HEADER_LENGTH_BYTES <= dmapItems.size())
  {
    QByteArray itemTag = dmapItems.mid(itemStartOffset, 4);
    qint64 itemValueLengthBytes =
    qFromBigEndian<quint32>(dmapItems.constData() + itemStartOffset + 4);
    QByteArray itemValue =
    dmapItems.mid(itemStartOffset + DMAP_ITEM_HEADER_LENGTH_BYTES, itemValueLengthBytes);

    if (itemTag == DMAP_LIST_ITEM_CONTAINER_TAG)
      copyDmapItemsIntoTrackMetadata(itemValue, trackMetadata);
    else if (itemTag == DMAP_TRACK_TITLE_TAG)
      trackMetadata.title = QString::fromUtf8(itemValue);
    else if (itemTag == DMAP_TRACK_ARTIST_TAG)
      trackMetadata.artist = QString::fromUtf8(itemValue);
    else if (itemTag == DMAP_TRACK_ALBUM_TAG)
      trackMetadata.album = QString::fromUtf8(itemValue);

    itemStartOffset += DMAP_ITEM_HEADER_LENGTH_BYTES + itemValueLengthBytes;
  }
}

AirPlayTrackMetadata AirPlayTrackMetadata::fromDmapTaggedData(const QByteArray& dmapTaggedData)
{
  AirPlayTrackMetadata trackMetadata;
  copyDmapItemsIntoTrackMetadata(dmapTaggedData, trackMetadata);
  return trackMetadata;
}
