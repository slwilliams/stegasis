#include <math.h>
#include <stdio.h>

#include "lcg.h"

// Borrowed from http://rosettacode.org/wiki/Greatest_common_divisor
int LCG::gcd(int u, int v) {
  int shift;
 
  if (u == 0) return v;
  if (v == 0) return u;
 
  for (shift = 0; ((u | v) & 1) == 0; shift ++) {
    u >>= 1;
    v >>= 1;
  }
 
  while ((u & 1) == 0) {
    u >>= 1;
  }
 
  do {
    while ((v & 1) == 0) {
      v >>= 1;
    }
    if (u > v) {
      int t = v; v = u; u = t;
    }
    v = v - u;                       
  } while (v != 0);
  return u << shift;
};

LCG::LCG(int targetM, int key, bool zero) {
  this->zero = zero;
  this->trueM = targetM;

  // Find the next largest power of 2 >= targetM
  if ((targetM & (targetM - 1)) == 0) {
    this->m = targetM;
  } else {
    this->m = pow(2, ceil(log(targetM)/log(2)));
  }

  // To sasify Hull-Dobell Theorem
  int tmpC = this->m - 2;
  while (true) { 
    if (gcd(tmpC, this->m) == 1)
      break;
    tmpC --;
  }
  this->c = tmpC;
  
  // To sasify Hull-Dobell Theorem
  int tmpA = key % this->m;
  // Force it to be a multiple of 4
  tmpA += (4 - (tmpA % 4));
  if (tmpA > this->m) tmpA -= 4;
  if (tmpA == 0) tmpA += 4;
  this->a = tmpA + 1;

  // Populate the seed->offset map
  this->setSeed(1);
  this->map[0] = 1;
  for (int i = 1; i < this->trueM; i ++) {
    this->map[i] = this->iterate();
  }
};

void LCG::setSeed(int seed) {
  this->seed = (long long)seed;
};

int LCG::iterate() {
  if (this->zero == true) {
    do {
      this->seed = (a * seed + c) % m;
    } while(this->seed >= this->trueM || this->seed % 64 == 0);
  } else {
    do {
      this->seed = (a * seed + c) % m;
    } while(this->seed >= this->trueM);
  }
  return (int)this->seed;
};
