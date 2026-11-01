(function() {
const TICKS_PER_MILLISECOND = 10000;

class airPlayPlayer {
    constructor({ events, playbackManager }) {
        this.events = events;
        this.playbackManager = playbackManager;

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

        window.apiPromise.then((api) => this.followAirPlayStreams(api));
    }

    followAirPlayStreams(api) {
        this._airPlayReceiver = api.airplay;
        api.airplay.airPlayStreamStarted.connect(() => this.startShowingAirPlayStream());
        api.airplay.airPlayTrackChanged.connect((title, artist, album) => this.showAirPlayTrack(title, artist, album));
        api.airplay.airPlayProgressChanged.connect((positionSeconds, durationSeconds) => this.showAirPlayProgress(positionSeconds, durationSeconds));
        api.airplay.airPlayAudioDiscarded.connect(() => this.showAirPlayPaused());
        api.airplay.airPlayStreamEnded.connect(() => this.stopShowingAirPlayStream());
    }

    async startShowingAirPlayStream() {
        const otherPlayer = this.playbackManager.getCurrentPlayer();
        if (otherPlayer && otherPlayer !== this && this.playbackManager.isPlaying(otherPlayer)) {
            await this.playbackManager.stop(otherPlayer);
        }

        this._isStreaming = true;
        this._isPaused = false;
        this._positionMsAtLastProgressUpdate = 0;
        this._lastProgressUpdateTimeMs = Date.now();
        this._nowPlayingItem = {
            Id: 'airplay',
            PlaylistItemId: 'airplay',
            Name: 'AirPlay',
            Artists: [],
            Type: 'Audio',
            MediaType: 'Audio',
            MediaStreams: [],
            RunTimeTicks: null
        };
        this._timeUpdateInterval = setInterval(() => this.events.trigger(this, 'timeupdate'), 1000);
        this.events.trigger(this, 'itemstarted', [this._nowPlayingItem, this.currentMediaSource()]);
    }

    showAirPlayTrack(title, artist, album) {
        if (!this._isStreaming) {
            return;
        }
        this._nowPlayingItem.Name = title;
        this._nowPlayingItem.Artists = artist ? [artist] : [];
        this._nowPlayingItem.Album = album;
        this.events.trigger(this, 'statechange', [this.getPlayerState()]);
    }

    showAirPlayProgress(positionSeconds, durationSeconds) {
        if (!this._isStreaming) {
            return;
        }
        this._positionMsAtLastProgressUpdate = positionSeconds * 1000;
        this._lastProgressUpdateTimeMs = Date.now();
        this._nowPlayingItem.RunTimeTicks = durationSeconds * 1000 * TICKS_PER_MILLISECOND;
        if (this._isPaused) {
            this._isPaused = false;
            this.events.trigger(this, 'unpause');
        }
        this.events.trigger(this, 'statechange', [this.getPlayerState()]);
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
        const endedItem = this._nowPlayingItem;
        const endedMediaSource = this.currentMediaSource();
        const endedPositionMs = this.currentTime();
        this._isStreaming = false;
        clearInterval(this._timeUpdateInterval);
        this._timeUpdateInterval = null;
        this.events.trigger(this, 'itemstopped', [{
            item: endedItem,
            mediaSource: endedMediaSource,
            positionMs: endedPositionMs,
            nextItem: null,
            nextMediaType: null
        }]);
    }

    getPlayerState() {
        return {
            PlayState: {
                IsPaused: this._isPaused,
                PositionTicks: this.currentTime() * TICKS_PER_MILLISECOND,
                CanSeek: false,
                IsMuted: false,
                VolumeLevel: 100,
                RepeatMode: 'RepeatNone',
                ShuffleMode: 'Sorted',
                PlayMethod: 'DirectPlay'
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
            Id: 'airplay',
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
        return 100;
    }

    setVolume() {
    }

    isMuted() {
        return false;
    }

    setMute() {
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
