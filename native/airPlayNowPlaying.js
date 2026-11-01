(function() {
const airPlayNowPlayingStyles = `
#airPlayNowPlaying {
    position: fixed;
    inset: 0;
    z-index: 100000;
    display: none;
    align-items: center;
    justify-content: center;
    gap: 5vw;
    padding: 5vw;
    background: #101010;
    color: #fff;
    font-family: inherit;
    overflow: hidden;
}
#airPlayNowPlaying.visible {
    display: flex;
}
#airPlayNowPlayingBackdrop {
    position: absolute;
    inset: -10%;
    background-size: cover;
    background-position: center;
    filter: blur(60px) brightness(0.35);
}
#airPlayNowPlayingCoverArt {
    position: relative;
    width: 40vh;
    height: 40vh;
    object-fit: cover;
    border-radius: 0.5vh;
    box-shadow: 0 1vh 4vh rgba(0, 0, 0, 0.6);
    background: #333;
}
#airPlayNowPlayingDetails {
    position: relative;
    max-width: 45vw;
}
#airPlayNowPlayingSource {
    font-size: 2.2vh;
    opacity: 0.6;
    text-transform: uppercase;
    letter-spacing: 0.3vh;
}
#airPlayNowPlayingTitle {
    font-size: 5.5vh;
    font-weight: 600;
    margin: 1.5vh 0;
}
#airPlayNowPlayingArtist {
    font-size: 3.5vh;
}
#airPlayNowPlayingAlbum {
    font-size: 3vh;
    opacity: 0.7;
    margin-top: 0.8vh;
}
#airPlayNowPlayingProgress {
    margin-top: 4vh;
    display: flex;
    align-items: center;
    gap: 1.5vh;
    font-size: 2.2vh;
    opacity: 0.8;
}
#airPlayNowPlayingProgressTrack {
    flex: 1;
    height: 0.6vh;
    min-width: 25vw;
    background: rgba(255, 255, 255, 0.25);
    border-radius: 0.3vh;
}
#airPlayNowPlayingProgressFill {
    height: 100%;
    width: 0;
    background: #fff;
    border-radius: 0.3vh;
}
`;

const airPlayNowPlayingMarkup = `
<div id="airPlayNowPlayingBackdrop"></div>
<img id="airPlayNowPlayingCoverArt" alt="">
<div id="airPlayNowPlayingDetails">
    <div id="airPlayNowPlayingSource">AirPlay</div>
    <div id="airPlayNowPlayingTitle"></div>
    <div id="airPlayNowPlayingArtist"></div>
    <div id="airPlayNowPlayingAlbum"></div>
    <div id="airPlayNowPlayingProgress">
        <span id="airPlayNowPlayingPosition"></span>
        <div id="airPlayNowPlayingProgressTrack"><div id="airPlayNowPlayingProgressFill"></div></div>
        <span id="airPlayNowPlayingDuration"></span>
    </div>
</div>
`;

const trackProgress = {
    positionSecondsAtLastUpdate: 0,
    durationSeconds: 0,
    lastUpdateTimeMs: 0,
    isAdvancing: false
};

function airPlayNowPlayingElement() {
    let nowPlayingElement = document.getElementById('airPlayNowPlaying');
    if (nowPlayingElement) {
        return nowPlayingElement;
    }

    const styleElement = document.createElement('style');
    styleElement.textContent = airPlayNowPlayingStyles;
    document.head.appendChild(styleElement);

    nowPlayingElement = document.createElement('div');
    nowPlayingElement.id = 'airPlayNowPlaying';
    nowPlayingElement.innerHTML = airPlayNowPlayingMarkup;
    nowPlayingElement.addEventListener('click', hideAirPlayNowPlaying);
    document.body.appendChild(nowPlayingElement);
    return nowPlayingElement;
}

function showAirPlayNowPlaying() {
    airPlayNowPlayingElement().classList.add('visible');
}

function hideAirPlayNowPlaying() {
    airPlayNowPlayingElement().classList.remove('visible');
}

function clearAirPlayNowPlaying() {
    showTrackDetails('', '', '');
    showCoverArt('');
    trackProgress.positionSecondsAtLastUpdate = 0;
    trackProgress.durationSeconds = 0;
    trackProgress.isAdvancing = false;
    showTrackProgress();
}

function showTrackDetails(title, artist, album) {
    airPlayNowPlayingElement();
    document.getElementById('airPlayNowPlayingTitle').textContent = title;
    document.getElementById('airPlayNowPlayingArtist').textContent = artist;
    document.getElementById('airPlayNowPlayingAlbum').textContent = album;
}

function showCoverArt(coverArtDataUrl) {
    airPlayNowPlayingElement();
    const coverArtElement = document.getElementById('airPlayNowPlayingCoverArt');
    const backdropElement = document.getElementById('airPlayNowPlayingBackdrop');
    if (coverArtDataUrl) {
        coverArtElement.src = coverArtDataUrl;
        backdropElement.style.backgroundImage = `url("${coverArtDataUrl}")`;
    } else {
        coverArtElement.removeAttribute('src');
        backdropElement.style.backgroundImage = 'none';
    }
}

function minutesAndSecondsText(totalSeconds) {
    const wholeSeconds = Math.max(0, Math.floor(totalSeconds));
    const minutes = Math.floor(wholeSeconds / 60);
    const seconds = String(wholeSeconds % 60).padStart(2, '0');
    return `${minutes}:${seconds}`;
}

function currentPositionSeconds() {
    if (!trackProgress.isAdvancing) {
        return trackProgress.positionSecondsAtLastUpdate;
    }
    const secondsSinceLastUpdate = (Date.now() - trackProgress.lastUpdateTimeMs) / 1000;
    const advancedPositionSeconds = trackProgress.positionSecondsAtLastUpdate + secondsSinceLastUpdate;
    return Math.min(advancedPositionSeconds, trackProgress.durationSeconds);
}

function showTrackProgress() {
    airPlayNowPlayingElement();
    const positionSeconds = currentPositionSeconds();
    const hasDuration = trackProgress.durationSeconds > 0;
    const playedFraction = hasDuration ? positionSeconds / trackProgress.durationSeconds : 0;
    document.getElementById('airPlayNowPlayingProgress').style.visibility = hasDuration ? 'visible' : 'hidden';
    document.getElementById('airPlayNowPlayingPosition').textContent = minutesAndSecondsText(positionSeconds);
    document.getElementById('airPlayNowPlayingDuration').textContent = minutesAndSecondsText(trackProgress.durationSeconds);
    document.getElementById('airPlayNowPlayingProgressFill').style.width = `${playedFraction * 100}%`;
}

function startTrackProgressFrom(positionSeconds, durationSeconds) {
    trackProgress.positionSecondsAtLastUpdate = positionSeconds;
    trackProgress.durationSeconds = durationSeconds;
    trackProgress.lastUpdateTimeMs = Date.now();
    trackProgress.isAdvancing = true;
    showTrackProgress();
}

function stopTrackProgress() {
    trackProgress.positionSecondsAtLastUpdate = currentPositionSeconds();
    trackProgress.isAdvancing = false;
}

window.apiPromise.then((api) => {
    api.airplay.airPlayStreamStarted.connect(() => {
        clearAirPlayNowPlaying();
        showAirPlayNowPlaying();
    });
    api.airplay.airPlayTrackChanged.connect((title, artist, album) => {
        showTrackDetails(title, artist, album);
        showAirPlayNowPlaying();
    });
    api.airplay.airPlayCoverArtChanged.connect(showCoverArt);
    api.airplay.airPlayProgressChanged.connect(startTrackProgressFrom);
    api.airplay.airPlayAudioDiscarded.connect(stopTrackProgress);
    api.airplay.airPlayStreamEnded.connect(() => {
        hideAirPlayNowPlaying();
        clearAirPlayNowPlaying();
    });
    api.input.hostInput.connect((actions) => {
        if (actions.includes('back')) {
            hideAirPlayNowPlaying();
        }
    });
    setInterval(() => {
        if (trackProgress.isAdvancing) {
            showTrackProgress();
        }
    }, 1000);
});
})();
