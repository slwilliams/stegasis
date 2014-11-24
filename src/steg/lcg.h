#ifndef lcg_h
#define lcg_h

#include <unordered_map>

class LCG {
  private:
    int m, c;
    int trueM;
    int a;
    long long seed;
    bool zero;
    int gcd(int u, int v);
  public:
    LCG() {};
    LCG(int m, int key, bool zero=false);
    LCG(int m, int c, int a, int trueM);
    void setSeed(int seed);
    int iterate();
    LCG getLCG();
    std::unordered_map<int, int> map;
    void debug();
};
#endif
