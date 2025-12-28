#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
//struct for shared buffer and variables
struct SharedData {
    int buffer_size;
    int upper_limit;
    int *buffer;
    int in, out;                //indices for producer and consumer
    int shared_int;             //shared variable for producers
    sem_t empty, full;          //semaphores for empty and full slots
    pthread_mutex_t mutex;      //mutex
    pthread_spinlock_t spinlock; //spinlock
    int use_spinlock;           //1 for spinlock, 0 for mutex
};
 
//#define CSD 1e9 //critical section delay
 
//producer function
void *producer(void *param) {
    struct SharedData *data = (struct SharedData *)param;
    while (1) {
        sem_wait(&data->empty); //wait if no empty slots
 
        if (data->use_spinlock) {
            pthread_spin_lock(&data->spinlock); //enter critical section with spinlock
        } else {
            pthread_mutex_lock(&data->mutex); //enter critical section with mutex
        }
 
        //for (int j = 0; j < CSD; j++);
 
        if (data->shared_int >= data->upper_limit) { //check if upper limit reached
            if (data->use_spinlock) {
                pthread_spin_unlock(&data->spinlock);
            } else {
                pthread_mutex_unlock(&data->mutex);
            }
            sem_post(&data->empty);
            break;
        }
 
        //produce item and add it to the buffer
        data->buffer[data->in] = data->shared_int++;
        data->in = (data->in + 1) % data->buffer_size;
 
        if (data->use_spinlock) {
            pthread_spin_unlock(&data->spinlock);
        } else {
            pthread_mutex_unlock(&data->mutex);
        }
 
        sem_post(&data->full); //signal that an item is available
    }
    pthread_exit(0);
}
 
//consumer function
void *consumer(void *param) {
    void **params = (void **)param;
    struct SharedData *data = (struct SharedData *)params[0];
    int consumer_id = *((int *)params[1]);
 
    while (1) {
        sem_wait(&data->full); //wait if no items are available
 
        if (data->use_spinlock) {
            pthread_spin_lock(&data->spinlock); //enter critical section with spinlock
        } else {
            pthread_mutex_lock(&data->mutex); //enter critical section with mutex
        }
 
        //for (int j = 0; j < CSD; j++);
 
        if (data->buffer[data->out] == -1) { //check for termination signal
            if (data->use_spinlock) {
                pthread_spin_unlock(&data->spinlock);
            } else {
                pthread_mutex_unlock(&data->mutex);
            }
            sem_post(&data->full);
            break;
        }
 
        //consume item from buffer
        int item = data->buffer[data->out];
        //printf("%d, %d\n", item, consumer_id); //print consumed#  and consumer#
        data->out = (data->out + 1) % data->buffer_size;
 
        if (data->use_spinlock) {
            pthread_spin_unlock(&data->spinlock);
        } else {
            pthread_mutex_unlock(&data->mutex);
        }
 
        sem_post(&data->empty); //signal available slot
    }
    pthread_exit(0);
}
 
int main(int argc, char *argv[]) {
    clock_t start, end;
    double elapsed_time;
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <buffer_size> <num_producers> <num_consumers> <upper_limit>\n", argv[0]);
        return -1;
    }
 
    //command-line arguments
    int buffer_size = atoi(argv[1]);
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int upper_limit = atoi(argv[4]);
 
    //shared data
    struct SharedData *data = (struct SharedData *)malloc(sizeof(struct SharedData));
    data->buffer_size = buffer_size;
    data->upper_limit = upper_limit;
    data->buffer = (int *)malloc(sizeof(int) * buffer_size);
    data->in = 0;
    data->out = 0;
    data->shared_int = 0;
    data->use_spinlock = 1; //set to 1 for spinlock, 0 for mutex
 
    for (int i = 0; i < buffer_size; i++) {
        data->buffer[i] = -1; //initialize buffer with dummy values
    }
 
    //semaphores and mutex/spinlock
    sem_init(&data->empty, 0, buffer_size); //empty slots in buffer
    sem_init(&data->full, 0, 0); //full slots in buffer
    pthread_mutex_init(&data->mutex, NULL);
    pthread_spin_init(&data->spinlock, PTHREAD_PROCESS_PRIVATE);
 
    start = clock(); //start elapsed time clock
 
    //create producer and consumer threads
    pthread_t producers[num_producers], consumers[num_consumers];
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producers[i], NULL, producer, data);
    }
 
    int consumer_ids[num_consumers];
    void *consumer_params[num_consumers][2];
    for (int i = 0; i < num_consumers; i++) {
        consumer_ids[i] = i + 1; //user-friendly consumer index
        consumer_params[i][0] = data;
        consumer_params[i][1] = &consumer_ids[i];
        pthread_create(&consumers[i], NULL, consumer, consumer_params[i]);
    }
 
    //wait for all producers to finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
 
    //signal for consumers to exit
    for (int i = 0; i < buffer_size; i++) {
        sem_wait(&data->empty);
        data->buffer[data->in] = -1; //signal termination
        data->in = (data->in + 1) % data->buffer_size;
        sem_post(&data->full);
    }
 
    //wait for all consumers to complete
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }
 
    //end elapsed time
    end = clock();
    elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Elapsed Time: %.2f seconds\n",elapsed_time);
 
    //clean up resources
    sem_destroy(&data->empty);
    sem_destroy(&data->full);
    pthread_mutex_destroy(&data->mutex);
    pthread_spin_destroy(&data->spinlock);
    free(data->buffer);
    free(data);
 
    //printf("Producers-consumers complete.\n");
    return 0;
}
