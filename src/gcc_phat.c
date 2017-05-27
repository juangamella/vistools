#include "kiss_fftr.h"

#define WINDOW_SIZE 1024
#define INTERPOLATION 8
#define MAX_TAU 10

void read_to_buffers(kiss_fft_scalar ** buffers,
                     int buffer_size,
                     int channels);
void gcc_phat(kiss_fft_scalar * x,
              kiss_fft_scalar * y,
              kiss_fft_scalar * output,
              int nfft,
              int interpolation);

int main(int argc, char ** argv) {
  float output_buffer[WINDOW_SIZE * INTERPOLATION];
  
  kiss_fft_scalar * input_buffers[2];
  input_buffers[0] = 
    (kiss_fft_scalar *) malloc(sizeof(kiss_fft_scalar) * WINDOW_SIZE);
  input_buffers[1] = 
    (kiss_fft_scalar *) malloc(sizeof(kiss_fft_scalar) * WINDOW_SIZE);
  
  fprintf(stderr, "(%s): sample size: %li\n", argv[0], sizeof(kiss_fft_scalar));
  
  while(1) {
    read_to_buffers(input_buffers, WINDOW_SIZE, 2);
    gcc_phat(input_buffers[0],
             input_buffers[1],
             output_buffer,
             WINDOW_SIZE,
             INTERPOLATION);
    if(write(1,
             &output_buffer[(WINDOW_SIZE / 2 - MAX_TAU) * INTERPOLATION],
             sizeof(kiss_fft_scalar) * (2 * MAX_TAU * INTERPOLATION + 1)) < 0) {
      perror("Could not write");
      exit(1);
    }
  }
  return 0;
}

/* Reads samples from the standard input, deinterlaces them,
   transforms them to kiss_fft_scalar and puts them into a buffer for
   each channel */
void read_to_buffers(kiss_fft_scalar ** buffers, int buffer_size, int channels) {
  int samples_read, i, j;
  for (samples_read = 0; samples_read < buffer_size; samples_read++) {
    for(i = 0; i < channels; i++) {
      /* Read a sample into channel i */
      char sample[sizeof(short int)];
      for(j = 0; j < sizeof(short int); j++) {
        /* Read each byte of the sample and write it in the sample buffer */
        int n = read(0, &sample[j], 1);
        if ( n < 0 ) {
          perror("Could not read");
          exit(1);
        } else if ( n == 0) {
          fprintf(stderr, "Standard input closed\n");
              exit(0);
        }
      }
      /* Copy sample into channel buffer */
      short int * aux = (short int *) sample;
      buffers[i][samples_read] = (kiss_fft_scalar) *aux;
    }
  }
}

void gcc_phat(kiss_fft_scalar * x,
              kiss_fft_scalar * y,
              kiss_fft_scalar * output,
              int nfft,
              int interpolation) {
  kiss_fftr_cfg cfg = 
    kiss_fftr_alloc(nfft, 0, NULL, NULL);
  kiss_fftr_cfg cfgi = 
    kiss_fftr_alloc(nfft * interpolation, 1, NULL, NULL);
  kiss_fft_cpx X [nfft * interpolation];
  kiss_fft_cpx Y [nfft * interpolation];
  /* Perform gcc */
  kiss_fftr(cfg, x, X);
  kiss_fftr(cfg, y, Y);
  int i;
  for(i = 0; i <= nfft/2; i++) {
    kiss_fft_cpx aux;
    aux.r = X[i].r * Y[i].r;
    aux.i = X[i].i * Y[i].r - X[i].r * Y[i].i - X[i].i * Y[i].i;
    double norm = sqrt(aux.r * aux.r + aux.i * aux.i);
    X[i].r = (kiss_fft_scalar) aux.r / norm / nfft;
    X[i].i = (kiss_fft_scalar) aux.i / norm / nfft;
  }
  /* Set extra zeroes for interpolation */
  for(i = nfft/2 + 1; i < nfft * interpolation; i++) {
    X[i].r = 0;
    X[i].i = 0;
  }
  kiss_fft_scalar aux[nfft * interpolation];
  kiss_fftri(cfgi, X, aux);
  /* FFT shift */
  for(i = 0; i < nfft * interpolation / 2; i++) {
    output[i] = aux[nfft * interpolation / 2 + i];    
    output[nfft * interpolation /2 + i] = aux[i];
  }
  free(cfg);
  free(cfgi);
}
