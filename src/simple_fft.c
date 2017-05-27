#include "kiss_fftr.h"

#define WINDOW_SIZE 1024
#define INTERPOLATION 1

kiss_fft_scalar input_buffer[WINDOW_SIZE];
kiss_fft_cpx output_buffer[WINDOW_SIZE];

int main() {        
  kiss_fftr_cfg cfg = kiss_fftr_alloc(WINDOW_SIZE, 0, NULL, NULL);
  
  fprintf(stderr, "FFT: sample size: %li\n", sizeof(kiss_fft_scalar));
  
  int i, j;
  char sample[sizeof(kiss_fft_scalar)];
  while(1) {
    /* Read WINDOW_SIZE samples into the input buffer */
    for(i = 0; i < WINDOW_SIZE; i++) {
      /* Samples must be read byte by byte because we might not be
         reading from a disk file ie. a pipe*/
      for(j = 0; j < sizeof(kiss_fft_scalar); j++) {
        int n = read(0, &sample[j], 1);
        if ( n < 0 ) {
          perror("FFT: Could not read");
          free(cfg);
          exit(1);
        } else if ( n == 0) {
          fprintf(stderr, "FFT: Standard input closed\n");
          free(cfg);
          exit(0);
        }
      }
      /* Copy sample into input buffer */
      memcpy(&input_buffer[i], sample, sizeof(kiss_fft_scalar));
    }
    kiss_fftr(cfg, &input_buffer[0], &output_buffer[0]);
    input_buffer[0] -= 15687;
    /* Print modulus of FFT */
    for(i = 0; i < WINDOW_SIZE; i++) {
      kiss_fft_cpx c = output_buffer[i];
      kiss_fft_scalar r = c.r;
      kiss_fft_scalar i = c.i;
      float output = sqrt(r*r + i*i);
      if(write(1, &output, sizeof(output)) < 0) {
        perror("FFT: Could not write");
        free(cfg);
        exit(1);
      }
    }
  }
  return 0;
}
