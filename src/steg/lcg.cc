#include <set>
#include <stdio.h>

#include "lcg.h"

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

LCG::LCG(int targetM, int key) {
  this->trueM = targetM;
  int n = 2;
  while (true) {
    n *= 2;
    if (n >=targetM) {
      break;
    }
  }
  this->m = n;

  // To sasify Hull-Dobell Theorem
  int tmpC = this->m-1;
  while(true) {
    if (gcd(tmpC, this->m) == 1)
      break;
    tmpC --;
  }
  this->c = tmpC;
  
  // To sasify Hull-Dobell Theorem
  int tmpA = key % this->m;
  tmpA += (4 - (tmpA % 4));
  this->a = tmpA + 1;

  // Populate the seed->offset map
  int i = 0;
  this->setSeed(0);
  this->map[0] = 0;
  for (i = 1; i < this->trueM; i ++) {
    this->map[i] = this->iterate();
  }
};

LCG::LCG(int m, int c, int a, int trueM): m(m), c(c), a(a), trueM(trueM) {};

void LCG::setSeed(int seed) {
  this->seed = seed;
};

int LCG::iterate() {
  do {
    this->seed = (a * seed + c) % m;
  } while(this->seed >= this->trueM);
  return this->seed;
};

void LCG::debug() {
  printf("m: %d, a: %d, c: %d\n", this->m, this->a, this->c);
};

LCG LCG::getLCG() {
  return LCG(this->m, this->c, this->a, this->trueM);
};
