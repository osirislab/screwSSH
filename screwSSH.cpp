/*
 * Copyright 2010 Boris Kochergin <bk@isis.poly.edu>. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <vector>
#include <utility>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

using namespace std;

in_addr *internetAddress;
sockaddr_in socketAddress;
uint16_t port;
vector <size_t> socketIndex;
pthread_mutex_t outputLock;
char timeString[128];

void setTime() {
  time_t _time = time(NULL);
  tm __time;
  localtime_r(&_time, &__time);
  pthread_mutex_lock(&outputLock);
  strftime(timeString, sizeof(timeString), "[%F %T] ", &__time);
  pthread_mutex_unlock(&outputLock);
}

void *clock(void*) {
  while (1) {
    sleep(1);
    setTime();
  }
  /* Not reached. */
  return NULL;
}

void *screwSSH(void *index) {
  char buffer[1024];
  int fd, error;
  const size_t _index = *(size_t*)index + 1;
  /* Connect to server, and reconnect immediately after being disconnected. */
  while (1) {
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
      pthread_mutex_lock(&outputLock);
      cerr << timeString << "Socket " << _index << ": socket(): "
           << strerror(errno) << endl;
      pthread_mutex_unlock(&outputLock);
      sleep(1);
      continue;
    }
    if (connect(fd, (const sockaddr*)&socketAddress,
                sizeof(sockaddr_in)) == -1) {
      pthread_mutex_lock(&outputLock);
      cerr << timeString << "Socket " << _index << ": connect(): "
           << strerror(errno) << endl;
      pthread_mutex_unlock(&outputLock);
      sleep(1);
      continue;
    }
    pthread_mutex_lock(&outputLock);
    cout << timeString << "Socket " << _index << " connected." << endl;
    pthread_mutex_unlock(&outputLock);
    /* Just something to catch disconnects. */
    do {
      error = read(fd, &buffer, sizeof(buffer));
    } while (error > 0);
    pthread_mutex_lock(&outputLock);
    cout << timeString << "Socket " << _index << " disconnected; reconnecting."
         << endl;
    pthread_mutex_unlock(&outputLock);
    close(fd);
  }
  /* Not reached. */
  return NULL;
}

int main(int argc, char *argv[]) {
  hostent *host;
  int error;
  vector <pthread_t> threads;
  pthread_t clockThread;
  if (argc < 4) {
    cerr << "usage: " << argv[0] << " host port connections" << endl;
    return 1;
  }
  /* Look up host's IP address using DNS, if necessary. */
  host = gethostbyname(argv[1]);
  if (host == NULL) {
    cerr << argv[0] << ": gethostbyname(): " << hstrerror(h_errno) << endl;
    return 1;
  }
  error = pthread_mutex_init(&outputLock, NULL);
  if (error != 0) {
    cerr << argv[0] << ": pthread_mutex_create(): " << strerror(error) << endl;
    return 1;
  }
  internetAddress = (in_addr*)(host -> h_addr_list[0]);
  port = strtoul(argv[2], NULL, 10);
  /* Zero out socket structure. */
  bzero(&socketAddress, sizeof(struct sockaddr_in));
  /* Set socket's address family to IPv4. */
  socketAddress.sin_family = AF_INET;
  /* Set socket's address to the one we got from DNS. */
  socketAddress.sin_addr = *internetAddress;
  /*
   * Set socket's port to the big-endian version of the one we got from the 
   * user.
   */
  socketAddress.sin_port = htons(port);
  threads.resize(strtoul(argv[3], NULL, 10));
  socketIndex.resize(threads.size());
  setTime();
  /* Spawn one thread for each TCP connection. */
  for (size_t i = 0; i < threads.size(); ++i) {
    socketIndex[i] = i;
    error = pthread_create(&(threads[i]), NULL, &screwSSH, &(socketIndex[i]));
    if (error != 0) {
      pthread_mutex_lock(&outputLock);
      cerr << argv[0] << ": pthread_create(): " << strerror(error) << endl;
      pthread_mutex_unlock(&outputLock);
      return 1;
    }
  }
  error = pthread_create(&clockThread, NULL, &clock, NULL);
  if (error != 0) {
    pthread_mutex_lock(&outputLock);
    cerr << argv[0] << ": pthread_create(): " << strerror(error) << endl;
    pthread_mutex_unlock(&outputLock);
    return 1;
  }
  /* Wait forever for clock thread to return. */
  error = pthread_join(clockThread, NULL);
  if (error != 0) {
    pthread_mutex_lock(&outputLock);
    cerr << argv[0] << ": pthread_join(): " << strerror(error) << endl;
    pthread_mutex_unlock(&outputLock);
    return 1;
  }
  /* Not reached. */
  return 0;
}
