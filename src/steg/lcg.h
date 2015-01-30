#ifndef lcg_h
#define lcg_h

#include <unordered_map>

using namespace std;

class LCG {
  private:
    long long m, c;
    int trueM;
    long long a;
    long long seed;
    bool zero;
    int gcd(int u, int v);
  public:
    LCG() {};
    LCG(int m, int key, bool zero=false);
    void setSeed(int seed);
    int iterate();
    unordered_map<int, int> map;
};
#endif
