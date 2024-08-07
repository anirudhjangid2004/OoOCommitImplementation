#include <algorithm>
#include <array>
#include <map>
#include <cstdlib>
#include "cache.h"

constexpr int PREFETCH_DEGREE = 3;
constexpr int PREFETCH_DISTANCE = 128;

struct stream_entry {
uint64_t start_pointer = 0;
uint64_t end_pointer = 0;
bool direction = 0; // forward is 1, reverse is 0
int state = 0;
uint64_t last_used_cycle=0;
uint64_t last_cl_addr = 0;
};

struct lookahead_entry {
  uint64_t address = 0;
  int64_t stride = 0;
  int degree = 0; // degree remaining
  bool direction = 1;
};

//constexpr std::size_t TRACKER_SETS = 256;
//constexpr std::size_t TRACKER_WAYS = 4;
constexpr std::size_t N_STREAMS = 64;
std::map<CACHE*, lookahead_entry> lookahead;
std::map<CACHE*, std::array<stream_entry, N_STREAMS>> streams;

void CACHE::prefetcher_initialize() { std::cout << NAME << " stream prefetcher " << std::endl; }

void CACHE::prefetcher_cycle_operate()
{
  // If a lookahead is active

  if (auto [old_pf_address,stride,degree,direction] = lookahead[this]; degree > 0) {
    //cout<<"old_pf_addr = "<<old_pf_address<<" degree "<<degree<<" direction "<<direction<<" stride "<< stride << endl;
    uint64_t pf_address = 0;
    if(direction==1)  pf_address  = old_pf_address + (stride << LOG2_BLOCK_SIZE);
    else pf_address = old_pf_address - (stride << LOG2_BLOCK_SIZE);

    // If the next step would exceed the degree or run off the page, stop
    if (virtual_prefetch || (pf_address >> LOG2_PAGE_SIZE) == (old_pf_address >> LOG2_PAGE_SIZE)) {
      // check the MSHR occupancy to decide if we're going to prefetch to this
      // level or not
      bool success = prefetch_line(pf_address, (get_occupancy(0, pf_address) < get_size(0, pf_address) / 2), 0);
      if (success)
      {
        //cout<<"success, pf addr = "<<pf_address<<endl;
        lookahead[this] = {pf_address,stride,degree - 1,direction};
      // If we fail, try again next cycle
      }
    } else {
      lookahead[this] = {};
    }   
  }
}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in)
{
  uint64_t cl_addr = addr >> LOG2_BLOCK_SIZE;
  bool stream_detected = 0;
  //cout << cl_addr << endl;
  int64_t stride = 0;

  // get boundaries of stream set
  //auto set_begin = std::next(std::begin(streams[this]), start_pointer % N_STREAMS);
  //auto set_end = std::next(set_begin,1);

//if 
  //cout << streams.begin()->second[0].state;
    for(int i=0;i<N_STREAMS;i++)
    {
      stream_entry stream = streams.begin()->second[i];
      //cout << "cl_addr " << cl_addr<< " Stream no " << i << " start ptr " << stream.start_pointer << " end ptr " << stream.end_pointer << " state " << stream.state << " direction " << stream.direction << " stride "<< stride << endl;
      //cout << "cl_addr " << cl_addr << endl;
      if((stream.state==1) && (abs(int(stream.start_pointer-cl_addr))<=16))
      {
        stream.state+=1;
        //cout << "I reached state 2" << endl;
        if(int(cl_addr-stream.start_pointer)>0)
          {
            stream.direction = 1;
          //cout << " wrong direction " << endl;
          }
        else
          {stream.direction = 0;}

        stride = abs(int(cl_addr-stream.last_cl_addr));
        stream.last_cl_addr = cl_addr;
        streams.begin()->second[i] = stream;
        stream_detected = 1;
        break;  
      }
          
      else if (((stream.state == 2) && (int(cl_addr - stream.start_pointer) > 0) && (stream.direction == 0) && (abs(int(stream.start_pointer-cl_addr))<=16)) || ((stream.state == 2) && (int(cl_addr - stream.start_pointer) < 0) && (stream.direction == 1)&& (abs(int(stream.start_pointer-cl_addr))<=16)) )
        {
          //cout << "I reached state 1 from state 2" << endl;
          stream.state-=1;
          streams.begin()->second[i] = stream;
          stream_detected = 1;
          break;
        }
      else if (((stream.state==2) && (int(cl_addr - stream.start_pointer)>0) && (stream.direction==1) && (abs(int(stream.start_pointer-cl_addr))<=16)) || ((stream.state==2) && (int(cl_addr - stream.start_pointer))<0 && (stream.direction==0)&& (abs(int(stream.start_pointer-cl_addr))<=16))) 
        {
          //cout << "I reached state 3" << endl;
          stream.state+=1;
          if(stream.direction==1)
            stream.end_pointer = stream.start_pointer+PREFETCH_DISTANCE;
          else
            stream.end_pointer = stream.start_pointer-PREFETCH_DISTANCE;
          stride = abs(int(cl_addr-stream.last_cl_addr));
          stream.last_cl_addr = cl_addr;  
          streams.begin()->second[i] = stream;
          stream_detected = 1;
          
          break;
        }
      //else if ((stream.state==3) && (stream.start_pointer<cl_addr) && (cl_addr<stream.end_pointer) && (stream.end_pointer<stream.start_pointer+PREFETCH_DISTANCE) && (stream.direction == 1))
        //{
          //stream.end_pointer += PREFETCH_DEGREE;
          // stream.state+=1;
          //lookahead[this] = {stream.end_pointer << LOG2_BLOCK_SIZE,PREFETCH_DEGREE,stream.direction};
          //streams.begin()->second[i] = stream; 
          //cout << "I reached state 4" << endl;
          //stream_detected = 1;
          //break;
        //}
      //else if ((stream.state==3) && (stream.start_pointer>cl_addr) && (cl_addr>stream.end_pointer) && (stream.start_pointer<stream.end_pointer+PREFETCH_DISTANCE) && (stream.direction == 0))
        //{
          //stream.end_pointer -= PREFETCH_DEGREE;
          // stream.state+=1;
          //lookahead[this] = {stream.end_pointer << LOG2_BLOCK_SIZE,PREFETCH_DEGREE,stream.direction};
          //streams.begin()->second[i] = stream; 
          //cout << "I reached state 4" << endl;
          //stream_detected = 1;
          //break;
        //}  
      else if ((stream.state==3) && (stream.start_pointer<cl_addr) && (cl_addr<stream.end_pointer) && (stream.direction == 1))
        {
          //cout<<"QWERTY"<<endl;
          //stream.state+=1;
          stride = abs(int(cl_addr-stream.last_cl_addr));
          stream.last_cl_addr = cl_addr;
          lookahead[this] = {stream.end_pointer << LOG2_BLOCK_SIZE,stride,PREFETCH_DEGREE,stream.direction};
          stream.end_pointer += PREFETCH_DEGREE;
          stream.start_pointer = stream.end_pointer - PREFETCH_DISTANCE;
          streams.begin()->second[i] = stream; 
          //cout << "I reached state 4" << endl;
          
          stream_detected = 1;
          break;
        }
      else if ((stream.state==3) && (stream.start_pointer>cl_addr) && (cl_addr>stream.end_pointer) && (stream.direction == 0))
        {
          //cout<<"QWERTY"<<endl;
          //stream.state+=1;
          stride = abs(int(cl_addr-stream.last_cl_addr));
          stream.last_cl_addr = cl_addr;
          lookahead[this] = {stream.end_pointer << LOG2_BLOCK_SIZE,stride,PREFETCH_DEGREE,stream.direction};
          stream.end_pointer -= PREFETCH_DEGREE;
          stream.start_pointer = stream.end_pointer + PREFETCH_DISTANCE;
          streams.begin()->second[i] = stream; 
          //cout << "I reached state 4" << endl;
          
          stream_detected = 1;
          break;
        }  
      else if (stream.state == 0)
        {
          stream.start_pointer = cl_addr;
          stream.last_cl_addr = cl_addr;
          //cout << "Stream no = " << i << " start ptr " << stream.start_pointer << endl;
          stream.state+=1;
          //cout << "temp_state " <<stream.state << endl;
          streams.begin()->second[i] = stream; 
          //cout << "I reached state 1"<<endl;
          stream_detected = 1;
          break;
        }
      else 
        {//cout << "I reached here";
        continue;}
 

      //new_stream = streams.begin()->second[i]
    }
      if(streams.begin()->second[N_STREAMS-1].state!=0 && stream_detected==0)
        {
          uint64_t min_until_now = streams.begin()->second[0].last_used_cycle;
          uint64_t lru_stream = 0;
          for(int i=0;i<N_STREAMS;i++)
            { if(min_until_now>streams.begin()->second[i].last_used_cycle)
                {min_until_now = streams.begin()->second[i].last_used_cycle;
                lru_stream = i;
                }
            }
          streams.begin()->second[lru_stream] =  {cl_addr,0,0,0,0,cl_addr};

        }
        //cout << "final_state "<<streams.begin()->second[0].state << endl;  
        //cout << streams.begin()->second[52].direction << endl;  
 
    // replace by LRU
    //if(PREFETCH_STREAM_SIZE>N_STREAMS)
    //new_stream = std::min_element(set_begin, set_end, [](stream_entry x, stream_entry y) { return x.last_used_cycle < y.last_used_cycle; });




  // update tracking set
  //new_stream = {start}
  return metadata_in;

}


uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_final_stats() {}
