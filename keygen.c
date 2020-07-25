#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[])
{
    srand(time(0));

    if(argc != 2)
    {
        fprintf(stderr, "correct usage: %s <length>\n", argv[0]);
        fflush(stderr);
        return(-1);
    }

    int length = atoi(argv[1]);
    char key[length + 1];  // length + 1 for null terminator

    char *allowed = "ABCDEFGHIJKLMOPQRSTUVWXYZ ";
    
    for(int i = 0; i < length; i++)
    {
        key[i] = allowed[rand() % 27];
    }

    key[length] = '\0';

    fprintf(stdout, "KEY: %s\n", key);
    fflush(stdout);
    
  return (0);
}
