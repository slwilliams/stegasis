#ifndef lcg_h
#define lcg_h

#include <unordered_map>

class LCG {
  private:
    int m, c;
    int trueM;
    int a;
    int seed;
    int gcd(int u, int v);
  public:
    LCG(int m);
    LCG(int m, int c, int a, int trueM);
    void setSeed(int seed);
    int iterate();
    void debug();
    LCG getLCG();
    std::unordered_map<int, int> map;
};
#endif
