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
   glhckObject *cube;
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

   glfwOpenWindowHint(GLFW_DEPTH_BITS, 24);
   if (!(window = glfwOpenWindow(WIDTH, HEIGHT, GLFW_WINDOWED, "client", NULL)))
      return EXIT_FAILURE;

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(1);

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckDisplayCreate(WIDTH, HEIGHT, GLHCK_RENDER_AUTO))
      return EXIT_FAILURE;

   nethckInit(argc, argv);
   if (!nethckClientInit(NULL, 5050))
      return EXIT_FAILURE;

   if (!(camera = glhckCameraNew()))
      return EXIT_FAILURE;

   glhckCameraRange(camera, 1.0f, 1000.0f);
   glhckObjectPositionf(glhckCameraGetObject(camera), 0.0f, 0.0f, -5.0f);

   if (!(cube = glhckCubeNew(1)))
      return EXIT_FAILURE;

   glhckObjectPositionf(cube, 0.0f, 0.0f, 0.0f);
   glhckObjectRotatef(cube, 0.0f, 25.0f, 1.0f);
   glhckObjectColorb(cube, 0, 255, 255, 85);

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);

   char shouldRender = 1;
   RUNNING = 1;
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;
      glfwPollEvents();

      glhckCameraUpdate(camera);

#if 1
      if (nethckClientUpdate()) {
         glhckClear();
         glhckRender();
         glfwSwapBuffers();
         shouldRender = 1;
      }

      glhckObjectRotatef(cube, 0.0f, 30.0f * delta, 0.0f);
      if (shouldRender) {
         nethckClientObjectRender(cube);
         shouldRender = 0;
      }
#else
      glhckObjectRotatef(cube, 0.0f, 30.0f * delta, 0.0f);
      glhckObjectDraw(cube);
      glhckClear();
      glhckRender();
      glfwSwapBuffers();
#endif

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
