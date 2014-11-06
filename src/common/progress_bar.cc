#include <stdio.h>

#include "progress_bar.h"

// Process has done i out of n rounds,
// and we want a bar of width w.
void loadBar(int x, int n, int w) {
    // Calculuate the ratio of complete-to-incomplete.
    float ratio = x/(float)n;
    int   c     = ratio * w;
 
    // Show the percentage complete.
    printf("%3d%% [", (int)(ratio*100) );
 
    // Show the load bar.
    for (int x=0; x<c; x++)
       printf("=");
 
    for (int x=c; x<w; x++)
       printf(" ");
 
    // ANSI Control codes to go back to the
    // previous line and clear it.
    if (x != n) {
      printf("]\n\033[F\033[J");
    } else {
      printf("]\n");
    }
}

