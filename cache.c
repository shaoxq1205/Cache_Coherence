/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   ca2ca = Memtrans = interventions = invalidations = flushes = busrdxes = 0;
   busrd = busrdx = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::MSIAccess(int processor, int num_processors, ulong addr,uchar op, int protocol, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
	busrd = busrdx = busupgr = busupd = 0;
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		//MSI don't care if other copies exist, just grab from memory
		Memtrans++;

		//RW miss counter
		if(op == 'w') writeMisses++;
		else readMisses++;
		
		//Change state & parse Bus command
		cacheLine *newline = fillLine(addr);
		if (op == 'w')//PrWr
		{
			newline->setFlags(DIRTY);						
			if (protocol == 0) {busrdx = 1;busrdxes++;}
		}
		
		if (op == 'r') //PrRd
		{
			if (protocol == 0) busrd = 1;
		}
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		//Only SHARED -> MODIFIED has BusRdX and MemTransaction
		if ((line->getFlags() == VALID) && (op == 'w'))   
		{
			busrdx = 1;
			busrdxes++;
			Memtrans++;
			line->setFlags(DIRTY);
		}

	}

	//Other Caches deal with bus commands
	for (int i = 0; i < num_processors; i++)
	{
		if (i != processor)
		{
		cache[i]->MSIBusRd(busrd, addr);
		cache[i]->BusRdX(busrdx, addr);
		//no upgrade nor update for MSI
		}
	}

}
void Cache::MESIAccess(int processor, int num_processors, ulong addr,uchar op, int protocol , Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
	busrd = busrdx = busupgr = busupd = 0;
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		//Mem_transaction or other caches feed
		if(copyexist) ca2ca++;
		else Memtrans++;

		//RW miss counter
		if(op == 'w') writeMisses++;
		else readMisses++;
		
		//Change state & parse Bus command
		cacheLine *newline = fillLine(addr);
		if (op == 'w')//PrWr
		{
			newline->setFlags(DIRTY);			
			busrdx = 1;
			busrdxes++;

		}
		
		if (op == 'r') //PrRd
		{
			busrd = 1;
			if (copyexist ) newline->setFlags(VALID);//SHARED
			else newline->setFlags(EXCLUSIVE);
		}
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if ((line->getFlags() == VALID) && (op == 'w'))
			{
				busupgr = 1;
				line->setFlags(DIRTY) ;
			}
		if ((line->getFlags() == EXCLUSIVE) && (op == 'w'))
			line->setFlags(DIRTY) ;//MODIFIED
	}

	//Other Caches deal with bus commands
	for (int i = 0; i < num_processors; i++)
	{
		if (i != processor)
		{
		cache[i]->MESIBusRd(busrd, addr);
		cache[i]->BusRdX(busrdx, addr);
		cache[i]->BusUpgrade(busupgr, addr);
		//no update for MESI
		}
	}
}
void Cache::DragonAccess(int processor, int num_processors, ulong addr,uchar op, int protocol, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
	busrd = busrdx = busupgr = busupd = 0;
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		Memtrans++;

		//RW miss counter
		if(op == 'w') writeMisses++;
		else readMisses++;
		
		//Change state & parse Bus command
		cacheLine *newline = fillLine(addr);
		if (op == 'w')//PrWr
		{
			newline->setFlags(DIRTY);					
			busrd = 1;
			if (copyexist )
			{
				newline->setFlags(SHAREDM);
				busupd = 1;
			}
			else
			{
				newline->setFlags(DIRTY);//MODIFIED
			}
		}
		
		if (op == 'r') //PrRd
		{
			if (copyexist )
			{
				newline->setFlags(SHAREDC);
				busrd = 1;
			}
			else
			{
				newline->setFlags(EXCLUSIVE);
				busrd = 1;
			}
		}
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
			if (op == 'w')
			{
				if (line->getFlags() == EXCLUSIVE)
				{
					line->setFlags(DIRTY);//MODIFIED
				}
				if (line->getFlags() == SHAREDC)
				{
					if (copyexist )
					{
						line->setFlags(SHAREDM);
						busupd = 1;
					}
					else
					{
						line->setFlags(DIRTY);
						busupd = 1;
					}
				}
				if (line->getFlags() == SHAREDM)
				{
					if (copyexist )
						busupd = 1;
					else
					{
						line->setFlags(DIRTY);
						busupd = 1;
					}
				}
			}
	}

	//Other Caches deal with bus commands
	for (int i = 0; i < num_processors; i++)
	{
		if (i != processor)
		{
		cache[i]->DragonBusRd(busrd, addr);
		cache[i]->BusRdX(busrdx, addr);
		cache[i]->BusUpgrade(busupgr, addr);
		cache[i]->BusUpdate(busupd, addr);
		}
	}

}
/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if ((victim->getFlags() == DIRTY) || (victim->getFlags() == SHAREDM))
   {
	   writeBack(addr);
   }
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats()
{ 
	//printf("===== Simulation results      =====\n");
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
	printf("01. number of reads: 				%lu\n", getReads());
	printf("02. number of read misses: 			%lu\n", getRM());
	printf("03. number of writes: 				%lu\n", getWrites());
	printf("04. number of write misses:			%lu\n", getWM());
	printf("05. total miss rate: 				%.2f%%\n", (double)(getRM() + getWM()) * 100 / (getReads() + getWrites()));
	printf("06. number of writebacks: 			%lu\n", getWB());
	printf("07. number of cache-to-cache transfers: 	%lu\n", getC2C());
	printf("08. number of memory transactions: 		%lu\n", Memtrans);
	printf("09. number of interventions: 			%lu\n", getInterventions());
	printf("10. number of invalidations: 			%lu\n", getInvalidations());
	printf("11. number of flushes: 				%lu\n", getFlushes());
	printf("12. number of BusRdX: 				%lu\n", getBusRdX());
}


void Cache::MSIBusRd(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{               
		if (line->getFlags() == DIRTY)
		{
			writeBacks++;
			Memtrans++;
			flushes++;
			line->setFlags(VALID);
			interventions++;
		}
	}
}
}
void Cache::MESIBusRd(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{               
		if (line->getFlags() == DIRTY)
		{
			writeBacks++;
			Memtrans++;
			flushes++;
			line->setFlags(VALID);
			interventions++;
			return;
		}
		if (line->getFlags() == EXCLUSIVE)
		{
			line->setFlags(VALID);
			interventions++;
		}
	}
}
}
void Cache::DragonBusRd(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{               
		if (line->getFlags() == DIRTY)
		{
			line->setFlags(SHAREDM);
			flushes++;
			writeBacks++;
			Memtrans++;
			interventions++;
		}
		if (line->getFlags() == EXCLUSIVE)
		{
			line->setFlags(SHAREDC);
			interventions++;
		}

		if (line->getFlags() == SHAREDM)
		{
			flushes++;
			writeBacks++;
			Memtrans++;
			return;
		}
	}
}
}
void Cache::BusRdX(bool a, ulong addr)
{
if(a){
	cacheLine * line = findLine(addr);
	if (line != NULL)
	{
		if (line->getFlags() == DIRTY)
		{
			writeBacks++;
			Memtrans++;
			flushes++;
		}
		
		line->invalidate();
		invalidations++;
	}
}
}
void Cache::BusUpgrade(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{
		if (line->getFlags() == VALID)
		{
			line->invalidate();
			invalidations++;
		}
	}
}
}

void Cache::BusUpdate(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{
		if (line->getFlags() == SHAREDM) line->setFlags(SHAREDC);
	}
}
}
