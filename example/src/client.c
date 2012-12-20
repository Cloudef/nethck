#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <GL/glfw3.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>

static int RUNNING = 0;
static int WIDTH = 800, HEIGHT = 480;
static int close_callback(GLFWwindow window)
{
   RUNNING = 0;
   return 1;
}

static void resize_callback(GLFWwindow window, int width, int height)
{
   WIDTH = width; HEIGHT = height;
   glhckDisplayResize(width, height);
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckCamera *camera;
   float          now          = 0;
   float          last         = 0;
   unsigned int   frameCounter = 0;
   unsigned int   FPS          = 0;
   unsigned int   fpsDelay     = 0;
   float          duration     = 0;
   float          delta        = 0;
   char WIN_TITLE[256];
   memset(WIN_TITLE, 0, sizeof(WIN_TITLE));

   if (!glfwInit())
      return EXIT_FAILURE;

   glfwWindowHint(GLFW_DEPTH_BITS, 24);
   if (!(window = glfwCreateWindow(WIDTH, HEIGHT, GLFW_WINDOWED, "client", NULL)))
      return EXIT_FAILURE;

   glfwMakeContextCurrent(window);

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(1);

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckDisplayCreate(WIDTH, HEIGHT, GLHCK_RENDER_AUTO))
      return EXIT_FAILURE;

   if (!nethckClientInit(NULL, 5050))
      return EXIT_FAILURE;

   if (!(camera = glhckCameraNew()))
      return EXIT_FAILURE;

   glhckCameraRange(camera, 1.0f, 1000.0f);
   glhckObjectPositionf(glhckCameraGetObject(camera), 0.0f, 0.0f, -5.0f);

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);

   RUNNING = 1;
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;
      glfwPollEvents();

      glhckCameraUpdate(camera);
      if (nethckClientUpdate()) {
         glhckClear();
         glhckRender();
         glfwSwapBuffers(window);
      }

      if (fpsDelay < now) {
         if (duration > 0.0f) {
            FPS = (float)frameCounter / duration;
            snprintf(WIN_TITLE, sizeof(WIN_TITLE)-1, "OpenGL [FPS: %d]", FPS);
            glfwSetWindowTitle(window, WIN_TITLE);
            frameCounter = 0; fpsDelay = now + 1; duration = 0;
         }
      }
      ++frameCounter;
      duration += delta;
   }

   nethckClientKill();
   glhckTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}
