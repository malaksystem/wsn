/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A very simple case shows how to run coffee file systerm on the cc2650 sensortag
 * \author
 *         Adam Dunkels <adam@sics.se>
 */


#include "contiki.h"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

#include <stdio.h> /* For printf() */
#include <string.h>



#define TEST_SIZE     256

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();

  int fd;
  int r;
  uint16_t i;

  uint32_t get_size;

  static unsigned char wbuf[TEST_SIZE];
  static unsigned char rbuf[TEST_SIZE];

  for(i = 0; i < TEST_SIZE; i++)
  {
    wbuf[i] = i;
  }
  /* write data into the file */

  //cfs_coffee_format();

  fd = cfs_open("test.txt", CFS_READ | CFS_WRITE);
  if(fd < 0) {
    printf("open fail!\n");
  }
  else
  {
    for(i = 0; i < 2048; i++)
    {
      r = cfs_write(fd, wbuf, sizeof(wbuf));
      //printf("write %d data into file!\n", r);
    }
  	
    cfs_close(fd);
  }

  /* read the data store in file */

  fd = cfs_open("test.txt", CFS_READ | CFS_WRITE);
  if(fd < 0) {
    printf("open fail!\n");
  }
  else
  {
    get_size = cfs_seek(fd, 0, CFS_SEEK_END);

    printf("file size: %ld\n", get_size);

    cfs_seek(fd, 0, CFS_SEEK_SET);


    for(i = 0; i < 2048; i++)
    {
      r = cfs_read(fd, rbuf, sizeof(rbuf));
      (void)r;
      //printf("read %d bytes from file \n", r);
      if(memcmp(wbuf, rbuf, TEST_SIZE) != 0)
      {
        printf("error\n");
      }
      
    }
    
    cfs_close(fd);
  }

  PROCESS_END();
}