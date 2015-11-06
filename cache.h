/*******************************************************
                          cache.h
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****SXQ: add new states, based on the protocol****/
enum{
	INVALID = 0,
	VALID,
	DIRTY,
	S,
	M,
	E,//exclusive
	SM, //shared modified
	SC //shared clean
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; //question: what does seq mean?
 
public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }//useful function              //question: why set tag 0?
   bool isValid()         { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   //SXQ: add coherence counters here///
   ulong intervention_counter, invalidation_counter;
   ulong cache_to_cache_counter, flush_counter;
   ulong mem_trans_counter;
   int busrd, busrdx, busupgr, busupd, copyexist;

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM(){return readMisses;} 
   ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}
   ulong getWrites(){return writes;}
   ulong getWB(){return writeBacks;}
   
   void writeBack(ulong)   {writeBacks++;}
   void Access(ulong,uchar);
   void printStats();
   void updateLRU(cacheLine *);

   //******///
   //SXQ: add other functions to handle bus transactions///
   ulong getC2CNum(){return cache_to_cache_counter;} 
   ulong getInterventionNum(){return intervention_counter;} 
   ulong getInvalidationNum(){return invalidation_counter;} 
   ulong getFlushes(){return flush_counter;} 
   ulong getMemTrans(){return mem_trans_counter;}  

   void MSIAccess(ulong, uchar, int, int, Cache **);
   void MESIAccess(ulong, uchar);
   void DragonAccess(ulong, uchar);
   //******///

};

#endif
