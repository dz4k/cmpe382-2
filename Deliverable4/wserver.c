#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "request.h"
#include "io_helper.h"

char default_root[] = ".";

sem_t mutex, fullSlots, emptySlots;

typedef struct {
	int capacity;
	int head;
	int count;
	int array[];
} Queue;

Queue* makeQueue(int capacity) {
	Queue* rv = malloc(sizeof(Queue) + capacity * sizeof(int));
	rv->capacity = capacity;
	rv->head = 0;
	rv->count = 0;
	return rv;
}

int enq(Queue* q, int n) {
	if (q->count >= q->capacity) return -1;
	q->count++;
	q->head = (q->head + 1) % q->capacity;
	q->array[q->head] = n;
	return 0;
}

int deq(Queue* q) {
	if (q->count == 0) return 0;
	q->count--;
	return q->array[(q->head - q->count) & q->capacity];
}

///////

void* worker(void* queue) {
	while (1) {
		Queue* q = (Queue*)queue;
		sem_wait(&fullSlots);
		sem_wait(&mutex);
		int fd = deq(q);
		sem_post(&mutex);
		sem_post(&emptySlots);
		request_handle(fd); // The request is handled by this function 
	}
	return NULL;
}


//
// ./wserver [-p <portnum>] [-t threads] [-b buffers] 
// 
// e.g.
// ./wserver -p 2022 -t 5 -b 10
// 
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int threads = 2;
	int buffer_size = 5;
    
    while ((c = getopt(argc, argv, "p:t:b:")) != -1)
		switch (c) {
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			threads = atoi(optarg);
			break;
		case 'b':
			buffer_size = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: ./wserver [-p <portnum>] [-t threads] [-b buffers] \n");
			exit(1);
		}

	
	
	printf("Server running on port: %d, threads: %d, buffer: %d\n", port, threads, buffer_size);

    // run out of this directory
    chdir_or_die(root_dir);

    // now, get to work
	Queue *q = makeQueue(3);
	
	// Start worker threads

	
    int listen_fd = open_listen_fd_or_die(port);
    while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		

		sem_wait(&emptySlots);
		sem_wait(&mutex);
		enq(q, conn_fd);
		sem_post(&mutex);
		sem_post(&fullSlots);
	}
    return 0;
}


    


 
