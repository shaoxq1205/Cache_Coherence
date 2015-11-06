/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //SXQ: initialize your counters here//
   intervention_counter=0;
   invalidation_counter=0;
   cache_to_cache_counter=0;
   flush_counter=0;
   mem_trans_counter=0;
   busrd=busrdx=busupgr=busupd=copyexist=0;
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
void Cache::Access(ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') newline->setFlags(DIRTY);    
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') line->setFlags(DIRTY);
	}
}

void Cache::MSIAccess(ulong addr,uchar op, int proc_num, int proc_total_num, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/

	busrd=busrdx=0;		
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w')//PrWr 
			{
			line->setFlags(M);
			busrd=0;
			busrdx=1;
			mem_trans_counter++;
			writeMisses++;
			}
		else //PrRd
			{
			line->setFlags(S);
			busrd=1;
			busrdx=0;
			mem_trans_counter++;
			readMisses++;
			}

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') newline->setFlags(DIRTY);    
		
	}
	else/*hit*/
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		switch(line->getFlags())
			{
			case INVALID:
				{
					if(op == 'w')//PrWr 
						{
						line->setFlags(M);
						busrd=0;
						busrdx=1;
						mem_trans_counter++;

						}
					else //PrRd
						{
						line->setFlags(S);
						busrd=1;
						busrdx=0;
						mem_trans_counter++;
						}
				}
			case S:
				{
					if(op == 'w')//PrWr 
						{
						line->setFlags(M);
						busrdx=1;
						mem_trans_counter++;

						}
					//else do nth 
				}
			case M:
				{
					//do nth.
				}
			}
	}
	
	//Each processor/cache read bus commands
	for (int i=0; i<proc_total_num; i++)
		{
		if(i!=proc_num)//not current processor/cache
			{
				cacheline *temp = cache[i]->findLine(addr);
				if(temp) //!=NULL
					switch (temp->getFlags())
					{
						case S:
							{
								if(busrdx) //invalidate all the other caches.
									{
									temp->setFlags(INVALID);
									cache[i]->invalidation_counter++;																	
									}
							}
						case M:
							{
								if(busrd) //difference: invalidation and intervention
									{
									temp->setFlags(S);
									cache[i]->flush_counter++;
									cache[i]->intervention_counter++;
									cache[i]->writebacks++;
									}
								if(busrdx)
									{
									temp->setFlags(INVALID);
									cache[i]->flush_counter++;
									cache[i]->invalidation_counter++;
									cache[i]->writebacks++;
									}
							}
						case INVALID: //do nth.
							
					}
			}
		}
}

void Cache::MESIAccess(ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') newline->setFlags(DIRTY);    
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') line->setFlags(DIRTY);
	}
}

void Cache::DragonAccess(ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') newline->setFlags(DIRTY);    
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') line->setFlags(DIRTY);
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
   if(victim->getFlags() == DIRTY) writeBack(addr);
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats()
{ 
	printf("===== Simulation results      =====\n");
	printf("01. number of reads: 				%lu\n", getReads());
	printf("02. number of read misses: 			%lu\n", getRM());
	printf("03. number of writes: 				%lu\n", getWrites());
	printf("04. number of write misses:			%lu\n", getWM());
	printf("05. total miss rate: 				%.2f%%\n", (double)(getRM() + getWM()) * 100 / (getReads() + getWrites()));
	printf("06. number of writebacks: 			%lu\n", getWB());
	printf("07. number of cache-to-cache transfers: 	%lu\n", getC2CNum());
	printf("08. number of memory transactions: 		%lu\n", getMemTrans());
	printf("09. number of interventions: 			%lu\n", getInterventionNum());
	printf("10. number of invalidations: 			%lu\n", getInvalidationNum());
	printf("11. number of flushes: 				%lu\n", getFlushes());	
}
