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
   while ((packets+=nethckServerUpdate())<2)
      sleep(1);

   nethckServerKill();
   return EXIT_SUCCESS;
}
