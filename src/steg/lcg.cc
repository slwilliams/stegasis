class LCG {
  private:
    int m, a, c;
    int seed;
    int gcd(int u, int v) {
      int shift;
     
      if (u == 0) return v;
      if (v == 0) return u;
     
      for (shift = 0; ((u | v) & 1) == 0; ++shift) {
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
  public:
    LCG(int m, int seed): m(m), seed(seed) {
      // TODO find suitable values for a and c
      int tmpC = m-1;
      while(true) {
        if (gcd(tmpC, m) == 1)
          break;
      }
      c = tmpC;
    };
    int iterate() {
      seed = (a * seed + c) % m;
      return seed;
    };

};
