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

#define AXIS_RADIUS 0.3
#define TRI_RADIUS (AXIS_RADIUS * 0.25)

#define AXIS_COLOR 0.0, 0.7, 0.9
#define TRI_COLOR 0.0, 0.7, 0.9

#define PI 3.141592653589793238462643383279502884197169399375105820974

#define D_TH (3 * PI / 2)

#define WINDOW_SIZE_PX 800

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
float                   colors[4][3] = {{0.1, 0.1, 0.1},
                                        {0.1, 0.1, 0.1},
                                        {0.1, 0.1, 0.1},
                                        {0.0, 0.7, 0.9}};

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
  
  int i, j, th;
  double max = 0, theta_max;

  /* Draw points */
  glBegin(GL_LINES);
  for(i = 0; i < channels; i++) {
    double y0 = (channels - 1 - 2.0 * i)/channels;
    for(j = 0; j < buffer_size; j++) {
      double value, theta, rho;
      value = buffers[i][j];
      if (value > 0) {
        if(i == 3 && value > max) {
          max = value;
          theta_max = j;
        }
        theta = 2 * PI * j / buffer_size + D_TH;
        rho =  value * value * 2 + AXIS_RADIUS;
        glColor3f(colors[i%4][0], colors[i%4][1], colors[i%4][2]);
        vertex(AXIS_RADIUS * cos(theta), 
                   AXIS_RADIUS * sin(theta));
        glColor3f(colors[i%4][0]*2, colors[i%4][1]*2, colors[i%4][2]*2);
        vertex(rho * cos(theta), rho * sin(theta));
      }
    }
  }
  glEnd();

  /* Draw max */
  glBegin(GL_LINE_STRIP);
  theta_max = 2 * PI * theta_max / buffer_size + D_TH;
  max = 1.1 * max + AXIS_RADIUS;
  glColor3f(colors[3][0], colors[3][1], colors[3][2]);
  vertex(0.8 * AXIS_RADIUS * cos(theta_max), 
             0.8 * AXIS_RADIUS * sin(theta_max));
  vertex(AXIS_RADIUS * cos(theta_max),
             AXIS_RADIUS * sin(theta_max));
  glColor3f(1, 0.8, 0);
  vertex(AXIS_RADIUS * cos(theta_max),
             AXIS_RADIUS * sin(theta_max));
  vertex(max * cos(theta_max), max * sin(theta_max));
  glEnd();

  /* Draw circumference */
  glBegin(GL_LINE_STRIP);
  glColor3f(AXIS_COLOR);
  for(th = 0; th <= 2048; th++) {
    vertex(AXIS_RADIUS * cos(2 * PI * th / 2048),
               AXIS_RADIUS * sin(2 * PI * th / 2048));
  }
  glEnd();

  /* Draw array */
  glBegin(GL_LINE_LOOP);
  glColor3f(TRI_COLOR);
  vertex(TRI_RADIUS*0.75 * cos(D_TH),
             TRI_RADIUS * sin(D_TH));
  vertex(TRI_RADIUS * cos(2 * PI / 3 + D_TH),
             TRI_RADIUS * sin(2 * PI / 3 + D_TH));
  vertex(TRI_RADIUS * cos(4 * PI / 3 + D_TH),
             TRI_RADIUS * sin(4 * PI / 3 + D_TH));
  glEnd();

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
