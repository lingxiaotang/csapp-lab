#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <malloc.h>
#include "cachelab.h"

//here defines some macros for dealing with load,store and modify
#define LOAD 0
#define STORE 1
#define MODIFY 2
#define MAXLINE 100
//define the result of the fetch progress
#define HIT 0
#define MISS 1
#define EVICT 2
//define a global variable to indicate whether the process is visible
static int isVisible = 0;

//here defines some global datas
//S is the number of sets
uint64_t s = 0, S = 0;
//E is the number of lines per set;
uint64_t E = 0;
//b is the number of block bits and B is the block size
uint64_t b = 0, B = 0;
//fileName is the name of the input file
char fileName[20];

//here defines some data structures for the cache
//define the line of a set
typedef struct
{
    int isValid;
    uint64_t tagIndex;
    uint64_t time;
} line;

//define the set
typedef struct
{
    line *lines;
    uint64_t setIndex;
} set;

//define the cache
typedef struct
{
    set *sets;
} cache;

//here defines some variables to store the result
static int hitNum = 0, missNum = 0, evictNum = 0;

//parse command line parameters and save them to local variables
void parseCommandLine(int argc, char *argv[]);
cache *buildCache();
void destoryCache(cache *thisCache);
uint64_t getSetIndex(uint64_t address);
uint64_t getTagIndex(uint64_t address);

// to parse a file line,the address and the command type will be stored in address and  commandType variable
void parseLine(char *commandLine, uint64_t *address, int *commandType);
//deal with the cache processing process
void DealProcess(uint64_t *address, int *commandType, cache *thisCache, char *commandLine);
//check whether the address is in cache
int hasDataInCache(cache *thisCache, uint64_t setIndex, uint64_t tagIndex);
//find the unused line in the current set,returns -1 if there is no empty set
int hasEmptyLineInSet(cache *thisCache, uint64_t setIndex);
//add time for all valid lines
void addTime(cache *thisCache);
//help function for the LFU strategy
int findTheLeastFrequencyLine(cache *thisCache, uint64_t setIndex);
long hexToDec(char *source);
int getIndexOfSigns(char ch);
//when an instruction occurs,update the data block in the cache
void updateCache(cache *thisCache, uint64_t setIndex, uint64_t tagIndex, char *commandLine, int *status);
//print the processing process to the terminal
void printProcess(char *commandLine, int *status, int isPrintCommandLine, int isPrintNewLine);

int main(int argc, char *argv[])
{
    parseCommandLine(argc, argv);
    FILE *fp;
    char commandLine[MAXLINE];
    if ((fp = fopen(fileName, "r")) == NULL)
    {
        puts("File open error!\nPlease check whether file exists!");
        return 0;
    }
    cache *thisCache = buildCache();
    while (fgets(commandLine, MAXLINE, fp) != NULL)
    {
        uint64_t address = -1;
        int commandType = -1;
        parseLine(commandLine, &address, &commandType);
        if (address == -1 || commandType == -1)
            continue;
        DealProcess(&address, &commandType, thisCache, commandLine + 1);
    }
    printSummary(hitNum, missNum, evictNum);
}

void parseCommandLine(int argc, char *argv[])
{
    int ch = 0;
    char *helpMessage = "./csim-ref: Missing required command line argument\n"
                        "Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
                        "Options:\n"
                        "-h         Print this help message.\n"
                        "-v         Optional verbose flag.\n"
                        "-s <num>   Number of set index bits.\n"
                        "-E <num>   Number of lines per set.\n"
                        "-b <num>   Number of block offset bits.\n"
                        "-t <file>  Trace file.\n"

                        "Examples:\n"
                        "linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
                        "linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace";
    opterr = 0;
    while ((ch = getopt(argc, argv, "h::v::s:E:b:t:")) != -1)
    {
        switch (ch)
        {
        case 'h':
            puts(helpMessage);
            exit(0);
            break;
        case 'v':
            isVisible = 1;
            break;
        case 's':
            s = atol(optarg);
            S = pow(2, s);
            break;
        case 'E':
            E = atol(optarg);
            break;
        case 'b':
            b = atol(optarg);
            B = pow(2, b);
            break;
        case 't':
            strcpy(fileName, optarg);
            break;
        case '?': //some error occurs
            puts("error in the parameters!Please reenter the parameters!");
        default:
            puts(helpMessage);
            exit(0);
            break;
        }
    }
}

cache *buildCache()
{
    cache *thisCache = (cache *)malloc(sizeof(cache));
    thisCache->sets = (set *)malloc(S * sizeof(set));
    for (int i = 0; i < S; i++)
    {
        thisCache->sets[i].setIndex = i;
        thisCache->sets[i].lines = (line *)malloc(E * sizeof(line));
        for (int j = 0; j < E; j++)
        {
            thisCache->sets[i].lines[j].isValid = 0;
        }
    }
    return thisCache;
}

void destoryCache(cache *thisCache)
{
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            free(thisCache->sets[i].lines);
        }
    }

    for (int i = 0; i < S; i++)
        free(thisCache->sets);

    free(thisCache);
}

uint64_t getSetIndex(uint64_t address)
{
    return (address >> b) & ((1 << s) - 1);
}

uint64_t getTagIndex(uint64_t address)
{
    return address >> (s + b);
}

void parseLine(char *commandLine, uint64_t *address, int *commandType)
{
    // remove I
    if (commandLine[0] != ' ')
        return;
    int i = 1;
    if (commandLine[i] == 'M')
        *commandType = MODIFY;
    if (commandLine[i] == 'L')
        *commandType = LOAD;
    if (commandLine[i] == 'S')
        *commandType = STORE;
    i = 3;
    int j = 0;
    char addressLine[MAXLINE];
    while (commandLine[i] != ',')
    {
        addressLine[j++] = commandLine[i++];
    }
    addressLine[j] = '\0';
    *address = hexToDec(addressLine);
}

long hexToDec(char *source)
{
    long sum = 0;
    long t = 1;
    int i, len;

    len = strlen(source);
    for (i = len - 1; i >= 0; i--)
    {
        sum += t * getIndexOfSigns(*(source + i));
        t *= 16;
    }
    return sum;
}

/* 返回ch字符在sign数组中的序号 */
int getIndexOfSigns(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    return -1;
}

void DealProcess(uint64_t *address, int *commandType, cache *thisCache, char *commandLine)
{
    uint64_t setIndex = getSetIndex(*address);
    uint64_t tagIndex = getTagIndex(*address);
    int status = 0;
    if (*commandType == LOAD)
    {
        updateCache(thisCache, setIndex, tagIndex, commandLine, &status);
        if (isVisible)
            printProcess(commandLine, &status, 1, 1);
    }
    if (*commandType == STORE)
    {
        updateCache(thisCache, setIndex, tagIndex, commandLine, &status);
        if (isVisible)
            printProcess(commandLine, &status, 1, 1);
    }
    if (*commandType == MODIFY)
    {
        updateCache(thisCache, setIndex, tagIndex, commandLine, &status);
        if (isVisible)
            printProcess(commandLine, &status, 1, 0);
        updateCache(thisCache, setIndex, tagIndex, commandLine, &status);
        if (isVisible)
            printProcess(commandLine, &status, 0, 1);
    }
}

void updateCache(cache *thisCache, uint64_t setIndex, uint64_t tagIndex, char *commandLine, int *status)
{
    addTime(thisCache);
    if (hasDataInCache(thisCache, setIndex, tagIndex))
    {
        hitNum++;
        *status = HIT;
    }
    else
    {
        missNum++;
        *status = MISS;
        int emptyLineIndex = hasEmptyLineInSet(thisCache, setIndex);
        if (emptyLineIndex != -1)
        {
            thisCache->sets[setIndex].lines[emptyLineIndex].isValid = 1;
            thisCache->sets[setIndex].lines[emptyLineIndex].tagIndex = tagIndex;
            thisCache->sets[setIndex].lines[emptyLineIndex].time = 0;
        }
        else
        {
            *status = EVICT;
            evictNum++;
            int line = findTheLeastFrequencyLine(thisCache, setIndex);
            thisCache->sets[setIndex].lines[line].tagIndex = tagIndex;
            thisCache->sets[setIndex].lines[line].time = 0;
        }
    }
}

int hasDataInCache(cache *thisCache, uint64_t setIndex, uint64_t tagIndex)
{
    for (int i = 0; i < E; i++)
    {
        if (thisCache->sets[setIndex].lines[i].isValid && thisCache->sets[setIndex].lines[i].tagIndex == tagIndex)
        {
            thisCache->sets[setIndex].lines[i].time = 0;
            return 1;
        }
    }
    return 0;
}

int hasEmptyLineInSet(cache *thisCache, uint64_t setIndex)
{
    for (int i = 0; i < E; i++)
    {
        if (!thisCache->sets[setIndex].lines[i].isValid)
            return i;
    }
    return -1;
}

void addTime(cache *thisCache)
{
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            if (thisCache->sets[i].lines[j].isValid)
                thisCache->sets[i].lines[j].time++;
        }
    }
}

int findTheLeastFrequencyLine(cache *thisCache, uint64_t setIndex)
{
    int resultIndex = 0;
    uint64_t time = 0;
    for (int i = 0; i < E; i++)
    {
        if (thisCache->sets[setIndex].lines[i].time > time)
        {
            resultIndex = i;
            time = thisCache->sets[setIndex].lines[i].time;
        }
    }
    return resultIndex;
}

void printProcess(char *commandLine, int *status, int isPrintCommandLine, int isPrintNewLine)
{
    if (*status == HIT)
    {
        if (isPrintCommandLine)
            printf("%s ", commandLine);
        printf(" hit");
        if (isPrintNewLine)
            printf("\n");
    }

    if (*status == MISS)
    {
        if (isPrintCommandLine)
            printf("%s ", commandLine);
        printf(" miss");
        if (isPrintNewLine)
            printf("\n");
    }

    if (*status == EVICT)
    {
        if (isPrintCommandLine)
            printf("%s ", commandLine);
        printf("miss ");
        printf(" eviction");
        if (isPrintNewLine)
            printf("\n");
    }
}