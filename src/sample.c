#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdlib.h>

#define SAMPLE_SIZE 2 // Sample size in bytes
#define CHANNEL_MASK 7 // Channel selection mask

char count_channels(char mask) {
  int i = 0;
  char count = 0;
  for(i = 0; i<8; i++) {
    count = count + ((mask & (1<<i)) >> i);
  }
  return count;
}

/* nput arguments: Channel mask, sampling frequency, number of
   samples */
int main(int argc, char ** argv) {
  /* Process input arguments */
  if(argc !=  4) {
     fprintf(stderr, 
             "Usage: sample CHANNEL_MASK SAMPLING_FREQ NUM_SAMPLES\n");
     fprintf(stderr, 
             "Usage: takes NUM_SAMPLES at SAMPLING_FREQ Hz from CHANNEL_MASK\n");
     exit(1);
  }
  char channel_mask; 
  int num_samples, sampling_freq;
  char * endptr;
  channel_mask = (char) strtol(argv[1], &endptr, 2);
  if(*endptr != '\0') {
    fprintf(stderr, 
            "ERROR: first argument must be a binary of 8 bits\n");
    exit(1);
  }
  sampling_freq = (int) strtol(argv[2], &endptr, 10);
  if(*endptr != '\0') {
    fprintf(stderr, 
            "ERROR: second argument must be a positive integer\n");
    exit(1);
  } else if (sampling_freq <= 0 || sampling_freq > 44100 ) {
    fprintf(stderr, "ERROR: sampling frequency must be between 1 and 44100 Hz\n");
    exit(1);
  }
  num_samples = (int) strtol(argv[3], &endptr, 10);
  if(*endptr != '\0' || num_samples < 0) {
    fprintf(stderr, 
            "ERROR: third argument must be a non-negative integer\n");
    exit(1);
  }

  fprintf(stderr, 
          "Channel mask: %d\nSampling frequency: %d\nNumber of samples: %d\n", 
          channel_mask, sampling_freq, num_samples);
  int fd; // File descriptor for the port
  char byte; // Input buffer
  
  int bytes_read = 0;
  int i = 0;
  int num_channels = count_channels(channel_mask);

  /* Open and set up the port */
  fd = open("/dev/ttyS101", O_RDWR | O_NOCTTY);
  if (fd == -1) {
    perror("Unable to open /dev/S101");
    exit(1);
  }
  fcntl(fd, F_SETFL, 0);
  
  /* Send the channel mask */
  if (write(fd, &channel_mask, sizeof(channel_mask)) < 0) {
    perror("Could not write");
    exit(1);
  }
  /* Send the sampling frequency */
  if (write(fd, &sampling_freq, sizeof(sampling_freq)) < 0) {
    perror("Could not write");
    exit(1);
  }
  /* Send the number of samples */
  if (write(fd, &num_samples, sizeof(num_samples)) < 0) {
    perror("Could not write");
    exit(1);
  }

  /* Begin reading samples */
  while(!num_samples || bytes_read < num_samples * SAMPLE_SIZE * num_channels) {
    if (( i = read(fd, &byte, 1)) < 0) {
      perror("Could not read");
      fprintf(stderr, "%d\n", i); 
      close(fd);
      exit(1);
    }
    bytes_read = bytes_read + i;
    write(1, &byte, i); // Output through standard output
  }
  close(fd);
  return 0;
}
