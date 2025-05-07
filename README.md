# CS4448/5550 Project 1: Multithreaded Webserver

## Introduction
This project involves designing and implementing a multithreaded web server in C for Unix/Linux platforms. The server handles HTTP requests using multiple worker threads to improve performance and handle concurrent requests. The server supports different scheduling algorithms to determine which request is handled by each worker thread. The goal of this project is to simulate a basic HTTP server with different scheduling policies like FIFO, HPSC, HPDC, and ANY.

## Design Overview
The web server is designed to handle incoming HTTP requests and process them using multiple worker threads. The server consists of the following components:

1. **Main Thread**: 
   - Listens for incoming HTTP connections.
   - Inserts incoming requests into a fixed-size queue.
   - Creates a pool of worker threads.

2. **Scheduler Thread**: 
   - Responsible for scheduling requests from the queue to available worker threads based on the specified scheduling algorithm.

3. **Worker Threads**:
   - A pool of worker threads that handle HTTP requests concurrently.
   - Each worker thread processes either static or dynamic requests based on the chosen scheduling policy.

4. **Scheduling Policies**:
   - **ANY**: A worker thread handles any request in the buffer.
   - **FIFO**: A worker thread handles the first (oldest) request in the buffer.
   - **HPSC (Highest Priority to Static Content)**: A worker thread handles the first static content request, if available.
   - **HPDC (Highest Priority to Dynamic Content)**: A worker thread handles the first dynamic content request, if available.

## Complete Specification
### Command Line Arguments
The web server program is run as follows:
```bash
server [portnum] [threads] [buffers] [schedalg]
