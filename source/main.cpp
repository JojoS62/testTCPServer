#include "mbed.h"
#include <string>

TCPSocket *sockServer;
SocketAddress mySocketAddress;
NetworkInterface* network;

void serverStateChanged() 
{
    printf("sockServer state changed\n");
    if (sockServer == nullptr) {
        return;
    }
    
    nsapi_error_t error = NSAPI_ERROR_OK;
    TCPSocket *sockClient = sockServer->accept(&error);
    if (sockClient && (error == NSAPI_ERROR_OK)) {
        SocketAddress sockAddrClient;
        sockClient->getpeername(&sockAddrClient);
        printf("connected to %s\n", sockAddrClient.get_ip_address());

        string msg = "Hello from Mbed\n";
        sockClient->send(msg.c_str(), msg.length());

        sockClient->close();
        printf("client connection closed\n");
    }
}

void startServer()
{
    nsapi_error_t error = NSAPI_ERROR_OK;
    sockServer = new TCPSocket;
    
    sockServer->open(network);
    if (error != NSAPI_ERROR_OK) {
        printf("sockServer open error: %i\n", error);
    }

    sockServer->set_blocking(false);
    int optval = 1;
    sockServer->setsockopt(NSAPI_SOCKET, NSAPI_REUSEADDR,  &optval, sizeof(optval));
    sockServer->sigio(callback(serverStateChanged));
    
    error = sockServer->bind(9099);
    if (error != NSAPI_ERROR_OK) {
        printf("sockServer bind error: %i\n", error);
    }

    error = sockServer->listen(1);
    if (error != NSAPI_ERROR_OK) {
        printf("sockServer listen error: %i\n", error);
    }
    
    printf("server started\n");
}

void stopServer()
{
    if (sockServer) {
        sockServer->close();
        delete sockServer;
        sockServer = nullptr;
        printf("server stopped\n");
    }
}

void onEthIfUp() 
{
    startServer();
}

void onEthIfDown() 
{
    stopServer();
}

//    virtual void attach(mbed::Callback<void(nsapi_event_t, intptr_t)> status_cb);
void onEthIfEvent(nsapi_event_t evt, intptr_t value)
{
    if (NSAPI_EVENT_CONNECTION_STATUS_CHANGE == evt) {
        switch (value) {
            case NSAPI_STATUS_LOCAL_UP:
                printf("EthIF local up\n");
                break;
            case NSAPI_STATUS_GLOBAL_UP:
                printf("EthIF global up\n");
                network->get_ip_address(&mySocketAddress);
                printf("my IP is: %s\n", mySocketAddress.get_ip_address());
                onEthIfUp();
                break;
            case NSAPI_STATUS_DISCONNECTED:
                printf("EthIF disconnected\n");
                onEthIfDown();
                break;
            case NSAPI_STATUS_CONNECTING:
                onEthIfDown();
                printf("EthIF connecting\n");
                break;
            case NSAPI_STATUS_ERROR_UNSUPPORTED:
                printf("EthIF status unknown\n");
                break;
        }
    }
}

int main() {
    printf("Hello from "  MBED_STRINGIFY(TARGET_NAME) "\n");
    printf("Mbed OS version: %d.%d.%d\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
    printf("test TCPServer\n");

    // Connect to the network with the default networking interface
    // using non blocking, async event driven
    network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("Cannot connect to the network, see serial output\n");
    } 

    network->set_blocking(false);
    network->attach(callback(onEthIfEvent));
    network->connect();

    // main loop, check for key pressed
    while(1) {
        char ch = getc(stdin);

        switch (ch) {
            case 'd': 
                network->disconnect();
                break;
            case 'c': 
                network->connect();
                break;
            case 's': 
                stopServer();
                break;
            case 'r':
                startServer();
                break;
        }

        ThisThread::sleep_for(10ms);
    }
}

