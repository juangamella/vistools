#include<stdio.h>
#include<stdlib.h>
#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>
#include<math.h>
#include<limits.h>
#include<string.h>

/* Size of the buffer */
#define BUFFER_SIZE 16384

/* Number of samples added to the buffer in each iteration */
#define BUFFER_STEP 1024

Display                 *dpy;
Window                  root;
GLint                   att[] = {GLX_RGBA,
                                 GLX_DEPTH_SIZE,
                                 24,
                                 GLX_DOUBLEBUFFER,
                                 None};
XVisualInfo             *vi;
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
XWindowAttributes       gwa;
XEvent                  xev;
int                     channels;
 
void drawBuffer(short int ** buffers, int plot_pointer) {
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1., 1., -1., 1., 1., 20.);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0., 0., 5., 0., 0., 0., 0., 1., 0.);

  int i, j;
  double deltaX = 2.0 / BUFFER_SIZE;
  for(i = 0; i < channels; i++) {
    /* Channel x-axis y coordinate with respect to global coordinates*/
    double y0 = (channels - 1 - 2.0 * i)/channels;
    glBegin(GL_LINE_STRIP); 
    glColor3f(0.0, 0.7, 0.9);
    for(j = 0; j < BUFFER_SIZE; j++) {
      double value, x, y;
      value = (double) buffers[i][(plot_pointer + j) % BUFFER_SIZE];
      value = value / SHRT_MAX; // Normalize (value in [-1, 1])
      value = value / (channels + 0.1); // Fit in window sub-division
      y = y0 + value;
      x = -1 + j*deltaX;
      glVertex2f(x, y);
    }
    glEnd();
  }
}

int main(int argc, char *argv[]) {
  /* Process input arguments */
  if(argc !=  2) {
    fprintf(stderr, 
            "Usage: plot CHANNELS\n");
    fprintf(stderr, 
            "Usage: plots interlaced data into CHANNELS channels\n");
    exit(1);
  }
  
  char * endptr;
  channels = strtol(argv[1], &endptr, 10);
  if(*endptr != '\0' || channels < 1) {
    fprintf(stderr, 
            "ERROR: first argument must be a positive integer\n");
    exit(1);
  }
  fprintf(stderr, "Number of channels: %d\n", channels);
  
  /* Set up buffers */
  short int * buffers[channels];
  int i;
  for (i = 0; i < channels; i++)
    buffers[i] = (short int *) malloc(sizeof(short int) * BUFFER_SIZE);

    dpy = XOpenDisplay(NULL);
 
    if(dpy == NULL) {
      fprintf(stderr, "Cannot connect to X server\n");
      exit(0);
    }
        
    root = DefaultRootWindow(dpy);

    vi = glXChooseVisual(dpy, 0, att);

    if(vi == NULL) {
      fprintf(stderr, "No appropriate visual found\n");
      exit(0);
    } 
    else {
      fprintf(stderr, "visual %p selected\n", (void *)vi->visualid);
    }

    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask;
 
    win = XCreateWindow(dpy, 
                        root, 
                        0, 
                        0, 
                        1024, 
                        600, 
                        0, 
                        vi->depth,
                        InputOutput,
                        vi->visual,
                        CWColormap | CWEventMask, 
                        &swa);

    XMapWindow(dpy, win);
    XStoreName(dpy, win, "Plot Stream");
 
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
 
    glEnable(GL_DEPTH_TEST); 
    int buffer_full = 0,
      write_pointer = 0,
      plot_pointer = 0;
    while(1) {
      if (buffer_full)
        plot_pointer += BUFFER_STEP;
      int sam_read = 0,
        channel_pointer = 0;
      /* Read BUFFER_STEP samples for each channel from standard input */
      for (sam_read = 0; sam_read < BUFFER_STEP; sam_read++) {
        int i, j;
        for(i = 0; i < channels; i++) {
          /* Read a sample into channel i */
          char sample[sizeof(short int)];
          for(j = 0; j < sizeof(short int); j++) {
            /* Read each byte of the sample and
               write it in the sample buffer */
            int n = read(0, &sample[j], 1);
            if ( n < 0 ) {
              perror("Could not read");
              exit(1);
            } else if ( n == 0) {
              fprintf(stderr, "Standard input closed\n");
              exit(0);
            }
            if (write(1, &sample[j], 1) < 0) {
              perror("Could not write");
              exit(1);
            }
          }
          /* Copy sample into channel buffer */
          memcpy(&buffers[i][write_pointer + sam_read], 
                 sample, 
                 sizeof(short int));
        }
      }
      write_pointer += BUFFER_STEP;
      if(!buffer_full && write_pointer >= BUFFER_SIZE) {
        buffer_full = 1;
      }
      write_pointer = write_pointer % BUFFER_SIZE;
      XGetWindowAttributes(dpy, win, &gwa);
      glViewport(0, 0, gwa.width, gwa.height);
      drawBuffer(buffers, plot_pointer);
      glXSwapBuffers(dpy, win);      
    }
}
