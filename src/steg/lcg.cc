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

LCG::LCG(int targetM) {
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
  // TODO make this dependent on this->m
  this->a = 5;
};

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