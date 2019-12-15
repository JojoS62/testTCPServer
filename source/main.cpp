#include "mbed.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

class ThreadWithArg : public Thread {
public:
    int _intArg;

    void start(int intArg) {
        _intArg = intArg;
        Thread::start(callback(this, &ThreadWithArg::threadFn));
    }

    void threadFn()
    {
        DigitalOut led1(LED1);
        
        bool running = true;
        int i = 0;
        while(i < 10) {
            uint64_t nextTime = get_ms_count() + 200;

            i++;
            led1 = !led1;
            printf("thread arg: %i\r", _intArg);
            fflush(stdout);

            ThisThread::sleep_until(nextTime);
        }
    }
};



int main() {
    printf("Hello from "  TOSTRING(TARGET_NAME) "\n");
    printf("Mbed OS version: %d.%d.%d\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
    printf("test Thread\n");

    printf("create thread\n");
    ThreadWithArg *t = new ThreadWithArg();
    int intArg = 42;
    printf("start thread\n");
    t->start(intArg);

    printf("...wait for thread to finish\n");
    t->join();
    printf("delete thread\n");
    delete t;
    printf("done.\n");

    while(1) {
        ThisThread::sleep_for(1000);
    }
}

