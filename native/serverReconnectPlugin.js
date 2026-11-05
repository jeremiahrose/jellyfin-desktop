(function() {
const SECONDS_TO_WAIT_BEFORE_RECONNECTING_TO_SERVER = 5;

class serverReconnectPlugin {
    constructor({ events, ServerConnections }) {
        this.name = 'Server Reconnect Plugin';
        this.type = 'input';
        this.id = 'serverReconnectPlugin';
        this.events = events;
        this.serverConnections = ServerConnections;

        ServerConnections.getApiClients().forEach(apiClient => this.keepConnectedToServer(apiClient));
        events.on(ServerConnections, 'apiclientcreated', (_event, apiClient) => this.keepConnectedToServer(apiClient));
    }

    keepConnectedToServer(apiClient) {
        this.events.on(apiClient, 'websocketclose', () => {
            console.log(`Server connection closed, reconnecting in ${SECONDS_TO_WAIT_BEFORE_RECONNECTING_TO_SERVER} seconds`);
            setTimeout(() => apiClient.ensureWebSocket(), SECONDS_TO_WAIT_BEFORE_RECONNECTING_TO_SERVER * 1000);
        });
        this.events.on(apiClient, 'websocketopen', () => {
            apiClient.reportCapabilities(this.serverConnections.capabilities());
        });
    }
}

window._serverReconnectPlugin = serverReconnectPlugin;
})();
