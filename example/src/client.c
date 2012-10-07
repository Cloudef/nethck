#include <stdlib.h>
#include <unistd.h>
#include <GL/glfw3.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>

int main(int argc, char **argv)
{
   glhckInit(argc, argv);
   nethckInit(argc, argv);

   if (!nethckClientInit(NULL, 5050))
      return EXIT_FAILURE;

   while (nethckClientUpdate());
   nethckClientKill();
   return EXIT_SUCCESS;
}
