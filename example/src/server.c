#include <stdlib.h>
#include <unistd.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>

int main(int argc, char **argv)
{
   nethckInit(argc, argv);

   if (!nethckServerInit(NULL, 5050))
      return EXIT_FAILURE;

   int packets = 0;
   while (1) {
      printf("Connected clients: %u\n", nethckServerClientCount());
      if ((packets+=nethckServerUpdate()) && !nethckServerClientCount())
         break;
   }

   nethckServerKill();
   return EXIT_SUCCESS;
}
