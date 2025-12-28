# Producer-Consumer Synchronization

Multi-threaded producer–consumer implementation in C using pthreads, semaphores, mutexes, and spinlocks.

## Overview

This project demonstrates thread synchronization using the producer–consumer problem. Multiple producer and consumer threads operate on a bounded buffer, coordinated using semaphores and either mutexes or spinlocks. The project measures performance impacts of critical section length, thread count, and buffer size.

## Key Concepts

- Producer–consumer synchronization
- Mutexes vs spinlocks
- Semaphores for bounded buffers
- Thread coordination with pthreads
- Critical section design
- Performance measurement

## Build and Run

```bash
gcc -pthread main.c -o pc
./pc <buffer_size> <num_producers> <num_consumers> <upper_limit>
