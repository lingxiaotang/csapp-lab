#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
static int rowNum=0;
static int columnNum=0;
uint64_t aHeader=0;
uint64_t bHeader=0;

int s=5,b=5;
uint64_t getSetIndex(uint64_t address)
{
    return (address >> b) & ((1 << s) - 1);
}

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

void setTestEnvironment(int rnumber,int col_num,char* aAddress,char* bAddress)
{
    rowNum=rnumber;
    columnNum=col_num;
    aHeader=hexToDec(aAddress);
    bHeader=hexToDec(bAddress);
}

void parse(char*buff)
{
    char temp[10];
    int i=0,j=0;
    int hasMetZero=0;
    while(1)
    {
        if(buff[j]=='0'&&!hasMetZero)
        {
            temp[i++]=buff[j];
            hasMetZero=1;
        }
        else if(buff[j]==',')
            break;
        else if(hasMetZero)
            temp[i++]=buff[j];
        j++;
    }
    temp[i]='\0';
    strcpy(buff,temp);
}


int main()
{

    setTestEnvironment(64,64,"0030b060","0034b060");

    FILE*fp=fopen("trace.f0","r");
    if(fp==NULL)
    {
        printf("open file failed!\n");
        exit(0);
    }

    char buffer[100];
    while(fgets(buffer,100,fp)!=NULL)
    {
        parse(buffer);
        if(buffer[3]=='0')
        {
            printf("%s ",buffer);
            uint64_t thisAddress=hexToDec(buffer);
            uint64_t offset=(thisAddress-aHeader)/4;
            long row=offset/columnNum;
            long column=(offset-(row)*columnNum);
            uint64_t setIndex=getSetIndex(thisAddress);
            printf("a[%ld][%ld] set index:%ld\n",row,column,setIndex);
        }
        else{
            printf("%s ",buffer);
            uint64_t thisAddress=hexToDec(buffer);
            uint64_t offset=(thisAddress-bHeader)/4;
            long row=offset/columnNum;
            long column=(offset-(row)*columnNum);
            uint64_t setIndex=getSetIndex(thisAddress);
            printf("b[%ld][%ld] set index:%ld\n",row,column,setIndex);
        }
    }
}