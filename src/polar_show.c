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

#define AXIS_RADIUS 0.45
#define TRI_RADIUS (AXIS_RADIUS * 0.25)

#define AXIS_COLOR 0.0, 0.0, 0.0
#define TRI_COLOR 0.0, 0.7, 0.9

#define PI 3.141592653589793238462643383279502884197169399375105820974

#define D_TH (0)

#define WINDOW_SIZE_PX 1000

#define SAMPLE_TYPE float

Display                 *dpy;
Window                  root;
GLint                   att[] = {GLX_RGBA,
                                 GLX_DEPTH_SIZE,
                                 24,
                                 GLX_DOUBLEBUFFER,
                                 None };
XVisualInfo             *vi;
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
XWindowAttributes       gwa;
XEvent                  xev;
int                     channels;
int                     buffer_size;
float                   colors[4][3] = {{0.0, 0.35, 0.45},
{0, 0.9, 0.7},
                                        {1, 0.9, 0.0},
                                        {0.1, 0.1, 0.1}};

/* Return the red percentage of the rainbow spectrum for the given angle */
float r (double theta);

/* Return the green percentage of the rainbow spectrum for the given angle */
float g (double theta);

/* Return the blue percentage of the rainbow spectrum for the given angle */
float b (double theta);

/* Draw a vertex maintaining square window proportion **/
void vertex(float x, float y);

/* Reads samples from the standard input and deinterlaces them into a
   buffer for each channel. Rewrites each sample to the standard
   output */
void read_to_buffers(SAMPLE_TYPE ** buffers,
                     int buffer_size,
                     int channels) {
  int samples_read, i, j;
  for (samples_read = 0; samples_read < buffer_size; samples_read++) {
    for(i = 0; i < channels; i++) {
      /* Read a sample into channel i */
      char sample[sizeof(SAMPLE_TYPE)];
      for(j = 0; j < sizeof(SAMPLE_TYPE); j++) {
        /* Read each byte of the sample and store it */
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
      memcpy(&buffers[i][samples_read], sample, sizeof(SAMPLE_TYPE));
    }
  }
}

/* Draws each buffer channel */
void drawBuffer(SAMPLE_TYPE ** buffers) {
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1., 1., -1., 1., 1., 20.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0., 0., 5., 0., 0., 0., 0., 1., 0.);
  int points = buffer_size / 3;
  int i, j, th;
  double max = 0, theta_max;
  /** Iterate over each channel, painting stuff **/
  for(i = 0; i < channels; i++) {
    double sum = 0;
    for (j = 1; j < points; j++) {
      sum += buffers[i][j] / SHRT_MAX / 4;
    }
    sum += AXIS_RADIUS;

    /* Draw points */
    glBegin(GL_LINES);
    for(j = 1; j < points; j++) {
      double value, theta, rho;
      value = buffers[i][j] / SHRT_MAX * 64;
      if (value > 0) {
        if(i == 3 && value > max) {
          max = value;
          theta_max = j;
        }
        theta = 2 * PI * j / points + D_TH;
        rho = value / 2 + sum;
        //        glColor3f(r(theta) / 4, g(theta) / 4, b(theta) / 4);
        glColor3f(colors[0][0], colors[0][1], colors[0][2]);
        vertex(sum * cos(theta),
               sum * sin(theta));
        //        glColor3f(r(theta), g(theta), b(theta));
        glColor3f(colors[1][0], colors[1][1], colors[1][2]);
        vertex(rho * cos(theta), rho * sin(theta));
      }
    }
  }
  glEnd();

  /* Draw outer circumference */
  /* glBegin(GL_LINE_STRIP); */
  /* glColor3f(AXIS_COLOR); */
  /* for(th = 0; th <= 2048; th++) { */
  /*   vertex(AXIS_RADIUS * cos(2 * PI * th / 2048), */
  /*              AXIS_RADIUS * sin(2 * PI * th / 2048)); */
  /* } */
  /* glEnd(); */

}

int main(int argc, char *argv[]) {
  /* Process input arguments */
  if(argc !=  3) {
    fprintf(stderr, 
            "(%s) Usage: plot BUFFER_SIZE CHANNELS\n", argv[0]);
    fprintf(stderr, 
            "(%s) Usage: plots interlaced data into CHANNELS channels\n",
            argv[0]);
    exit(1);
  }

  char * endptr;
  buffer_size = strtol(argv[1], &endptr, 10);
  if(*endptr != '\0' || buffer_size < 1) {
    fprintf(stderr, 
            "(%s) ERROR: first argument must be a positive integer\n",
            argv[0]);
    exit(1);
  }
  fprintf(stderr, "(%s) Buffer size: %d\n", argv[0], buffer_size);
  
  channels = strtol(argv[2], &endptr, 10);
  if(*endptr != '\0' || channels < 1) {
    fprintf(stderr,
            "(%s) ERROR: first argument must be a positive integer\n",
            argv[0]);
    exit(1);
  }
  fprintf(stderr, "(%s) Number of channels: %d\n", argv[0], channels);
  
  /* Set up buffers */
  SAMPLE_TYPE * buffers[channels];
  int i;
  for (i = 0; i < channels; i++) {
    buffers[i] = (SAMPLE_TYPE *) malloc(sizeof(SAMPLE_TYPE) * buffer_size);
  }
  
  /* Set up display */
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
                      WINDOW_SIZE_PX,
                      WINDOW_SIZE_PX,
                      0,
                      vi->depth,
                      InputOutput,
                      vi->visual,
                      CWColormap | CWEventMask,
                      &swa);
  XMapWindow(dpy, win);
  XStoreName(dpy, win, "Polar Plot");
 
  glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
  glXMakeCurrent(dpy, win, glc);

  /* Main loop */
  while(1) {
    read_to_buffers(buffers, buffer_size, channels);
    XGetWindowAttributes(dpy, win, &gwa);
    glViewport(0, 0, gwa.width, gwa.height);
    drawBuffer(buffers);
    glXSwapBuffers(dpy, win);      
  }
}

void vertex(float x, float y) {
  double scale = (1.0 * gwa.height) / gwa.width;
  glVertex2f(scale * x, y);
}

double min(double a, double b) {
  if (a < b) 
    return a;
  else
    return b;
}

double max(double a, double b) {
  if (a > b) 
    return a;
  else
    return b;
}

float r(double theta) {
  return 0;
  //  return max(0, cos(theta) + 1/3) + 0.7;
}

float g(double theta) {
  return 0.5;
  //return max(0, cos(theta + 2 * PI / 3) + 1/3) + 0.7;
}

float b(double theta) {
  return 0.7;
  //return max(0, cos(theta + 4 * PI / 3) + 1/3) + 0.7;
}


