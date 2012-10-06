#include <stdlib.h>
#include <GL/glfw3.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>

int main(int argc, char **argv)
{
   glhckInit(argc, argv);
   nethckInit(argc, argv);

   return EXIT_SUCCESS;
}
