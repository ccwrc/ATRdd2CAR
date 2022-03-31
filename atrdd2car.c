/*--------------------------------------------------------------------*/
/* ATRdd2CAR                                                          */
/* by GienekP                                                         */
/* (c) 2022                                                           */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
typedef unsigned char U8;
/*--------------------------------------------------------------------*/
#define CARSIZE (512*1024)
#define LDRSIZE (8*1024)
#define ATRSIZE (CARSIZE-LDRSIZE-128*5)
/*--------------------------------------------------------------------*/
#include "starter.h"
/*--------------------------------------------------------------------*/
U8 checkATR(const U8 *data)
{
	U8 ret=0;
	if ((data[0]==0x96) && (data[1]==02) && (data[4]==0x00) && (data[5]==0x01))
	{
		ret=1;
	};
	return ret;
}
/*--------------------------------------------------------------------*/
U8 loadATR(const char *filename, U8 *data)
{
	U8 header[16];
	U8 ret=0;
	int i;
	FILE *pf;
	for (i=0; i<ATRSIZE; i++)
	{
		data[i]=0xFF;
	};
	pf=fopen(filename,"rb");
	if (pf)
	{
		i=fread(header,sizeof(U8),16,pf);
		if (i==16)
		{
			if (checkATR(header))
			{
				i=fread(data,sizeof(U8),ATRSIZE,pf);
				ret=1;
			}
			else
			{
				printf("Unknown ATR SD header.\n");
			};
		}
		else
		{
			printf("Wrong ATR header size.\n");
		}
		fclose(pf);
	}
	else
	{
		printf("\"%s\" does not exist.\n",filename);
	};
	return ret;
}
/*--------------------------------------------------------------------*/
U8 saveCAR(const char *filename, U8 *data, unsigned int sum)
{
	U8 header[16]={0x43, 0x41, 0x52, 0x54, 0x00, 0x00, 0x00, 0x25,
		           0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
	U8 ret=0;
	int i;
	FILE *pf;
	header[8]=((sum>>24)&0xFF);
	header[9]=((sum>>16)&0xFF);
	header[10]=((sum>>8)&0xFF);
	header[11]=(sum&0xFF);
	pf=fopen(filename,"wb");
	if (pf)
	{
		i=fwrite(header,sizeof(U8),16,pf);
		if (i==16)
		{
			i=fwrite(data,sizeof(U8),CARSIZE,pf);
			if (i==CARSIZE)
			{
				ret=1;
			};			
		};
	};
	return ret;
}
/*--------------------------------------------------------------------*/
unsigned int buildCar(const U8 *loader, unsigned int stsize, const U8 *atrdata, U8 *cardata)
{
	unsigned int i,j,sum=0,toend=stsize;
	for (i=0; i<CARSIZE; i++)
	{
		cardata[i]=0xFF;
	};
	for (j=1; j<4; j++)
	{
		for (i=0; i<128; i++)
		{
			cardata[j*256+i]=atrdata[(j-1)*128+i];
		};
	};
	for (i=(3*128); i<ATRSIZE; i++)
	{
		cardata[i+(4*256)-(3*128)]=atrdata[i];
	};
	if (toend>LDRSIZE)
	{
		toend=LDRSIZE;
	};
	for (i=0; i<toend; i++)
	{
		cardata[CARSIZE-toend+i]=loader[stsize-toend+i];
	};
	for (i=0; i<CARSIZE; i++)
	{
		sum+=cardata[i];
	};
	return sum;
}
/*--------------------------------------------------------------------*/
void first(U8 mode, U8 *atrdata, unsigned int i)
{
	static U8 f=0;
	if (mode=='c')
	{
		atrdata[i+1]=0x00;
		atrdata[i+2]=0x01;
	};
	if (f==0)
	{
		f=1;
		if (mode=='c')
		{
			printf("Replace calls:\n");
		}
		else
		{
			printf("Possible calls:\n");
		};
	};
}
/*--------------------------------------------------------------------*/
void checkJDSKINT(U8 *atrdata, U8 mode)
{
	unsigned int i;
	for (i=0; i<ATRSIZE-3; i++)
	{
		if ((atrdata[i+1]==0x53) && (atrdata[i+2]==0xE4))
		{
			if (atrdata[i]==0x20)
			{
				first(mode,atrdata,i);
				printf(" JSR JDSKINT ; 0x%06X 20 53 E4 -> 20 00 01\n",i+16);
				
			};
			if (atrdata[i]==0x4C)
			{
				first(mode,atrdata,i);
				printf(" JMP JDSKINT ; 0x%06X 4C 53 E4 -> 4C 00 01\n",i+16);
			};			
		};
		if ((atrdata[i+1]==0xB3) && (atrdata[i+2]==0xC6))
		{
			if (atrdata[i]==0x20)
			{
				first(mode,atrdata,i);
				printf(" JSR DSKINT ; 0x%06X 20 B3 C6 -> 20 00 01\n",i+16);
			};
			if (atrdata[i]==0x4C)
			{
				first(mode,atrdata,i);
				printf(" JMP DSKINT ; 0x%06X 4C B3 C6 -> 4C 00 01\n",i+16);
			};			
		};
		if ((atrdata[i+1]==0x59) && (atrdata[i+2]==0xE4))
		{
			if (atrdata[i]==0x20)
			{
				first(mode,atrdata,i);
				printf(" JSR JSIOINT ; 0x%06X 20 59 E4 -> 20 00 01\n",i+16);
			};
			if (atrdata[i]==0x4C)
			{
				first(mode,atrdata,i);
				printf(" JMP JSIOINT ; 0x%06X 4C 59 E4 -> 4C 00 01\n",i+16);
			};			
		};		
	};
}
/*--------------------------------------------------------------------*/
void atrdd2car(const char *atrfn, const char *carfn, U8 mode)
{
	unsigned int sum;
	U8 atrdata[ATRSIZE];
	U8 cardata[CARSIZE];
	if (loadATR(atrfn,atrdata))
	{
		printf("Load \"%s\"\n",atrfn);
		checkJDSKINT(atrdata,mode);
		sum=buildCar(starter_bin,starter_bin_len,atrdata,cardata);
		if (saveCAR(carfn,cardata,sum))
		{
			printf("Save \"%s\"\n",carfn);
		}
		else
		{
			printf("Save \"%s\" ERROR!\n",carfn);
		};
	}
	else
	{
		printf("Load \"%s\" ERROR!\n",atrfn);
	};
}
/*--------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{	
	printf("ATRdd2CAR - ver: %s\n",__DATE__);
	if (argc==3)
	{
		atrdd2car(argv[1],argv[2],0);
	}
	else if (argc==4)
	{
		char *ptr;
		ptr=argv[3];
		U8 mode=ptr[1];
		atrdd2car(argv[1],argv[2],mode);
	}
	else
	{
		printf("(c) GienekP\n");
		printf("use:\natrdd2car game.atr game.car [-c]\n");
	};
	return 0;
}
/*--------------------------------------------------------------------*/
