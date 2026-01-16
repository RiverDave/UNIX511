#include "pidUtil.h"
#include "stdio.h"
#include <vector>
#include <cassert>

using namespace std;

int main(void) {

  vector<int> pidList;
  GetAllPids(pidList);

  assert(!pidList.empty());


  return 0;
}
