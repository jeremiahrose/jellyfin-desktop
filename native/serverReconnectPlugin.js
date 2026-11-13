(function() {
const SECONDS_TO_WAIT_BEFORE_REPORTING_CAPABILITIES_AGAIN = 5;
const MESSAGE_TYPE_THE_SDK_NEVER_PASSES_TO_SUBSCRIBERS = 'ForceKeepAlive';

class serverReconnectPlugin {
    constructor({ events, ServerConnections }) {
        this.name = 'Server Reconnect Plugin';
        this.type = 'input';
        this.id = 'serverReconnectPlugin';
        this.serverConnections = ServerConnections;

        ServerConnections.getApiClients().forEach(apiClient => this.reportCapabilitiesWheneverServerConnectionOpens(apiClient));
        events.on(ServerConnections, 'apiclientcreated', (_event, apiClient) => this.reportCapabilitiesWheneverServerConnectionOpens(apiClient));
    }

    reportCapabilitiesWheneverServerConnectionOpens(apiClient) {
        const sdkServerConnection = this.openSdkServerConnection(apiClient);
        sdkServerConnection.onStatusChange(serverConnectionStatus => {
            if (serverConnectionStatus === WebSocket.OPEN) {
                this.reportCapabilitiesUntilServerAccepts(apiClient, sdkServerConnection);
            }
        });
    }

    openSdkServerConnection(apiClient) {
        apiClient.subscribe([MESSAGE_TYPE_THE_SDK_NEVER_PASSES_TO_SUBSCRIBERS], () => {});
        return apiClient._sdk.webSocket;
    }

    async reportCapabilitiesUntilServerAccepts(apiClient, sdkServerConnection) {
        try {
            await apiClient.reportCapabilities(this.serverConnections.capabilities());
        } catch (serverRefusalResponse) {
            console.error(`Server refused capabilities with status ${serverRefusalResponse.status}, reporting again in ${SECONDS_TO_WAIT_BEFORE_REPORTING_CAPABILITIES_AGAIN} seconds`);
            setTimeout(() => {
                if (sdkServerConnection.socketStatus === WebSocket.OPEN) {
                    this.reportCapabilitiesUntilServerAccepts(apiClient, sdkServerConnection);
                }
            }, SECONDS_TO_WAIT_BEFORE_REPORTING_CAPABILITIES_AGAIN * 1000);
        }
    }
}

window._serverReconnectPlugin = serverReconnectPlugin;
})();
