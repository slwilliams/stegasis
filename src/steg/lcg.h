#ifndef lcg_h
#define lcg_h

#include <unordered_map>

class LCG {
  private:
    int m, c;
    int trueM;
    int a;
    long long seed;
    int gcd(int u, int v);
  public:
    LCG() {};
    LCG(int m, int key);
    LCG(int m, int c, int a, int trueM);
    void setSeed(int seed);
    int iterate();
    void debug();
    LCG getLCG();
    std::unordered_map<int, int> map;
};
#endif
