#include "mbed.h"
#include <string>
#include "../../mbed-os/features/lwipstack/LWIPStack.h"
#include "Adafruit_ST7735.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

using namespace std;

TCPSocket sockServer;
SocketAddress mySocketAddress;
NetworkInterface* network;
Adafruit_ST7735 display(PB_15, PB_14, PB_10, PB_9, PE_4, PE_5);


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
        while(running) {
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

    nsapi_error_t error = NSAPI_ERROR_OK;
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

struct tcp_pcb* testpcb;
struct tcp_pcb* echopcb;

err_t tcpAcceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    (void)arg;
    (void)err;
    (void)newpcb;
    printf("newpcb: %08lX\r\n", (uint32_t)testpcb);
    printf("TCP Accept Err: %i\r\n", err);
    echopcb = newpcb;
    return 0;
}

void tcp_echo_init(void){
  
    printf("creating tcp echo test\r\n");
    testpcb = tcp_new();
    printf("testpcb: %08lX\r\n", (uint32_t)testpcb);

    //tcp_err(testpcb, tcpErrorHandler);
    //tcp_recv(testpcb, tcpRecvCallback);
    //tcp_sent(testpcb, tcpSentCallback);

    static ip4_addr_t ip;
    IP4_ADDR(&ip, 0,0,0,0);

    err_t err;
    err = tcp_bind(testpcb, &ip, 666);
    printf("TCP Bind Err: %i\r\n", err);

    if (ERR_OK != err){
      return;
    }
    
    // The tcp_listen() function returns a new connection identifier, 
    // and the one passed as an argument to the function will be deallocated.
    testpcb = tcp_listen_with_backlog_and_err(testpcb, 1, &err);
    printf("testpcb: %08lX\r\n", (uint32_t)testpcb);
    printf("TCP Listen Err: %i\r\n", err);
    if (NULL == testpcb){
      return;
    }
    
    tcp_accept(testpcb, tcpAcceptCallback);
    
}

void onConnect() 
{
    network->get_ip_address(&mySocketAddress);
    printf("my IP is: %s\n", mySocketAddress.get_ip_address());
}

//    virtual void attach(mbed::Callback<void(nsapi_event_t, intptr_t)> status_cb);
void onEthIfEvent(nsapi_event_t evt, intptr_t value)
{
    if (NSAPI_EVENT_CONNECTION_STATUS_CHANGE == evt) {
        display.setCursor(10, 30);
        switch (value) {
            case NSAPI_STATUS_LOCAL_UP:
                display.setTextColor(ST7735_GREEN, ST7735_BLACK);
                display.printf("EthIF local up      ");
                printf("EthIF local up\n");
                break;
            case NSAPI_STATUS_GLOBAL_UP:
                display.setTextColor(ST7735_GREEN, ST7735_BLACK);
                display.printf("EthIF global up     ");
                display.setTextColor(ST7735_YELLOW, ST7735_BLACK);
                printf("EthIF global up\n");
                onConnect();
                display.setCursor(10, 20);
                display.printf(mySocketAddress.get_ip_address());
                break;
            case NSAPI_STATUS_DISCONNECTED:
                display.setTextColor(ST7735_RED, ST7735_BLACK);
                display.printf("EthIF disconnected  ");
                printf("EthIF disconnected\n");
                display.setCursor(10, 20);
                display.printf("                ");
                break;
            case NSAPI_STATUS_CONNECTING:
                display.setTextColor(ST7735_CYAN, ST7735_BLACK);
                display.printf("EthIF connecting    ");
                printf("EthIF connecting\n");
                display.setCursor(10, 20);
                display.printf("                ");
                break;
            case NSAPI_STATUS_ERROR_UNSUPPORTED:
                display.setTextColor(ST7735_RED, ST7735_BLACK);
                display.printf("status unknown      ");
                printf("EthIF status unknown\n");
                break;
        }
    }
}

int main() {
    printf("Hello from "  TOSTRING(TARGET_NAME) "\n");
    printf("Mbed OS version: %d.%d.%d\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
    printf("test TCPServer\n");

    display.initS();
    display.setRotation(3);
    display.fillScreen(ST7735_BLACK);
    display.drawRect(0, 0, 160, 80, ST7735_BLUE);
    
    display.setTextColor(ST7735_GREEN);
    display.setCursor(10, 10);
    display.printf("Hello");

    //ThreadWithArg *t = new ThreadWithArg();
    //int intArg = 42;
    //t->start(intArg);


    // Connect to the network with the default networking interface
    // if you use WiFi: see mbed_app.json for the credentials
    network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("Cannot connect to the network, see serial output\n");
    } 
    network->set_blocking(false);
    network->attach(callback(onEthIfEvent));
    network->connect();

    sockServer.open(network);
    sockServer.set_blocking(false);
    sockServer.sigio(callback(stateChanged));
    sockServer.bind(9099);
    sockServer.listen(5);

//    tcp_echo_init();
    ThisThread::sleep_for(60000);
    network->disconnect();
    
    while(1) {
        ThisThread::sleep_for(1000);
    }
}

