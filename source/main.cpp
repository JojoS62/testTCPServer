#include "mbed.h"
#include <string>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

using namespace std;

TCPSocket sockServer;

class ThreadWithArg : public Thread {
public:
    int _intArg;

    void start(int intArg) {
        _intArg = intArg;
        Thread::start(callback(this, &ThreadWithArg::threadFn));
    }

    void threadFn()
    {
        printf("start thread\n");
        
        DigitalOut led1(PE_2);
        
        bool running = true;
        int i = 0;
        while(true) {
            uint64_t nextTime = get_ms_count() + 200;

            i++;
            led1 = !led1;

            ThisThread::sleep_until(nextTime);
        }
    }
};

void stateChanged() 
{
    printf("sockServer state changed\n");

    nsapi_error_t error = NULL;
    TCPSocket *sockClient = sockServer.accept(&error);
    if (sockClient && (error == NSAPI_ERROR_OK)) {
        SocketAddress sockAddrClient;
        sockClient->getpeername(&sockAddrClient);
        printf("connected to %s\n", sockAddrClient.get_ip_address());

        string msg = "Hello from Mbed\n";
        sockClient->send(msg.c_str(), msg.length());

        sockClient->close();
    }

}

int main() {
    printf("Hello from "  TOSTRING(TARGET_NAME) "\n");
    printf("Mbed OS version: %d.%d.%d\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
    printf("test TCPServer\n");

    ThreadWithArg *t = new ThreadWithArg();
    int intArg = 42;
    //t->start(intArg);


    // Connect to the network with the default networking interface
    // if you use WiFi: see mbed_app.json for the credentials
    NetworkInterface* network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("Cannot connect to the network, see serial output\n");
        return 1;
    } 
    nsapi_error_t connect_status = network->connect();

    if (connect_status != NSAPI_ERROR_OK) {
        printf("Failed to connect to network (%d)\n", connect_status);
        return 2;
    } else {
        SocketAddress socketAddress;
        network->get_ip_address(&socketAddress);
        printf("my IP is: %s\n", socketAddress.get_ip_address());
    } 

    sockServer.open(network);
    sockServer.set_blocking(false);
    sockServer.sigio(callback(stateChanged));
    sockServer.bind(9099);
    sockServer.listen(5);


    while(1) {
        ThisThread::sleep_for(1000);
    }
}

