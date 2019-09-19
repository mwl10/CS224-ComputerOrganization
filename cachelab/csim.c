#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define MAXFILE 256
#define MAXLINE 256

// Matthew Lowery

// fix warnings
// cache is empty/line isn't full--put in first available line


// Data for a line in the cache
// You should not modify this struct
typedef struct cacheLine {
  // ignore block offset bits, and set bits; everthing else that is the tag
    long tag;      // tag for the line
    // EVICTION based on smallest value for LRU************************
    long lru;      // counter for lru
    char valid;    // is the cache line valid
} cacheLine_t;

// Data for the cache
// You may add to this struct if needed
typedef struct cacheInfo {
    // These fields are set in getOptions
    int E;       // lines per set
    int s;       // number of set bits
    int b;       // number of block bits
    char verbose; // print out more cache info
    char trace[MAXFILE]; // Name of the tracefile

    // These fields are set in cacheInit
    int hits;    // total number of cache hits
    int misses;  // total number of cache misses
    int evictions; // total number of evictions;
    cacheLine_t *cache;  // Our cache


    // Add any more fields you need here
    int maxLRU;   // value for the maximum LRU

} cacheInfo_t;

/*
   Print out each line of the cache in a human readable format

   First print out a line with the total hits, misses, and evictions

   Hits: 112195, Misses: 174532, Evictions: 174528

   Then for each line in the cache, print out the set, line, valid
   bit, tag, and lru number.  A printout for a cache with 4 sets and 2
   lines per set might look like the following:


   S:0  L:1  V:1  T:1ffbfdb    LRU:104160
   S:1  L:0  V:1  T:1ffbf81    LRU:104177
   S:1  L:1  V:1  T:1ffbfda    LRU:101416
   S:2  L:0  V:1  T:1ffbf9c    LRU:104146
   S:2  L:1  V:1  T:1ffbf9b    LRU:104116
   S:3  L:0  V:1  T:1ffbfd9    LRU:101287
   S:3  L:1  V:1  T:1ffbfda    LRU:101545

*/

void printCache(cacheInfo_t *cacheInfo) {
  int S = 1 << cacheInfo->s;
  int E = cacheInfo->E;
  int size = S * E;
  // print hits,misses,evictions
  printf("Hits: %d, ", cacheInfo->hits);
  printf("Misses: %d, ", cacheInfo->misses);
  printf("Evictions: %d\n", cacheInfo->evictions);
  // for sets
  for (int i = 0; i < size; i++) {
    printf("S:%-2d L:%-2d V:%-2d T:%-10lx LRU:%-10ld\n", i/E, i%E,
    cacheInfo->cache[i].valid, cacheInfo->cache[i].tag, cacheInfo->cache[i].lru);
  }
    return;
}

/* This is the code that simulates the cache access.  The mode is a
   character with the value of either 'L', 'S', or 'M'.  address is
   the memory address to lookup, and cacheInfo is our cacheInfo
   struct.  You should modify the cache to reflect this memory access
   and update the hits, misses, and evictions in the cacheInfo
   accordingly.

 */

void accessCache(char mode, unsigned long address, cacheInfo_t *cacheInfo) {
  int minLRULineIndex = 0;
  // initialize minimumLRU to maxLRU
  int minimumLRU = cacheInfo->maxLRU;
  long s = cacheInfo->s;
  long E = cacheInfo->E;
  long b = cacheInfo->b;
  // create a mask of ones where the set bits would be
  long setBitMask = ~((~(~0 << b)) ^ (~0 << (b + s)));
  // get the set from the address w/ the bitmask, multiply it by the lines per set
  long set = ((setBitMask & address) >> b) * E;
  // get the tag by shifting out the set and block bits
  long tag = address >> (b + s);

  // add one to hits if the mode is modify
  // as loads/stores are processed the same but modify is different as it adds one more hit
  if (mode == 'M') {
    cacheInfo->hits++;
  }

  // loop through each line
  for(int i = 0; i < E; i++) {
    ///////////////// to find the minimumLRU line index w/in the set
    // iterate through the set any replace the minimun value as necessary
    if (cacheInfo->cache[set + i].lru < minimumLRU) {
      minLRULineIndex = i;
      minimumLRU = cacheInfo->cache[set + i].lru;
    }
    /////////////////
    // cache hit? if so, increment hits, and update lru
    if((cacheInfo->cache[set + i].valid == 1) &&
      (tag == cacheInfo->cache[set + i].tag)) {
        cacheInfo->cache[set +i].lru = cacheInfo->maxLRU++;
        cacheInfo->hits++;
        return;
      }
  }

  // no hit so increment Misses,
  cacheInfo->misses++;
  // then loop through each line in a set and look for invalid line to cache it at instead
  for (int j = 0; j < E; j++){
    if (cacheInfo->cache[set + j].valid == 0) {
      // make valid; set tag; update lru val
      cacheInfo->cache[set + j].valid = 1;
      cacheInfo->cache[set + j].tag = tag;
      cacheInfo->cache[set + j].lru = cacheInfo->maxLRU++;
      return;
    }
  }
  // if the lines are all valid, use the line of the minimum LRU to decide eviction
  // and increment evictions
  cacheInfo->evictions++;
  cacheInfo->cache[set + minLRULineIndex].lru = cacheInfo->maxLRU++;
  cacheInfo->cache[set + minLRULineIndex].tag = tag;

}

// initialize the cache
// called once before running the cache simulation
int cacheInit(cacheInfo_t *cacheInfo) {
    int numSets;  // number of sets in the cache
    int numLines; // number of lines in the cache

    // initialize our cache stats
    cacheInfo->hits = 0;
    cacheInfo->misses = 0;
    cacheInfo->evictions = 0;
    // initialize the added cache stat
    cacheInfo->maxLRU = 1;

    // get the number of sets in our cache
    // S = 2^s
    // use left shift
    numSets = 1 << cacheInfo->s;

    // get the number of lines in our cache
    // E * numSets
    numLines = cacheInfo->E * numSets;
    // think of as an array of cache lines i.e. (cacheLine_t*) example ln118
    // Allocate the lines for our cache
    cacheInfo->cache = (cacheLine_t*)malloc(numLines * sizeof(cacheLine_t));
    if (cacheInfo->cache == NULL) {
        printf("Could not allocate cache lines\n");
        return 1;
    }
    else {
        // Initialize the lines in the cache
        for (int i = 0; i < numLines; i++) {
            cacheInfo->cache[i].tag = 0;
            cacheInfo->cache[i].valid = 0;
            cacheInfo->cache[i].lru = 0;
        }
    }
    return 0;
}

// load the file and loop through one line at a time
// return 1 on error
// you do not need to modify this function
int runSimulator(cacheInfo_t *cacheInfo) {
    FILE *fp;          // file object
    char buf[MAXLINE]; // holds a line of text
    char mode;         // memory access: 'L', 'S', or 'M'
    unsigned long address; // memory address of access
    int numRead;       // number of arguments read
    int numBytes;      // number of bytes read from the memory access

  // open the trace file read only
  fp = fopen(cacheInfo->trace, "r");
  if (!fp) {
    printf("Cound not open file %s.\n", cacheInfo->trace);
    return 1;
  }

  // read through the tracefile one line at a time until end
  while((fgets(buf, MAXLINE, fp)) != NULL) {
      // convert a line of the trace to variables
      numRead = sscanf(buf, " %c %lx,%d", &mode, &address, &numBytes);
      if (numRead == 3 && mode != 'I') {
          // valid request, process it
          if (cacheInfo->verbose) {
              printf("%c %lx,%d\n", mode, address, numBytes);
          }
          accessCache(mode, address, cacheInfo);
          if (cacheInfo->verbose) {
              printCache(cacheInfo);
          }
      }
  }
  // finished with file, close it
  fclose(fp);
  return 0;
}

// get command line opts for the program
// return 0 if all opts are set.
// return 1 if -h was specified or args missing or incorrect
// you do not need to modify this function
int getOptions(cacheInfo_t *cacheInfo, int argc, char* argv[]) {
  int c;  // holds the argument for getopt

  // initialize E, s, b, verbose, trace
  cacheInfo->E = -1;
  cacheInfo->s = -1;
  cacheInfo->b = -1;
  cacheInfo->verbose = 0;     // default is no verbosity
  cacheInfo->trace[0] = '\0'; // empty string

  while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
      switch(c) {
      case 'h':
          return 1;
      case 'v':
          cacheInfo->verbose = 1;
          break;
      case 's':
          c = atoi(optarg);
          if (c >= 0) {
              cacheInfo->s = c;
          }
          break;
      case 'E':
          c = atoi(optarg);
          if (c > 0) {
              cacheInfo->E = c;
          }
          break;
      case 'b':
          c = atoi(optarg);
          if (c >= 0) {
              cacheInfo->b = c;
          }
          break;
      case 't':
          strncpy(cacheInfo->trace, optarg, MAXFILE);
          break;
      }
  }
  // did we get all of our required arguments
  if (cacheInfo->E == -1 || cacheInfo->s == -1 || cacheInfo->b == -1 ||
      cacheInfo->trace[0] == '\0') {
      return 1;
  }
  else {
      return 0;
  }
}

// print out usage message on help or error
// you do not need to modify this function
void printUsage(char *progName) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", progName);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", progName);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", progName);
}

// our program starts here
// you do not need to modify this function
int main(int argc, char *argv[]) {
    // cacheInfo is our main structure
    // it holds our cache and extra information needed for the simulator
    cacheInfo_t cacheInfo;

    // get our command line options
    // return 0 on success
    if (getOptions(&cacheInfo, argc, argv)) {
        printUsage(argv[0]);
        return 1;
    }

    // initialize our cache
    // return 0 on success
    if (cacheInit(&cacheInfo)) {
        // Error!
        printf("Error initializing cache\n");
        return 1;
    }

    // run our simulation
    // return 0 on success
    if (runSimulator(&cacheInfo)) {
        printf("Error running cache simulator\n");
        return 1;
    }
    else {
        // Success
        printSummary(cacheInfo.hits, cacheInfo.misses, cacheInfo.evictions);
        return 0;
    }
}
