#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "count.h"

#define COMPILER_BARRIER() { __asm__("" ::: "memory"); }

#if TARGET_OS_IPHONE
#define MFENCE() { __asm__("dmb ish" ::: "memory"); }
#else
#define MFENCE() { __asm__("mfence" ::: "memory"); }
#define LFENCE() { __asm__("lfence" ::: "memory"); }
#define SFENCE() { __asm__("sfence" ::: "memory"); }
#endif

volatile bool flag[2];
volatile int turn;

void init_lock(void) {
    flag[0] = false;
    flag[1] = false;
    turn = 0;
}

void lock(int id) {
    int other_id = 1 - id;

    flag[id] = true;            // we want in
    turn = other_id;            // ... but let the other in first
    MFENCE();

    while (flag[other_id] && turn == other_id) /* spin */;
}

void unlock(int id) {
    flag[id] = false;           // we don't want in anymore
}


int counter = 0;

void *counting_thread(void *arg) {
    int my_id = (long)arg; // kludgy

    for (int i = 0; i < thread_cycles; i++) {
        lock(my_id);
        int a = counter;
        COMPILER_BARRIER();
        counter = a + 1;
        COMPILER_BARRIER();
        flag[my_id] = false;           // we don't want in anymore
        // unlock(my_id);
    }

    return NULL;
}

int main(void) {
    pthread_t threads[2];

    init_lock();

    while (true) {
        counter = 0;
        
        pthread_create(&threads[0], NULL, counting_thread, (void*)0);
        pthread_create(&threads[1], NULL, counting_thread, (void*)1);

        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);

        printf("counter: %i (%i)\n", counter, 2*thread_cycles - counter);
    }


    return counter == 2*thread_cycles - counter; // never reached
}
