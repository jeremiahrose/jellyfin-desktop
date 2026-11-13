(function() {
const TICKS_PER_MILLISECOND = 10000;
const TRACK_DETAILS_BEFORE_SENDER_DESCRIBES_TRACK = { Title: '', Artist: '', Album: '' };

class airPlayPlayer {
    constructor({ events, playbackManager, ServerConnections }) {
        this.events = events;
        this.playbackManager = playbackManager;
        this.serverConnections = ServerConnections;

        this.name = 'AirPlay';
        this.type = 'mediaplayer';
        this.id = 'airplayplayer';
        this.isLocalPlayer = true;

        this._isStreaming = false;
        this._isPaused = false;
        this._nowPlayingItem = null;
        this._positionMsAtLastProgressUpdate = 0;
        this._lastProgressUpdateTimeMs = 0;
        this._timeUpdateInterval = null;
        this._serverUpdatesInOrder = Promise.resolve();

        window.apiPromise.then((api) => this.followAirPlayStreams(api));
    }

    followAirPlayStreams(api) {
        this._airPlayReceiver = api.airplay;
        this._defaultAudioOutputVolume = api.defaultAudioOutputVolume;
        api.airplay.airPlayStreamStarted.connect(() => this.startShowingAirPlayStream());
        api.airplay.airPlayTrackChanged.connect((title, artist, album) => {
            this.inOrderWithOtherServerUpdates(() => this.showNewAirPlayTrack({ Title: title, Artist: artist, Album: album }));
        });
        api.airplay.airPlayCoverArtChanged.connect((coverArtMimeType, coverArtBase64) => {
            this.inOrderWithOtherServerUpdates(() => this.uploadCoverArt(coverArtMimeType, coverArtBase64));
        });
        api.airplay.airPlayProgressChanged.connect((positionSeconds, durationSeconds) => this.showAirPlayProgress(positionSeconds, durationSeconds));
        api.airplay.airPlayAudioStartedFlowing.connect(() => this.showAirPlayPlaying());
        api.airplay.airPlayAudioStoppedFlowing.connect(() => this.showAirPlayPaused());
        api.airplay.airPlayStreamEnded.connect(() => {
            this.stopShowingAirPlayStream();
            this.inOrderWithOtherServerUpdates(() => this.removeAirPlayTrackFromServer());
        });
    }

    inOrderWithOtherServerUpdates(serverUpdate) {
        this._serverUpdatesInOrder = this._serverUpdatesInOrder
            .then(serverUpdate)
            .catch((serverUpdateError) => console.error('AirPlay now playing update failed', serverUpdateError));
    }

    apiClient() {
        return this.serverConnections.currentApiClient();
    }

    startShowingAirPlayStream() {
        this._isStreaming = true;
        this._isPaused = true;
        this._positionMsAtLastProgressUpdate = 0;
        this._lastProgressUpdateTimeMs = Date.now();
        this._timeUpdateInterval = setInterval(() => this.events.trigger(this, 'timeupdate'), 1000);
        this.inOrderWithOtherServerUpdates(async () => {
            await this.stopOtherPlayback();
            await this.showNewAirPlayTrack(TRACK_DETAILS_BEFORE_SENDER_DESCRIBES_TRACK);
        });
    }

    async stopOtherPlayback() {
        const otherPlayer = this.playbackManager.getCurrentPlayer();
        if (otherPlayer && otherPlayer !== this && this.playbackManager.isPlaying(otherPlayer)) {
            await this.playbackManager.stop(otherPlayer);
        }
    }

    async showNewAirPlayTrack(trackDetails) {
        if (!this._isStreaming) {
            return;
        }

        const apiClient = this.apiClient();
        const airPlayTrackItem = await apiClient.ajax({
            type: 'POST',
            url: apiClient.getUrl('AirPlay/NowPlaying'),
            data: JSON.stringify(trackDetails),
            contentType: 'application/json',
            dataType: 'json'
        });
        const trackItem = await apiClient.getItem(apiClient.getCurrentUserId(), airPlayTrackItem.ItemId);
        if (!this._isStreaming) {
            return;
        }

        if (this._nowPlayingItem) {
            this.events.trigger(this, 'itemstopped', [this.stopInfoForNowPlayingItem('Audio')]);
        }
        this._nowPlayingItem = trackItem;
        this.events.trigger(this, 'itemstarted', [trackItem, this.currentMediaSource()]);
    }

    async uploadCoverArt(coverArtMimeType, coverArtBase64) {
        if (!this._nowPlayingItem) {
            return;
        }

        const coverArtBytes = Uint8Array.from(atob(coverArtBase64), (character) => character.charCodeAt(0));
        const apiClient = this.apiClient();
        const coverArtUploadHeaders = { 'Content-Type': coverArtMimeType };
        apiClient.setRequestHeaders(coverArtUploadHeaders);
        const coverArtUploadResponse = await fetch(apiClient.getUrl('AirPlay/NowPlaying/CoverArt'), {
            method: 'POST',
            headers: coverArtUploadHeaders,
            body: coverArtBytes
        });
        if (!coverArtUploadResponse.ok) {
            throw new Error(`Cover art upload failed with status ${coverArtUploadResponse.status}`);
        }
        this._nowPlayingItem = await apiClient.getItem(apiClient.getCurrentUserId(), this._nowPlayingItem.Id);
        this.events.trigger(this, 'statechange', [this.getPlayerState()]);
    }

    async uploadDuration(durationSeconds) {
        const apiClient = this.apiClient();
        await apiClient.ajax({
            type: 'POST',
            url: apiClient.getUrl('AirPlay/NowPlaying/Duration'),
            data: JSON.stringify({ DurationSeconds: durationSeconds }),
            contentType: 'application/json'
        });
    }

    showAirPlayProgress(positionSeconds, durationSeconds) {
        if (!this._isStreaming) {
            return;
        }

        this._positionMsAtLastProgressUpdate = positionSeconds * 1000;
        this._lastProgressUpdateTimeMs = Date.now();

        const durationTicks = Math.round(durationSeconds * 1000 * TICKS_PER_MILLISECOND);
        const durationHasChanged = this._nowPlayingItem && this._nowPlayingItem.RunTimeTicks !== durationTicks;
        if (durationHasChanged) {
            this._nowPlayingItem.RunTimeTicks = durationTicks;
            this.inOrderWithOtherServerUpdates(() => this.uploadDuration(durationSeconds));
        }
        this.events.trigger(this, 'statechange', [this.getPlayerState()]);
    }

    showAirPlayPlaying() {
        if (!this._isStreaming || !this._isPaused) {
            return;
        }
        this._lastProgressUpdateTimeMs = Date.now();
        this._isPaused = false;
        this.events.trigger(this, 'unpause');
    }

    showAirPlayPaused() {
        if (!this._isStreaming || this._isPaused) {
            return;
        }
        this._positionMsAtLastProgressUpdate = this.currentTime();
        this._isPaused = true;
        this.events.trigger(this, 'pause');
    }

    stopShowingAirPlayStream() {
        if (!this._isStreaming) {
            return;
        }

        const stopInfo = this.stopInfoForNowPlayingItem(null);
        this._isStreaming = false;
        clearInterval(this._timeUpdateInterval);
        this._timeUpdateInterval = null;

        if (this._nowPlayingItem) {
            this.events.trigger(this, 'itemstopped', [stopInfo]);
            this._nowPlayingItem = null;
        }
    }

    async removeAirPlayTrackFromServer() {
        const apiClient = this.apiClient();
        await apiClient.ajax({ type: 'DELETE', url: apiClient.getUrl('AirPlay/NowPlaying') });
    }

    stopInfoForNowPlayingItem(nextMediaType) {
        return {
            item: this._nowPlayingItem,
            mediaSource: this.currentMediaSource(),
            positionMs: this.currentTime(),
            nextItem: null,
            nextMediaType: nextMediaType
        };
    }

    getPlayerState() {
        return {
            PlayState: {
                IsPaused: this._isPaused,
                PositionTicks: this.currentTime() * TICKS_PER_MILLISECOND,
                CanSeek: false,
                IsMuted: this.isMuted(),
                VolumeLevel: this.getVolume(),
                RepeatMode: 'RepeatNone',
                ShuffleMode: 'Sorted',
                PlayMethod: 'DirectPlay',
                MediaSourceId: this._nowPlayingItem?.Id
            },
            NowPlayingItem: this._nowPlayingItem,
            MediaSource: this.currentMediaSource()
        };
    }

    currentItem() {
        return this._nowPlayingItem;
    }

    currentMediaSource() {
        if (!this._nowPlayingItem) {
            return null;
        }
        return {
            Id: this._nowPlayingItem.Id,
            RunTimeTicks: this._nowPlayingItem.RunTimeTicks,
            MediaStreams: []
        };
    }

    canPlayMediaType() {
        return false;
    }

    canPlayItem() {
        return false;
    }

    isPlaying(mediaType) {
        return this._isStreaming && (!mediaType || mediaType === 'Audio');
    }

    currentSrc() {
        return this._isStreaming ? 'airplay' : null;
    }

    currentTime() {
        if (!this._isStreaming) {
            return 0;
        }
        if (this._isPaused) {
            return this._positionMsAtLastProgressUpdate;
        }
        const msSinceLastProgressUpdate = Date.now() - this._lastProgressUpdateTimeMs;
        return this._positionMsAtLastProgressUpdate + msSinceLastProgressUpdate;
    }

    duration() {
        const durationTicks = this._nowPlayingItem?.RunTimeTicks;
        return durationTicks ? durationTicks / TICKS_PER_MILLISECOND : null;
    }

    stop() {
        if (this._isStreaming) {
            this._airPlayReceiver.disconnectAirPlaySenders();
            this.stopShowingAirPlayStream();
        }
        return Promise.resolve();
    }

    destroy() {
    }

    paused() {
        return this._isPaused;
    }

    pause() {
    }

    unpause() {
    }

    seekable() {
        return false;
    }

    getBufferedRanges() {
        return [];
    }

    getVolume() {
        return this._defaultAudioOutputVolume.volumePercent;
    }

    setVolume(volumePercent) {
        this._defaultAudioOutputVolume.setVolumePercent(Math.round(Number(volumePercent)));
    }

    volumeUp() {
        this._defaultAudioOutputVolume.raiseVolumeByOneStep();
    }

    volumeDown() {
        this._defaultAudioOutputVolume.lowerVolumeByOneStep();
    }

    isMuted() {
        return this._defaultAudioOutputVolume.muted;
    }

    setMute(mute) {
        this._defaultAudioOutputVolume.setMuted(mute);
    }

    getPlaybackRate() {
        return 1;
    }

    getPlaylist() {
        return Promise.resolve(this.getPlaylistSync());
    }

    getPlaylistSync() {
        return this._nowPlayingItem ? [this._nowPlayingItem] : [];
    }

    getCurrentPlaylistIndex() {
        return 0;
    }

    getCurrentPlaylistItemId() {
        return this._nowPlayingItem?.PlaylistItemId;
    }

    getRepeatMode() {
        return 'RepeatNone';
    }

    getQueueShuffleMode() {
        return 'Sorted';
    }

    supports() {
        return false;
    }
}

window._airPlayPlayer = airPlayPlayer;
})();
