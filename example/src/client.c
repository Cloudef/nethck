#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <GL/glfw3.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>

typedef struct ClientData {
   kmVec3 cameraPos, cameraRot;
   double mouseX, mouseY;
   double lastMouseX, lastMouseY;
   float delta;
   int width, height;
   glhckProjectionType projectionType;
   char activity;
   char running;
} ClientData;

static void closeCallback(GLFWwindow *window)
{
   ClientData *data = (ClientData*)glfwGetWindowUserPointer(window);
   if (!data) return;
   data->running = 0;
}

static void resizeCallback(GLFWwindow *window, int width, int height)
{
   ClientData *data = (ClientData*)glfwGetWindowUserPointer(window);
   if (!data) return;
   data->width = width;
   data->height = height;
   glhckDisplayResize(width, height);
   data->activity = 1;
}

static void mouseCallback(GLFWwindow* window, double mousex, double mousey)
{
   ClientData *data = (ClientData*)glfwGetWindowUserPointer(window);
   if (!data) return;
   data->mouseX = mousex;
   data->mouseY = mousey;
   if (!data->lastMouseX && !data->lastMouseY) {
      data->lastMouseX = mousex;
      data->lastMouseY = mousey;
   }
   data->activity = 1;
}

static void handleCamera(GLFWwindow* window) {

   ClientData *data = (ClientData*)glfwGetWindowUserPointer(window);
   if (!data) return;

   float delta = data->delta;
   kmVec3 *cameraPos = &data->cameraPos;
   kmVec3 *cameraRot = &data->cameraRot;

   data->projectionType = GLHCK_PROJECTION_PERSPECTIVE;

   if (glfwGetKey(window, GLFW_KEY_P)) {
      data->projectionType = GLHCK_PROJECTION_ORTHOGRAPHIC;
      data->activity = 1;
   }

   if (glfwGetKey(window, GLFW_KEY_W)) {
      cameraPos->x -= cos((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->z += sin((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->y += cos((cameraRot->x + 90) * kmPIOver180) * 25.0f * delta;
      data->activity = 1;
   } else if (glfwGetKey(window, GLFW_KEY_S)) {
      cameraPos->x += cos((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->z -= sin((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->y -= cos((cameraRot->x + 90) * kmPIOver180) * 25.0f * delta;
      data->activity = 1;
   }

   if (glfwGetKey(window, GLFW_KEY_A)) {
      cameraPos->x -= cos((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      cameraPos->z += sin((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      data->activity = 1;
   } else if (glfwGetKey(window, GLFW_KEY_D)) {
      cameraPos->x += cos((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      cameraPos->z -= sin((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      data->activity = 1;
   }

   cameraRot->z = 0;
   cameraRot->z -=  glfwGetKey(window, GLFW_KEY_Z) * (float)(data->mouseX - data->lastMouseX) / 7;
   cameraRot->y -= !glfwGetKey(window, GLFW_KEY_Z) * (float)(data->mouseX - data->lastMouseX) / 7;
   cameraRot->x += (float)(data->mouseY - data->lastMouseY) / 7;

   data->lastMouseX = data->mouseX;
   data->lastMouseY = data->mouseY;
}

int main(int argc, char **argv)
{
   ClientData clientData;
   GLFWwindow *window;
   glhckCamera *camera;
   glhckObject *camObj;
   glhckLight **lights;
   glhckObject **objects;
   unsigned int objectCount, lightCount, i, i2;
   float          now          = 0;
   float          last         = 0;
   unsigned int   frameCounter = 0;
   unsigned int   FPS          = 0;
   unsigned int   fpsDelay     = 0;
   float          duration     = 0;
   float          delta        = 0;
   char WIN_TITLE[256];
   memset(WIN_TITLE, 0, sizeof(WIN_TITLE));
   memset(&clientData, 0, sizeof(ClientData));

   clientData.width = 800;
   clientData.height = 480;
   clientData.running = 1;

   if (!glfwInit())
      return EXIT_FAILURE;

   glfwWindowHint(GLFW_SAMPLES, 4);
   glfwWindowHint(GLFW_RED_BITS, 8);
   glfwWindowHint(GLFW_GREEN_BITS, 8);
   glfwWindowHint(GLFW_BLUE_BITS, 8);
   glfwWindowHint(GLFW_ALPHA_BITS, 8);
   glfwWindowHint(GLFW_DEPTH_BITS, 24);
   if (!(window = glfwCreateWindow(clientData.width, clientData.height, "client", NULL, NULL)))
      return EXIT_FAILURE;

   glfwSetWindowUserPointer(window, &clientData);
   glfwMakeContextCurrent(window);

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(1);

   if (!nethckClientCreate(NULL, 5050))
      return EXIT_FAILURE;

   if (!glhckContextCreate(argc, argv))
      return EXIT_FAILURE;

   if (!glhckDisplayCreate(clientData.width, clientData.height, GLHCK_RENDER_OPENGL))
      return EXIT_FAILURE;

   if (!(camera = glhckCameraNew()))
      return EXIT_FAILURE;

   glhckCameraFov(camera, 45.0f);
   glhckCameraRange(camera, 1.0f, 1000.0f);
   glhckObjectPositionf(glhckCameraGetObject(camera), 15.0f, 8.0f, -15.0f);

   lightCount = 2;
   lights = malloc(lightCount * sizeof(glhckLight*));
   if (!lights) return EXIT_FAILURE;

   for (i = 0; i != lightCount; ++i) {
      if (!(lights[i] = glhckLightNew()))
         return EXIT_FAILURE;

      glhckLightAttenf(lights[i], 0.2f, 0.2f, 0.4f);
      glhckLightPointLightFactor(lights[i], 1.0f);
      glhckLightColorb(lights[i], 255, 255, 255, 255);
      glhckObjectPositionf(glhckLightGetObject(lights[i]), 0.0f, 20.0f, 0.0f);
   }

   glfwSetWindowCloseCallback(window, closeCallback);
   glfwSetWindowSizeCallback(window, resizeCallback);
   glfwSetCursorPosCallback(window, mouseCallback);
   glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_CAPTURED);

   camObj = glhckCameraGetObject(camera);
   while (clientData.running && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;
      clientData.delta = delta;

      glfwPollEvents();
      handleCamera(window);
      glhckCameraProjection(camera, clientData.projectionType);
      glhckObjectPosition(camObj, &clientData.cameraPos);
      glhckObjectTargetf(camObj, clientData.cameraPos.x, clientData.cameraPos.y, clientData.cameraPos.z + 1);
      glhckObjectRotate(camObj, &clientData.cameraRot);
      glhckCameraUpdate(camera);

      if (lightCount) {
         glhckObjectPosition(glhckLightGetObject(lights[0]), &clientData.cameraPos);
         glhckObjectMovef(glhckLightGetObject(lights[0]), 0, 25.0f, 0);
      }

      if (glfwGetKey(window, GLFW_KEY_L)) {
         glhckRenderPassFlags(GLHCK_PASS_DEPTH | GLHCK_PASS_BLEND | GLHCK_PASS_TEXTURE);
         clientData.activity = 1;
      } else {
         glhckRenderPassFlags(GLHCK_PASS_DEPTH | GLHCK_PASS_BLEND | GLHCK_PASS_TEXTURE | GLHCK_PASS_LIGHTING);
      }

      if (nethckClientUpdate() || clientData.activity) {
         objects = nethckClientObjects(&objectCount);
         glhckRenderClear(GLHCK_COLOR_BUFFER | GLHCK_DEPTH_BUFFER);
         for (i = 0; i != lightCount; ++i) {
            for (i2 = 0; i2 != objectCount; ++i2) glhckObjectDraw(objects[i2]);
            glhckLightBeginProjectionWithCamera(lights[i], camera);
            glhckLightBind(lights[i]);
            glhckLightEndProjectionWithCamera(lights[i], camera);
            if (i) glhckRenderBlendFunc(GLHCK_ONE, GLHCK_ONE);
            glhckRender();
         }
         glhckRenderBlendFunc(GLHCK_ZERO, GLHCK_ZERO);
         glfwSwapBuffers(window);
         clientData.activity = 0;
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

   nethckClientTerminate();
   glhckContextTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}
