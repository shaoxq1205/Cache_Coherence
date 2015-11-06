/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu

                Developed by Xiaoqiang Shao
				    for Machine Problem 2
                		  Fall 2015
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	//SXQ: three variables read by each trace line
	int processornum;	
	char operation;
	int address;
	
	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }
	
	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	
	printf("===== 506 Personal information =====");
	printf("Name: Xiaoqiang Shao");
	printf("Unity ID: xshao2");
	printf("ECE492 Students? NO");
	printf("===== 506 SMP Simulator configuration =====");
	printf("L1_SIZE:		%d\n", cache_size);
	printf("L1_ASSOC:		%d\n", cache_assoc);
	printf("L1_BLOCKSIZE:		%d\n", blk_size);
	printf("NUMBER OF PROCESSORS:	%d\n", num_processors);
	if (protocol==0) printf("COHERENCE PROTOCOL:	MSI\n");
	if (protocol==1) printf("COHERENCE PROTOCOL:	MESI\n");
	if (protocol==2) printf("COHERENCE PROTOCOL:	Dragon\n");
	printf("TRACE FILE: 	traces/%s\n",argv[6]);
 
	//*********************************************//
    //*****create an array of caches here**********//

	Cache **cache = (Cache **)malloc(num_processors);
	for (int i = 0; i < num_processors; i++)
	{
		cache[i] = new Cache(cache_size, cache_assoc, blk_size);
	}
	//*********************************************//	

	pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
	while (!feof(pFile))
	{
		fscanf(pFile, "%d", &processornum);
		fscanf(pFile, " ");
		operation = fgetc(pFile);
		fscanf(pFile, "%x", &address);
		fscanf(pFile, "\n");

		if(protocol==0)
			cache[processornum]->MSIAccess(address, operation);//question: what's the difference between "->" and "."?
		if(protocol==1)
			cache[processornum]->MESIAccess(address, operation);
		if(protocol==2)
			cache[processornum]->DragonAccess(address, operation);

		/*
		if ( (protocol == 1) || (protocol==2) )   //MESI and DRAGON
		{
			cache[processornum]->copy = 0;
			for (int i = 0; i < num_processors; i++)
				if (i != processornum)
					if (cache[i]->ifCopyExist(address))
					{
						cache[processornum]->copy = 1;
						break;
					}
		}

		cache[processornum]->Access(address,operation);
		
		if (cache[processornum]->busrd==1)
		{
			if ((protocol == 1) && (cache[processornum]->copy == 1))
			{
				cache[processornum]->ca2ca++;
				cache[processornum]->Memtrans--;
			}

			for (int i = 0; i < num_processors; i++)
			{
				if (i != processornum) cache[i]->BusRd(address);
			}
		}
		if (cache[processornum]->busrdx==1)
		{
			if ((protocol == 1) && (cache[processornum]->copy == 1))
			{
				cache[processornum]->ca2ca++;
				cache[processornum]->Memtrans--;
			}

			for (int i = 0; i < num_processors; i++)
			{
				if (i != processornum) cache[i]->BusRdX(address);
			}
		}
		if (cache[processornum]->busupgr == 1)
		{
			for (int i = 0; i < num_processors; i++)
			{
				if (i != processornum) cache[i]->BusUpgrade(address);
			}
		}
		if (cache[processornum]->busupd == 1)
		{
			for (int i = 0; i < num_processors; i++)
			{
				if (i != processornum) cache[i]->BusUpdate(address);
			}
		}*/
	}
	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	for (int i = 0; i < num_processors; i++)
	{
		printf("============ Simulation results (Cache %d) ============\n", i);
		cache[i]->printStats();
	}
	
	
}
