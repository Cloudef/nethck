#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>

int main(int argc, char **argv)
{
   if (!nethckServerCreate(NULL, 5050))
      return EXIT_FAILURE;

   int packets = 0;
   while (1) {
      printf("Connected clients: %u\n", nethckServerClientCount());
      if ((packets+=nethckServerUpdate()) && !nethckServerClientCount())
         break;
   }

   nethckServerTerminate();
   return EXIT_SUCCESS;
}
