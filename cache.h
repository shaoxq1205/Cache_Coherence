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

/****add new states, based on the protocol****/
enum{
	INVALID = 0,
	S,     //Valid or Shared
	M,     //Dirty or Modified
	E,     //Exclusive
	SM,   //Shared Modified
	SC    //Shared CLean
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq;     // Question: What's this for?
 
public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }//useful function
   bool isValid()         { return ((Flags) != INVALID); }//Including M, SC, SM, E, S, important!!!
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   //Coherence counters
   ulong interventions, invalidations;
   ulong mem_trans; //memory transaction
   //Question: Why MSI and Dragon 0 C_to_C transfers?
   ulong c_to_c_trans; //cache to cache transfers
   ulong flushes;
   ulong busrdxes;

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  

   //Bus Command Flags and CopyExist Flag.
   int busrd, busrdx, busupgr, busupd;
   bool copyexist;
  
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
   ulong getC2C(){ return c_to_c_trans; }
   ulong getInterventions(){ return interventions; }
   ulong getInvalidations(){ return invalidations; }
   ulong getFlushes(){ return flushes; }
   ulong getBusRdX(){ return busrdxes; }


   void writeBack(ulong)   { writeBacks++; mem_trans++; }
   void MSIAccess(int, int, ulong,uchar, int, Cache **);
   void MESIAccess(int, int, ulong,uchar, int, Cache **);
   void DragonAccess(int, int, ulong,uchar, int, Cache **);
   void printStats();
   void updateLRU(cacheLine *);

   //Other functions to handle bus transactions
   void BusRdX(bool, ulong);
   void BusUpgrade(bool, ulong);
   void BusUpdate(bool, ulong);
   void MSIBusRd(bool, ulong);
   void MESIBusRd(bool, ulong);
   void DragonBusRd(bool, ulong);
};

#endif
