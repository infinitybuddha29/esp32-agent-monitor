#include "../firmware/AgentMonitor/marquee.h"
#include <cassert>
int main() {
  assert(marqueeOffset(12,22,9000)==0);
  assert(marqueeOffset(22,22,9000)==0);
  assert(marqueeOffset(25,22,1799)==0);
  assert(marqueeOffset(25,22,1800)==1);
  assert(marqueeOffset(25,22,2150)==2);
  assert(marqueeOffset(25,22,2500)==3);
  assert(marqueeOffset(25,22,4649)==3);
  assert(marqueeOffset(25,22,4650)==0);
  for(uint32_t t=0;t<100000;t+=23)assert(marqueeOffset(96,22,t)<=74);
}
