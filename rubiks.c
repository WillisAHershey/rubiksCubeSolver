//Willis A. Hershey
//WillisAHershey@gmail.com

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//The following preprocessor directive allows older POSIX systems that don't have compiler support for the
//C11 threads.h library to compile using pthread.h, since there is an almost one-to-one correspondence
#ifdef __STDC_NO_THREADS__
#	define THREAD_RETURN void*
#	include <pthread.h>
#	define thrd_t pthread_t
#	define thrd_create(a,b,c) pthread_create(a,NULL,b,c)
#	define thrd_join(a,b) pthread_join(a,b)
#	define mtx_t pthread_mutex_t
#	define mtx_init(a,b) pthread_mutex_init(a,NULL)
#	define mtx_lock(a) pthread_mutex_lock(a)
#	define mtx_unlock(a) pthread_mutex_unlock(a)
#	define mtx_destroy(a) pthread_mutex_destroy(a)
#else
#	define THREAD_RETURN int
#	include <threads.h>
#endif

//The following preprocessor directive allows the compiler to conditionally compile code for systems that
//don't have atomic read/write access to pointers
//This feature is here for listMatch(), which has never caused problems that I've noticed, but theoretically could
/*
#ifndef __STDC_NO_ATOMICS__
#	include <stdatomic.h>
#	if !ATOMIC_ADDRESS_LOCK_FREE
#		define	RESPECT_MUTEX
#	endif
#else
#	define	RESPECT_MUTEX
#endif
*/
#define NUM_THREADS 16 

//enum color represents a mapping between the six colors of a Rubik's cube and integers 0-5
//Notice that white and w are equivalent, as are blue and b etc.
enum color {white=0,blue=1,green=2,yellow=3,red=4,orange=5,w=0,b=1,g=2,y=3,r=4,o=5};

//A state is 48 colors in a specific order, which defines the placement of all of the colors on the cube
//The use of enum instead of another int type allows a reasonably advanced compiler to pack this struct more tightly and save memory
typedef struct{
  enum color c[48];
}state;

//Compares the two states referenced by the input pointers, and does not bother to compare anything before index tier
//Returns tier where difference occurs times negative one if a<b, 0 if a=b, and positive tier where difference occurs if a>b. This allows us to order states
//This return value nonsense helps quite a bit to optimize listMatch() (notice output 'tier' is index of difference + 1)
static inline int compareStates(state *a,state *b,int tier){
  if(tier<0)
	tier=0;
  for(int i=tier;i<48;++i)
	if(a->c[i] != b->c[i])
		return a->c[i] < b->c[i] ? -(i+1) : i+1;
  return 0;
}

//This allows us to print states for debugging purposes
void printState(state *in){
  printf("{");
  for(int c=0;c<48;++c){
	switch(in->c[c]){
		case white:
			printf("w");
			break;
		case blue:
			printf("b");
			break;
		case green:
			printf("g");
			break;
		case yellow:
			printf("y");
			break;
		case red:
			printf("r");
			break;
		case orange:
			printf("o");
			break;
	}
	if(c!=47)
		printf(",");
  }
  printf("}\n");
}

//This is the solved state, the state we attempt to achieve
const state solved=(state){.c={w,w,w,w,w,w,w,w,b,b,b,b,b,b,b,b,g,g,g,g,g,g,g,g,y,y,y,y,y,y,y,y,r,r,r,r,r,r,r,r,o,o,o,o,o,o,o,o}}; 

//Contains a state, pointers to all 18 possible children states (in the order of the functions below), tier of node in tree, and side it was computed on by parent
typedef struct stateTreeNodeStruct{
  unsigned int tier:4;
  unsigned int side:3;
  state s;
  struct stateTreeNodeStruct *children[18];
}stateTreeNode;

//The stateList is an m-ary tree with an m of 6, because of the six-sided six-colored Rubik's cube. It is designed to hold the states in order of magnitude so that they
//can be found to be unique or duplicate without searching the entire tree, and with as few pointer jumps as possible. The head stateListNode contains six state pointers,
//the first of which points to the lowest state such that the first color in the state is white (or zero), the second points to the lowest state such that the first color
//is blue (or one), so on and so forth. It also contains six stateListNode pointers, the first of which points to the head of the tree such that all states begin with white
//(or zero), the second of which points to the head of the tree such that all states begin with blue (or one) so on and so forth.

//States are ordered lowest to highest as if the states were 48-digit big-endian base-6 numbers, so that the zeroth element of the state array is most significant

//For instance, for some sufficiently full stateList, a pointer to the lowest state found that begins {white,blue,orange,blue... could be found by following pointers
//zero, one, and then five, and the second state pointer in that node would be what we want. The second stateListNode pointer on that node would then point to the tree
//containing all larger states that begin {white,blue,orange,blue...

//Node for m-ary tree of visited states
typedef struct stateListNodeStruct{
  stateTreeNode *s[6];
  struct stateListNodeStruct *next[6];
}stateListNode;

//Thread-safe structure for m-ary tree of visited states
typedef struct{
  stateListNode *head;
  mtx_t mutex;
}stateList;

//This is meant to be a literal stored in the data section of the executable, so we can memcpy() NULLs into an array, instead of using a loop which probs saves time
const stateTreeNode *eightteenNulls[]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
//Same story here. This is to use memcpy() into new stateListNodes in the m-ary tree stateList
const stateListNode emptyStateListNode=(stateListNode){.s={NULL,NULL,NULL,NULL,NULL,NULL},.next={NULL,NULL,NULL,NULL,NULL,NULL}};

//Realistically eighteenNulls could almost certainly be used also for the purposes of emptyStateListNode, but since it's not guaranteed that NULL is 0x0, and I have no
//way of knowing how struct packing works in every single system, I'll spend the extra few dozen bytes to ensure no wonky impossible-to-find bugs.

//The following eightteen functions mutate the input state to simulate some specific turn of the cube and save the resulting state at the given address
//These functions receive a state as an input, and write the output mutated state to some given memory location
//Despite their one-lined-ness, they are each rather expensive functions

void faceClock(state in,state *out){
  *out=(state){.c={in.c[5],in.c[3],in.c[0],in.c[6],in.c[1],in.c[7],in.c[4],in.c[2],in.c[40],in.c[41],in.c[42],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[32],in.c[33],in.c[34],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[8],in.c[9],in.c[10],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[16],in.c[17],in.c[18],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void faceCounter(state in,state *out){
  *out=(state){.c={in.c[2],in.c[4],in.c[7],in.c[1],in.c[6],in.c[0],in.c[3],in.c[5],in.c[32],in.c[33],in.c[34],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[40],in.c[41],in.c[42],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[16],in.c[17],in.c[18],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[8],in.c[9],in.c[10],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void faceTwice(state in,state *out){
  *out=(state){.c={in.c[7],in.c[6],in.c[5],in.c[4],in.c[3],in.c[2],in.c[1],in.c[0],in.c[16],in.c[17],in.c[18],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[8],in.c[9],in.c[10],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[40],in.c[41],in.c[42],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[32],in.c[33],in.c[34],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void leftClock(state in,state *out){
  *out=(state){.c={in.c[39],in.c[1],in.c[2],in.c[36],in.c[4],in.c[34],in.c[6],in.c[7],in.c[13],in.c[11],in.c[8],in.c[14],in.c[9],in.c[15],in.c[12],in.c[10],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[45],in.c[27],in.c[43],in.c[29],in.c[30],in.c[40],in.c[32],in.c[33],in.c[26],in.c[35],in.c[28],in.c[37],in.c[38],in.c[31],in.c[0],in.c[41],in.c[42],in.c[3],in.c[44],in.c[5],in.c[46],in.c[47]}};
}

void leftCounter(state in,state *out){
  *out=(state){.c={in.c[40],in.c[1],in.c[2],in.c[43],in.c[4],in.c[45],in.c[6],in.c[7],in.c[10],in.c[12],in.c[15],in.c[9],in.c[14],in.c[8],in.c[11],in.c[13],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[34],in.c[27],in.c[36],in.c[29],in.c[30],in.c[39],in.c[32],in.c[33],in.c[5],in.c[35],in.c[3],in.c[37],in.c[38],in.c[0],in.c[31],in.c[41],in.c[42],in.c[28],in.c[44],in.c[26],in.c[46],in.c[47]}};
}

void leftTwice(state in,state *out){
  *out=(state){.c={in.c[31],in.c[1],in.c[2],in.c[28],in.c[4],in.c[26],in.c[6],in.c[7],in.c[15],in.c[14],in.c[13],in.c[12],in.c[11],in.c[10],in.c[9],in.c[8],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[21],in.c[22],in.c[23],in.c[24],in.c[25],in.c[5],in.c[27],in.c[3],in.c[29],in.c[30],in.c[0],in.c[32],in.c[33],in.c[45],in.c[35],in.c[43],in.c[37],in.c[38],in.c[40],in.c[39],in.c[41],in.c[42],in.c[36],in.c[44],in.c[34],in.c[46],in.c[47]}};
}

void rightClock(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[42],in.c[3],in.c[44],in.c[5],in.c[6],in.c[47],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[21],in.c[19],in.c[16],in.c[22],in.c[17],in.c[23],in.c[20],in.c[18],in.c[32],in.c[25],in.c[26],in.c[35],in.c[28],in.c[37],in.c[30],in.c[31],in.c[7],in.c[33],in.c[34],in.c[4],in.c[36],in.c[2],in.c[38],in.c[39],in.c[40],in.c[41],in.c[29],in.c[43],in.c[27],in.c[45],in.c[46],in.c[24]}};
}

void rightCounter(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[37],in.c[3],in.c[35],in.c[5],in.c[6],in.c[32],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[18],in.c[20],in.c[23],in.c[17],in.c[22],in.c[16],in.c[19],in.c[21],in.c[47],in.c[25],in.c[26],in.c[44],in.c[28],in.c[42],in.c[30],in.c[31],in.c[24],in.c[33],in.c[34],in.c[27],in.c[36],in.c[29],in.c[38],in.c[39],in.c[40],in.c[41],in.c[2],in.c[43],in.c[4],in.c[45],in.c[46],in.c[7]}};
}

void rightTwice(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[29],in.c[3],in.c[27],in.c[5],in.c[6],in.c[24],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[13],in.c[14],in.c[15],in.c[23],in.c[22],in.c[21],in.c[20],in.c[19],in.c[18],in.c[17],in.c[16],in.c[7],in.c[25],in.c[26],in.c[4],in.c[28],in.c[2],in.c[30],in.c[31],in.c[47],in.c[33],in.c[34],in.c[44],in.c[36],in.c[42],in.c[38],in.c[39],in.c[40],in.c[41],in.c[37],in.c[43],in.c[35],in.c[45],in.c[46],in.c[32]}};
}

void rearClock(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[37],in.c[38],in.c[39],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[45],in.c[46],in.c[47],in.c[29],in.c[27],in.c[24],in.c[30],in.c[25],in.c[31],in.c[28],in.c[26],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[21],in.c[22],in.c[23],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[13],in.c[14],in.c[15]}};
}

void rearCounter(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[45],in.c[46],in.c[47],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[37],in.c[38],in.c[39],in.c[26],in.c[28],in.c[31],in.c[25],in.c[30],in.c[24],in.c[27],in.c[29],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[13],in.c[14],in.c[15],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[21],in.c[22],in.c[23]}};
}

void rearTwice(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[8],in.c[9],in.c[10],in.c[11],in.c[12],in.c[21],in.c[22],in.c[23],in.c[16],in.c[17],in.c[18],in.c[19],in.c[20],in.c[13],in.c[14],in.c[15],in.c[31],in.c[30],in.c[29],in.c[28],in.c[27],in.c[26],in.c[25],in.c[24],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[45],in.c[46],in.c[47],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[37],in.c[38],in.c[39]}};
}

void topClock(state in,state *out){
  *out=(state){.c={in.c[18],in.c[20],in.c[23],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[2],in.c[9],in.c[10],in.c[1],in.c[12],in.c[0],in.c[14],in.c[15],in.c[16],in.c[17],in.c[24],in.c[19],in.c[25],in.c[21],in.c[22],in.c[26],in.c[13],in.c[11],in.c[8],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[37],in.c[35],in.c[32],in.c[38],in.c[33],in.c[39],in.c[36],in.c[34],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void topCounter(state in,state *out){
  *out=(state){.c={in.c[13],in.c[11],in.c[8],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[26],in.c[9],in.c[10],in.c[25],in.c[12],in.c[24],in.c[14],in.c[15],in.c[16],in.c[17],in.c[0],in.c[19],in.c[1],in.c[21],in.c[22],in.c[2],in.c[18],in.c[20],in.c[23],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[34],in.c[36],in.c[39],in.c[33],in.c[38],in.c[32],in.c[35],in.c[37],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void topTwice(state in,state *out){
  *out=(state){.c={in.c[24],in.c[25],in.c[26],in.c[3],in.c[4],in.c[5],in.c[6],in.c[7],in.c[23],in.c[9],in.c[10],in.c[20],in.c[12],in.c[18],in.c[14],in.c[15],in.c[16],in.c[17],in.c[13],in.c[19],in.c[11],in.c[21],in.c[22],in.c[8],in.c[0],in.c[1],in.c[2],in.c[27],in.c[28],in.c[29],in.c[30],in.c[31],in.c[39],in.c[38],in.c[37],in.c[36],in.c[35],in.c[34],in.c[33],in.c[32],in.c[40],in.c[41],in.c[42],in.c[43],in.c[44],in.c[45],in.c[46],in.c[47]}};
}

void bottomClock(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[15],in.c[12],in.c[10],in.c[8],in.c[9],in.c[31],in.c[11],in.c[30],in.c[13],in.c[14],in.c[29],in.c[5],in.c[17],in.c[18],in.c[6],in.c[20],in.c[7],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[16],in.c[19],in.c[21],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[45],in.c[43],in.c[40],in.c[46],in.c[41],in.c[47],in.c[44],in.c[42]}};
}

void bottomCounter(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[16],in.c[19],in.c[21],in.c[8],in.c[9],in.c[7],in.c[11],in.c[6],in.c[13],in.c[14],in.c[5],in.c[29],in.c[17],in.c[18],in.c[30],in.c[20],in.c[31],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[15],in.c[12],in.c[10],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[42],in.c[44],in.c[47],in.c[41],in.c[46],in.c[40],in.c[43],in.c[45]}};
}

void bottomTwice(state in,state *out){
  *out=(state){.c={in.c[0],in.c[1],in.c[2],in.c[3],in.c[4],in.c[29],in.c[30],in.c[31],in.c[8],in.c[9],in.c[21],in.c[11],in.c[19],in.c[13],in.c[14],in.c[16],in.c[15],in.c[17],in.c[18],in.c[12],in.c[20],in.c[10],in.c[22],in.c[23],in.c[24],in.c[25],in.c[26],in.c[27],in.c[28],in.c[5],in.c[6],in.c[7],in.c[32],in.c[33],in.c[34],in.c[35],in.c[36],in.c[37],in.c[38],in.c[39],in.c[47],in.c[46],in.c[45],in.c[44],in.c[43],in.c[42],in.c[41],in.c[40]}};
}

//This array of function pointers allows us to invoke the above 18 functions in loops
void (*const transformations[])(state,state*)={faceClock,faceCounter,faceTwice,leftClock,leftCounter,leftTwice,rightClock,rightCounter,rightTwice,rearClock,rearCounter,rearTwice,topClock,topCounter,topTwice,bottomClock,bottomCounter,bottomTwice};

//String descriptions of the transformations in the array of function pointers are stored here in the same order for simplicity and rhyme/reason's sake
const char *descriptions[]={"Face clockwise","Face counter-clockwise","Face twice","Left clockwise","Left counter-clockwise","Left twice","Right clockwise","Right counter-clockwise","Right twice","Rear clockwise","Rear counter-clockwise","Rear twice","Top clockwise","Top counter-clockwise","Top twice","Bottom clockwise","Bottom counter-clockwise","Bottom twice"};

state shuffle(int in,int verbose){ //Performs a random virtual shuffle of some input number of moves upon the solved state and returns the shuffled state
  static int seed=0;
  if(!seed)
	srand((seed=time(NULL)));
  state out;
  int r;
  (transformations[(r=rand()%18)])(solved,&out);
  if(verbose)
	printf("%s\n",descriptions[r]);
  for(int c=0;c<in-1;++c){
	int last=r;
	r=rand()%15;
	if(last/3<=r/3)
		r+=3;
	(transformations[r])(out,&out);
	if(verbose)
		printf("%s\n",descriptions[r]);
  }
  return out;
}

//Returns a pointer to the place in the list where a pointer to the state should be stored, returns NULL if state is a duplicate.
stateTreeNode** addList(stateList *list,state *s){
/*The state* that is passed to this function by buildTree() is a pointer to a temporary and contantly-changing memory location on the stack of the calling thread,
  so saving that pointer in the list would be #verybad. A permament home for the state is also not made until this function confirms it is not a duplicate.
  To remedy this problem this function behaves in a rather unorthidox way. If the state is found to be a duplicate, the mutex is unlocked, and NULL is returned.
  If it is found to be a new unique state, memory is allocated in the list where the new state belongs, a pointer to that new uninitialized memory is returned,
  and it is up to the calling function to save a pointer to the permament home of the new state at the address returned, and also unlock the mutex after the fact.
  A little messy perhaps, but the alternatives would be to incorporate this entire function in buildTree(), search the tree twice, or waste more time before unlocking
  the mutex, all of which I think are worse solutions.
  */
  mtx_lock(&list->mutex);
  stateListNode *pt=list->head;							//A stateList is simultaneously a tree and an ordered linked list. See the definitions of stateList ansd stateListNode above
  for(int tier=0;tier<48;++tier){						//Because of the design of the cube itself this tree cannot be more than 48-deep
	if(!pt->s[s->c[tier]]) 							//If the place where the state belongs in this tier is unoccupied, return it
		return &pt->s[s->c[tier]];
	int comp=compareStates(s,&pt->s[s->c[tier]]->s,tier); 			//Otherwise compare the state in the treenode there to ours (Exact retval of compareStates is not useful in this function, only the sign)
	if(!comp){ 								//If state is equal to current one in the list our state is a duplicate and we're done		
		mtx_unlock(&list->mutex);
		return NULL; 							//Return NULL because it is a duplicate
	}
	if(comp>0){ 								//If state is greater than current one in the list
		if(pt->next[s->c[tier]]) 					//Continue down the tree if there is a node where we're going
			pt=pt->next[s->c[tier]]; 
		else{ 								//Otherwise make a new node, and return a pointer to the proper spot in the new node
			stateListNode *newNode=malloc(sizeof(stateListNode));
			memcpy(newNode,&emptyStateListNode,sizeof(stateListNode));
			pt->next[s->c[tier]]=newNode;
			return &pt->next[s->c[tier]]->s[s->c[tier+1]];
		}
	}
	else{ 									//If state is less than current one in the list
		stateTreeNode **out=&pt->s[s->c[tier]]; 			//This is what we will return, but first we have to find a home for the state we displaced
		stateTreeNode *push=pt->s[s->c[tier]];				//And save the state pointer that's there because we have to find it a new home
		while(pt->next[push->s.c[tier]]){ 				//We push it down the tree. This loop breaks when we run out of tree
			pt=pt->next[push->s.c[tier]];
			++tier;
			if(!pt->s[push->s.c[tier]]){				//If the spot where the state we're pushing belongs is unoccupied put it there and we're done
				pt->s[push->s.c[tier]]=push;
				return out;
			}
			else{							//Otherwise swap out the state that's there with push and push it instead
				stateTreeNode *hold=pt->s[push->s.c[tier]];
				pt->s[push->s.c[tier]]=push;
				push=hold;
			}
		}
		stateListNode *newNode=malloc(sizeof(stateListNode));		//If we run out of tree we make more tree and return a pointer to the proper spot in it
		memcpy(newNode,&emptyStateListNode,sizeof(stateListNode));
		pt->next[push->s.c[tier]]=newNode;
		pt->next[push->s.c[tier]]->s[push->s.c[tier+1]]=push;
		return out;							//We do not change the value of the state* before we return it because listMatch() does not always respect the mutexes
	}
  }
  //If this is reached, it means that the list has no duplicate of this state, and there is also no place in the list to put it, which should be impossible
  fprintf(stderr,"Something went wrong. addList() reached impossible state\n");
  return NULL;
}

//Frees all memory associated with the stateList
void freeStateListNode(stateListNode *node){
  for(int c=0;c<6;++c)
	if(node->next[c])
  		freeStateListNode(node->next[c]);
  free(node);
}

void freeStateList(stateList *list){
  mtx_destroy(&list->mutex);
  freeStateListNode(list->head);
}

#define USE_MMAP

//This preprocessor directive allows the contiguous-memory implementation of the queue to use either mmap() or malloc()
#ifdef USE_MMAP
#	include <unistd.h>
#	include <sys/mman.h>
#	define ALLOCATE_TREE_QUEUE_NODE mmap(NULL,4*sysconf(_SC_PAGE_SIZE),PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0)
#	define DEALLOCATE_TREE_QUEUE_NODE(c) munmap(c,4*sysconf(_SC_PAGE_SIZE))
#	define TREE_QUEUE_NODE_CAPACITY ((4*sysconf(_SC_PAGE_SIZE)-sizeof(stateTreeNode*))/sizeof(stateTreeNode*))
#else
#	define ALLOCATE_TREE_QUEUE_NODE malloc(16376)
#	define DEALLOCATE_TREE_QUEUE_NODE(c) free(c)
#	define TREE_QUEUE_NODE_CAPACITY ((16376-sizeof(stateTreeNode*))/sizeof(stateTreeNode*))
#endif

//A treeQueueNode is intended to occupy some arbitrary amount of memory, wasting only sizeof(void*) for linked-list-ness
typedef struct treeQueueNodeStruct{
  struct treeQueueNodeStruct *next;
  stateTreeNode *nodes[];
}treeQueueNode;

//Thread-safe struct for bfs queue
//treeQueueAdd() mantains two queues, one with states of the current tier and one of the states of the next tier
//this is so race-conditions cannot cause us to process a higher tier node before all lower tiers are processed
typedef struct{
  treeQueueNode *head;
  treeQueueNode *tail;
  treeQueueNode *newHead;
  treeQueueNode *newTail;
  treeQueueNode *blankPage;
  unsigned int hindex;
  unsigned int tindex;
  unsigned int newIndex;
  mtx_t mutex;
}treeQueue;

//Adds a stateTreeNode* to a valid treeQueue. Most likely segfaults on memory allocation failure
void treeQueueAdd(treeQueue *queue,stateTreeNode *node){
  treeQueueNode **applicableTail;				//applicableTail/Tindex allow us to run the same code regardles of which queue we're filling
  unsigned int *applicableTindex;
  mtx_lock(&queue->mutex);
  if(queue->head==queue->tail&&queue->hindex==queue->tindex){	//Main queue is empty
	if(queue->newHead==queue->newTail&&queue->newIndex==0){	//Secondary queue is also empty
		queue->hindex=0;
		queue->tindex=1;
		queue->tail->nodes[0]=node;
		mtx_unlock(&queue->mutex);
		return;
	}
	else{							//Swap the queues
		treeQueueNode *hold=queue->head;
		queue->head=queue->newHead;
		queue->tail=queue->newTail;
		queue->hindex=0;
		queue->tindex=queue->newIndex;
		queue->newIndex=0;
		queue->newHead=queue->newTail=hold;
		applicableTail=&queue->tail;
		applicableTindex=&queue->tindex;
	}
  }
  if(node->tier > queue->head->nodes[queue->hindex]->tier){
	applicableTail=&queue->newTail;
	applicableTindex=&queue->newIndex;
  }
  else{
	applicableTail=&queue->tail;
	applicableTindex=&queue->tindex;
  }
  if(*applicableTindex == TREE_QUEUE_NODE_CAPACITY-1){			//Checking for the value before the one that causes a problem does two things to our benefit
	(*applicableTail)->nodes[TREE_QUEUE_NODE_CAPACITY-1]=node;	//1) It ensures that we don't miss a rare case where the queue is empty while tindex is 0 and hindex is max and therefore !=
	if(queue->blankPage){						//2) It ensures that treeQueueAdd() doesn't segfault when it checks the tier of the node at head
		(*applicableTail)->next=queue->blankPage;
		*applicableTail=queue->blankPage;
		queue->blankPage=NULL;
	}
	else{
		treeQueueNode *n=ALLOCATE_TREE_QUEUE_NODE;
		(*applicableTail)->next=n;
		*applicableTail=n;
	}
	*applicableTindex=0;
  }
  else
  	(*applicableTail)->nodes[(*applicableTindex)++]=node;
  mtx_unlock(&queue->mutex);
}

//Removes a stateTreeNode* from a valid treeQueue. Returns NULL when the queue is empty
stateTreeNode* treeQueueRemove(treeQueue *queue){
  mtx_lock(&queue->mutex);
  if(queue->head==queue->tail&&queue->hindex==queue->tindex){
	if(queue->newHead==queue->newTail&&queue->newIndex==0){
		mtx_unlock(&queue->mutex);
		return NULL;
	}
	else{
		treeQueueNode *hold=queue->head;
		queue->head=queue->newHead;
		queue->tail=queue->newTail;
		queue->hindex=0;
		queue->tindex=queue->newIndex;
		queue->newIndex=0;
		queue->newHead=queue->newTail=hold;
	}
  }
  stateTreeNode *out;
  if(queue->hindex == TREE_QUEUE_NODE_CAPACITY-1){
	out=queue->head->nodes[TREE_QUEUE_NODE_CAPACITY-1];
	treeQueueNode *hold=queue->head->next;
	if(!queue->blankPage)
		queue->blankPage=queue->head;
	else
		DEALLOCATE_TREE_QUEUE_NODE(queue->head);		//tree queue nodes should really only get deallocated once a solution is found
	queue->head=hold;						//because the tree grows much faster than it is depleated
	queue->hindex=0;
  }
  else
  	out=queue->head->nodes[queue->hindex++];
  mtx_unlock(&queue->mutex);
  return out;
}

//This global variable functions as a signal to all threads to stop making new nodes
//I kinda hate global varibales, tho, so there's a good chance I'll replace this mechanism with actual signals someday
static volatile int solutionFound=0;

//This function compares two stateLists in a loop until a state is found that is present in both lists. When that duplicate state is found, it is returned
state listMatch(stateList *a,stateList *b){
  struct solutionS{state *s; int tier;} solution = (struct solutionS){.s=NULL,.tier=-1};
  while(!solutionFound){
	struct listStack{int index; stateListNode *node;} astack[48],bstack[48];
	int aindex=0,bindex=0,asindex=0,bsindex=0;
	stateListNode *anode=a->head,*bnode=b->head;
	state *astate=NULL,*bstate=NULL;
#ifdef RESPECT_MUTEX
	mtx_lock(&a->mutex);
#endif
	for(;aindex<6;++aindex)
		if(anode->s[aindex]){
			astate=&anode->s[aindex]->s;
			break;
		}
#ifdef RESPECT_MUTEX
	mtx_unlock(&a->mutex);
	mtx_lock(&b->mutex);
#endif
	for(;bindex<6;++bindex)
		if(bnode->s[bindex]){
			bstate=&bnode->s[bindex]->s;
			break;
		}
#ifdef RESPECT_MUTEX
	mtx_unlock(&b->mutex);
#endif
	while(aindex<6&&bindex<6){
		int comp=compareStates(astate,bstate,asindex<bsindex?asindex:bsindex);
		if(!comp){										//If the two states do in fact match
#ifdef RESPECT_MUTEX
			mtx_lock(&a->mutex);								//Both paths of if(!comp) need both mutexes just to compute combined tier
			mtx_lock(&b->mutex);
#endif
			if(!solutionFound){								//If this is the first match we've found
				solutionFound=1;							//Tell the threads to stop building the stateTrees
				printf("Solution found\n");
				solution = (struct solutionS){.s = astate,.tier = anode->s[aindex]->tier + bnode->s[bindex]->tier};
				aindex=bindex=asindex=bsindex=0;					//Restart listMatch at the beginning of both stateLists so we can find all matches
				anode=a->head;
				bnode=b->head;

				for(;aindex<6;++aindex)							//Find first state of both lists
					if(anode->s[aindex]){
						astate=&anode->s[aindex]->s;
						break;
					}

				for(;bindex<6;++bindex)
					if(bnode->s[bindex]){
						bstate=&bnode->s[bindex]->s;
						break;
					}
			}
			else{										//If this is not the first match we've found, ignore it unless the combined tier is lower
				if(anode->s[aindex]->tier + bnode->s[bindex]->tier < solution.tier)
					solution = (struct solutionS){.s = astate,.tier = anode->s[aindex]->tier + bnode->s[bindex]->tier};
				if(anode->next[aindex]){						//But either way move both astate and bstate exactly one state forward
					astack[asindex++]=(struct listStack){.index=aindex,.node=anode};
					anode=anode->next[aindex];
					aindex = -1;							//will ++ to 0
				}
				for(++aindex;aindex<6;++aindex)
					if(anode->s[aindex])
						break;
				while(aindex==6&&asindex>0){
					--asindex;
					anode=astack[asindex].node;
					aindex=astack[asindex].index;
					for(++aindex;aindex<6;++aindex)
						if(anode->s[aindex])
							break;
				}
				astate = &anode->s[aindex]->s;
				
				if(bnode->next[bindex]){
					bstack[bsindex++]=(struct listStack){.index=bindex,.node=bnode};
					bnode=bnode->next[bindex];
					bindex = -1;
				}
				for(++bindex;bindex<6;++bindex)
					if(bnode->s[bindex])
						break;
				while(bindex==6&&bsindex>0){
					--bsindex;
					bnode=bstack[bsindex].node;
					bindex=bstack[bsindex].index;
					for(++bindex;bindex<6;++bindex)
						if(bnode->s[bindex])
							break;
				}
				bstate = &bnode->s[bindex]->s;							//If this is a pointer to an oob element, the loop will break before it is dereferenced
			}
#ifdef RESPECT_MUTEX
			mtx_unlock(&a->mutex);									//and we must give them both back
			mtx_unlock(&b->mutex);
#endif
		}
		else if(comp<0){										//If astate is less than bstate
			comp*=-1;	//Make comp positive
			if(comp <= asindex)									//If tier of difference is less than current working tier
				anode=astack[(asindex = comp-1)].node;						//Pop from stack
			aindex=bstate->c[asindex];								//Slide index to the value in b's state at the place where the difference occurred
#ifdef RESPECT_MUTEX
			mtx_lock(&a->mutex);
#endif
			if(anode->next[aindex]){
				comp=compareStates(bstate,&anode->s[aindex]->s,asindex);
				while(asindex<comp&&anode->next[aindex]){
					astack[asindex++]=(struct listStack){.index=aindex,.node=anode};
					anode=anode->next[aindex];
					aindex=bstate->c[asindex];
				}
			}
			if(!anode->s[aindex]||astate==&anode->s[aindex]->s){
				for(++aindex;aindex<6;++aindex)
					if(anode->s[aindex])
						break;
				while(aindex==6&&asindex>0){
					--asindex;
					anode=astack[asindex].node;
					aindex=astack[asindex].index;
					for(++aindex;aindex<6;++aindex)
						if(anode->s[aindex])
							break;
				}
			}
			astate=&anode->s[aindex]->s;								//If this is a pointer to an oob element, the loop will break before it is dereferenced
#ifdef RESPECT_MUTEX
			mtx_unlock(&a->mutex);
#endif
		}
		else{
			if(comp <= bsindex)
				bnode=bstack[(bsindex = comp-1)].node;
			bindex=astate->c[bsindex];
#ifdef RESPECT_MUTEX
			mtx_lock(&b->mutex);
#endif
			if(bnode->s[bindex]){
				comp=compareStates(astate,&bnode->s[bindex]->s,bsindex);
				while(bsindex<comp&&bnode->next[bindex]){
					bstack[bsindex++]=(struct listStack){.index=bindex,.node=bnode};
					bnode=bnode->next[bindex];
					bindex=astate->c[bsindex];
				}
			}
			if(!bnode->s[bindex]||bstate==&bnode->s[bindex]->s){
				for(++bindex;bindex<6;++bindex)
					if(bnode->s[bindex])
						break;
				while(bindex==6&&bsindex>0){
					--bsindex;
					bnode=bstack[bsindex].node;
					bindex=bstack[bsindex].index;
					for(++bindex;bindex<6;++bindex)
						if(bnode->s[bindex])
							break;
				}
			}
			bstate=&bnode->s[bindex]->s;								//If this is a pointer to an oob element the loop will break before it is dereferenced
#ifdef RESPECT_MUTEX
			mtx_unlock(&b->mutex);
#endif
		}
	}
  }
  return *solution.s;												//This is the intersection state of the trees with the lowest average depth
}

//This is the struct to hold the data a thread needs to perform the buildTree function properly.
typedef struct{
  stateList *list;
  treeQueue *queue;
}buildTreeData;

//This allows us to keep track of which tier of the tree we're currently processing
volatile unsigned char currentTier=0;

//This is a function meant to be executed by a thread. It processes nodes from the BFS queue, and uses them to produce up to 15 more nodes to add to the queue
THREAD_RETURN buildTree(void *data){
  stateList *list=((buildTreeData*)data)->list;
  treeQueue *queue=((buildTreeData*)data)->queue;
  while(!solutionFound){
	stateTreeNode *node=treeQueueRemove(queue);
	if(!node)
		continue;
	unsigned char currentSide=node->side*3;
 	if(node->tier>currentTier){
		currentTier=node->tier;
		printf("Starting tier %d\n",node->tier);
	}
 	for(int c=0;c<18;++c){
		if(c==currentSide){
			node->children[c]=node->children[c+1]=node->children[c+2]=NULL;
			c+=2;
			continue;
		}
		state hold;
		(transformations[c])(node->s,&hold);				//Mutate the state and compare it to the list
		stateTreeNode **listSlip;
		if((listSlip=addList(list,&hold))){				//We found a new unvisited state so we must add it to the stateList
			node->children[c]=malloc(sizeof(stateTreeNode));
			node->children[c]->s=hold;				//State and tier must be initialized before connecting to the list
			node->children[c]->tier=node->tier+1;
			*listSlip=node->children[c];
			mtx_unlock(&list->mutex);
			node->children[c]->side=c/3;				//listMatch doesn't care about side, so we can do this after it has access
			treeQueueAdd(queue,node->children[c]);
		}
		else								//This state is a duplicate
			node->children[c]=NULL;
	}
  }
  //The pointers in all the nodes in the queue were left uninitialized, so they must be made NULL before search is done on the tree
  stateTreeNode *node;
  while((node=treeQueueRemove(queue)))	//This breaks when treeQueueRemove returns NULL, which happens when the queue is exhausted
	memcpy(node->children,eightteenNulls,sizeof eightteenNulls);
  return (THREAD_RETURN)0;		//We have nothing meaningful to return
}

//This searches the mixed tree and returns a string of instructions to get to the target state of the tree. Return string must be freed
char* searchTree(stateTreeNode *tree,state *target){
  char *out=NULL;
  if(!compareStates(&tree->s,target,0)){
	out=malloc(1);
	*out='\0';
	return out;
  }
  int c;
  char *hold;
  for(c=0;c<18;++c)
	if(tree->children[c])
		if((hold=searchTree(tree->children[c],target))){
			out=malloc(strlen(hold)+strlen(descriptions[c])+2);
			sprintf(out,"%s\n%s",descriptions[c],hold);
			free(hold);
			return out;
		}
  return NULL;
}

//This searches the solved tree and prints instructions on how to get from the target state to solved
int backwardsSearchTree(stateTreeNode *tree,state *target){
  if(!compareStates(&tree->s,target,0))
	return 1;
  for(int c=0;c<18;++c)
	if(tree->children[c])
		if(backwardsSearchTree(tree->children[c],target)){
			switch(c%3){
				case 0:
					printf("%s\n",descriptions[c+1]);
					break;
				case 1:
					printf("%s\n",descriptions[c-1]);
					break;
				case 2:
					printf("%s\n",descriptions[c]);
					break;
			}
			return 1;
		}
  return 0;
}

//Frees all malloced nodes of the tree
void recursiveFreeTree(stateTreeNode *tree){
  if(!tree)
	return;
  for(int c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
  free(tree);
}

void freeTree(stateTreeNode *tree){
  for(int c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
}


/*Two stateTrees, two stateLists, and two treeQueues are made. One for the mixed cube's state, and one for the solution state.
 *Half of the threads work on solving the mixed cube, and the other work on shuffling the solved cube, until they encounter a common state
 *When that common state is found, all threads terminate and the trees are compared to create a list of results.
 */
void solve(state in,int cleanup){
  printf("Solving ");
  printState(&in);
  //These are the first nodes to go in each tree. One with the solved state and one with the shuffled state
  //There is no tree struct, as there is only one root, and there is no need for mutex, as each thread will be working on a different leaf at all times
  stateTreeNode fromMixed=(stateTreeNode){.s=in,.tier=0,.side=7};
  stateTreeNode fromSolved=(stateTreeNode){.s=solved,.tier=0,.side=7};
  //The stateList is set up for the mixed tree, with the initial shuffled state as the only state
  stateList mixedList=(stateList){.head=malloc(sizeof(stateListNode))};
  memcpy(mixedList.head,&emptyStateListNode,sizeof(stateListNode));
  mtx_init(&mixedList.mutex,mtx_plain);
  *addList(&mixedList,&fromMixed.s)=&fromMixed; //The list is empty to we assume it will not return NULL
  mtx_unlock(&mixedList.mutex); //When addList() doesn't return NULL we must unlock the mutex
  //The stateList is set up for the solved tree, with the solved state as the only state
  stateList solvedList=(stateList){.head=malloc(sizeof(stateListNode))};
  memcpy(solvedList.head,&emptyStateListNode,sizeof(stateListNode));
  mtx_init(&solvedList.mutex,mtx_plain);
  *addList(&solvedList,&fromSolved.s)=&fromSolved; //The list is empty, so we assume it will not return NULL
  mtx_unlock(&solvedList.mutex); //When addList() doesn't return NULL we must unlock the mutex
  //Queue for running BFS on the mixed stateTree is set up with the first node of the tree as the only node in the queue
  treeQueue mixedQueue;
  mixedQueue.head=mixedQueue.tail=ALLOCATE_TREE_QUEUE_NODE;
  mixedQueue.newHead=mixedQueue.newTail=ALLOCATE_TREE_QUEUE_NODE;
  mixedQueue.hindex=mixedQueue.tindex=mixedQueue.newIndex=0;
  mixedQueue.tail->nodes[mixedQueue.tindex++]=&fromMixed;
  mixedQueue.blankPage=NULL;
  mtx_init(&mixedQueue.mutex,mtx_plain);
  //Queue for running BFS on the solved stateTree is set up with the first node of the tree as the only node in the queue
  treeQueue solvedQueue;
  solvedQueue.head=solvedQueue.tail=ALLOCATE_TREE_QUEUE_NODE;
  solvedQueue.newHead=solvedQueue.newTail=ALLOCATE_TREE_QUEUE_NODE;
  solvedQueue.hindex=solvedQueue.tindex=solvedQueue.newIndex=0;
  solvedQueue.tail->nodes[solvedQueue.tindex++]=&fromSolved;
  solvedQueue.blankPage=NULL;
  mtx_init(&solvedQueue.mutex,mtx_plain);
  //Produces NUM_THREADS or NUM_THREADS+1 threads, half of which work on the shuffled tree, and half of which work on the solved tree
  thrd_t tids[NUM_THREADS+1];															{ //Limiting the scope of int c, state link, and char *string from here...
  int c;
  for(c=0;c<NUM_THREADS;c+=2){
    thrd_create(&tids[c],buildTree,(void*)&(const buildTreeData){.list=&mixedList,.queue=&mixedQueue});	//Implicit compound literals are used instead of explicit stack structs
    thrd_create(&tids[c+1],buildTree,(void*)&(const buildTreeData){.list=&solvedList,.queue=&solvedQueue}); //Making them const tells the compiler that they don't all need their own copy
  }
  //Control blocks here until a match is found in the two stateLists, and then continues to block until listMatch ensures that it's found the best match
  state link=listMatch(&mixedList,&solvedList);
  //When a match is found, the threads work together to set all the pointers at the bottom of the trees to NULL, before terminating, and being joined here
  for(--c;c>-1;--c)
  	thrd_join(tids[c],NULL);
  //The instructions are then computed using the two trees
  char *string;
  if(!(string=searchTree(&fromMixed,&link)))
	printf("No solution found\n");
  else{
	string[strlen(string)-1]='\0';
	printf("%s\n",string);
	free(string);
	backwardsSearchTree(&fromSolved,&link);
  }																		} // ...to here
  //This is all cleanup and is unnecessary if the program is being torn down right now
  //It is up to the calling function to ensure that if several cubes are being solved in one run, that cleanup should really be done between cubes
  if(cleanup){
  	freeStateList(&solvedList);
  	freeStateList(&mixedList);
  	freeTree(&fromSolved);
  	freeTree(&fromMixed);
  	DEALLOCATE_TREE_QUEUE_NODE(mixedQueue.head);
  	DEALLOCATE_TREE_QUEUE_NODE(solvedQueue.head);
	if(mixedQueue.blankPage)			//These are almost certainly true
		DEALLOCATE_TREE_QUEUE_NODE(mixedQueue.blankPage);
	if(solvedQueue.blankPage)
		DEALLOCATE_TREE_QUEUE_NODE(solvedQueue.blankPage);
  	mtx_destroy(&mixedQueue.mutex);
  	mtx_destroy(&solvedQueue.mutex);
  }
}

//This program is written to accept either an integer, or a complete state of 48 colors
//If an integer n is entered, a virtual random shuffle of n moves is done on the solved state, and the program tries to solve that cube
//Otherwise, the program tries to solve the given state
int main(int args,char *argv[]){
  if(args!=2&&args!=49){
	printf("USAGE: %s ([integer]|{w,b,g,r,o,y,...})\n",argv[0]);
	exit(EXIT_FAILURE);
  }
  else{
	int num=atoi(argv[1]);
	if(!num){
		state s;
		for(int c=1;c<49;++c){
			if(argv[c][0]=='w')
				s.c[c-1]=white;
			else if(argv[c][0]=='b')
				s.c[c-1]=blue;
			else if(argv[c][0]=='r')
				s.c[c-1]=red;
			else if(argv[c][0]=='g')
				s.c[c-1]=green;
			else if(argv[c][0]=='y')
				s.c[c-1]=yellow;
			else if(argv[c][0]=='o')
				s.c[c-1]=orange;
			else{
				printf("Character '%c' not understood\n",argv[c][0]);
				printf("USAGE: %s ([integer]|{w,b,g,r,o,y,...})\n",argv[0]);
				exit(EXIT_FAILURE);
			}
		}
		solve(s,0);
	}
	else
		solve(shuffle(num,1),0);
  }
}
