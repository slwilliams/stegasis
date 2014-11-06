#include <stdio.h>

#include "progress_bar.h"

// Taken from rosshemsley.co.uk/2011/02/creating-a-progress-bar-in-c-or-any-other-console-app/
// Slightly modified

// Process has done i out of n rounds, with width w
void loadBar(int x, int n, int w) {
    float ratio = x / (float)n;
    int c = ratio * w;
 
    printf("%3d%% [", (int)(ratio * 100));
 
    int i = 0;
    for (i = 0; i < c; i ++) {
       printf("=");
    }
    for (i = c; i < w; i ++) {
       printf(" ");
    }
 
    if (x != n) {
      printf("]\n\033[F\033[J");
    } else {
      printf("]\n");
    }
}

