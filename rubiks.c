#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

enum color {white=0,blue=1,green=2,yellow=3,red=4,orange=5,w=0,b=1,g=2,y=3,r=4,o=5};
//This allows us to convert colors directly to integer types, and it also saves memory, since we only need 3 bits

typedef struct state{ //A state is 48 colors in a very specific order, which defines the placement of all of the colors on the cube
  enum color c[48];
}state_t;

void printState(state_t *in){ //This allows us to print any given state for debugging
  int c;
  printf("{");
  for(c=0;c<48;++c){
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
state_t solved=(state_t){{white,white,white,white,white,white,white,white,blue,blue,blue,blue,blue,blue,blue,blue,green,green,green,green,green,green,green,green,yellow,yellow,yellow,yellow,yellow,yellow,yellow,yellow,red,red,red,red,red,red,red,red,orange,orange,orange,orange,orange,orange,orange,orange}}; 

typedef struct stateTreeNode{ //Has a pointer for all possible movements from this point and a state
  state_t state;
  //struct stateTreeNode *faceClock,*faceCounter,*faceTwice,*leftClock,*leftCounter,*leftTwice,*rightClock,*rightCounter,*rightTwice,*rearClock,*rearCounter,*rearTwice,*topClock,*topCounter,*topTwice,*bottomClock,*bottomCounter,*bottomTwice;
  struct stateTreeNode *children[18];
}stateTreeNode_t;

typedef struct treeQueueNode{ //Nodes for treeQueue
  stateTreeNode_t *node;
  struct treeQueueNode *next;
}treeQueueNode_t;

typedef struct treeQueue{ //Queue so we can process the nodes in order for BFS search
  treeQueueNode_t *head;
  treeQueueNode_t *tail;
  sem_t turn;
}treeQueue_t;

typedef struct stateList{ //List of states we have visited before, so none get repeated.
  state_t *state;
  struct stateList *next;
  sem_t turn[0];
}stateList_t;

//The following functions mutate the input state to simulate some specific turn of the cube and return the resulting state.

void faceClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[5],in->c[3],in->c[0],in->c[6],in->c[1],in->c[7],in->c[4],in->c[2],in->c[40],in->c[41],in->c[42],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[32],in->c[33],in->c[34],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[8],in->c[9],in->c[10],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[16],in->c[17],in->c[18],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void faceCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[2],in->c[4],in->c[7],in->c[1],in->c[6],in->c[0],in->c[3],in->c[5],in->c[32],in->c[33],in->c[34],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[40],in->c[41],in->c[42],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[16],in->c[17],in->c[18],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[8],in->c[9],in->c[10],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void faceTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[7],in->c[6],in->c[5],in->c[4],in->c[3],in->c[2],in->c[1],in->c[0],in->c[16],in->c[17],in->c[18],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[8],in->c[9],in->c[10],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[40],in->c[41],in->c[42],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[32],in->c[33],in->c[34],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void leftClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[39],in->c[1],in->c[2],in->c[36],in->c[4],in->c[34],in->c[6],in->c[7],in->c[13],in->c[11],in->c[8],in->c[14],in->c[9],in->c[15],in->c[12],in->c[10],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[45],in->c[27],in->c[43],in->c[29],in->c[30],in->c[40],in->c[32],in->c[33],in->c[26],in->c[35],in->c[28],in->c[37],in->c[38],in->c[31],in->c[0],in->c[41],in->c[42],in->c[3],in->c[44],in->c[5],in->c[46],in->c[47]}};
}

void leftCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[40],in->c[1],in->c[2],in->c[43],in->c[4],in->c[45],in->c[6],in->c[7],in->c[10],in->c[12],in->c[15],in->c[9],in->c[14],in->c[8],in->c[11],in->c[13],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[34],in->c[27],in->c[36],in->c[29],in->c[30],in->c[39],in->c[32],in->c[33],in->c[5],in->c[35],in->c[3],in->c[37],in->c[38],in->c[0],in->c[31],in->c[41],in->c[42],in->c[28],in->c[44],in->c[26],in->c[46],in->c[47]}};
}

void leftTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[31],in->c[1],in->c[2],in->c[28],in->c[4],in->c[26],in->c[6],in->c[7],in->c[15],in->c[14],in->c[13],in->c[12],in->c[11],in->c[10],in->c[9],in->c[8],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[21],in->c[22],in->c[23],in->c[24],in->c[25],in->c[5],in->c[27],in->c[3],in->c[29],in->c[30],in->c[0],in->c[32],in->c[33],in->c[45],in->c[35],in->c[43],in->c[37],in->c[38],in->c[40],in->c[39],in->c[41],in->c[42],in->c[36],in->c[44],in->c[34],in->c[46],in->c[47]}};
}

void rightClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[42],in->c[3],in->c[44],in->c[5],in->c[6],in->c[47],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[21],in->c[19],in->c[16],in->c[22],in->c[17],in->c[23],in->c[20],in->c[18],in->c[32],in->c[25],in->c[26],in->c[35],in->c[28],in->c[37],in->c[30],in->c[31],in->c[7],in->c[33],in->c[34],in->c[4],in->c[36],in->c[2],in->c[38],in->c[39],in->c[40],in->c[41],in->c[29],in->c[43],in->c[27],in->c[45],in->c[46],in->c[24]}};
}

void rightCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[37],in->c[3],in->c[35],in->c[5],in->c[6],in->c[32],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[18],in->c[20],in->c[23],in->c[17],in->c[22],in->c[16],in->c[19],in->c[21],in->c[47],in->c[25],in->c[26],in->c[44],in->c[28],in->c[42],in->c[30],in->c[31],in->c[24],in->c[33],in->c[34],in->c[27],in->c[36],in->c[24],in->c[38],in->c[39],in->c[40],in->c[41],in->c[2],in->c[43],in->c[4],in->c[45],in->c[46],in->c[7]}};
}

void rightTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[29],in->c[3],in->c[27],in->c[5],in->c[6],in->c[24],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[13],in->c[14],in->c[15],in->c[23],in->c[22],in->c[21],in->c[20],in->c[19],in->c[18],in->c[17],in->c[16],in->c[7],in->c[25],in->c[26],in->c[4],in->c[28],in->c[2],in->c[30],in->c[31],in->c[47],in->c[33],in->c[34],in->c[44],in->c[36],in->c[42],in->c[38],in->c[39],in->c[40],in->c[41],in->c[37],in->c[43],in->c[35],in->c[45],in->c[46],in->c[32]}};
}

void rearClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[37],in->c[38],in->c[39],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[45],in->c[46],in->c[47],in->c[29],in->c[27],in->c[24],in->c[30],in->c[25],in->c[31],in->c[28],in->c[26],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[21],in->c[22],in->c[23],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[13],in->c[14],in->c[15]}};
}

void rearCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[45],in->c[46],in->c[47],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[37],in->c[38],in->c[39],in->c[26],in->c[28],in->c[31],in->c[25],in->c[30],in->c[24],in->c[27],in->c[29],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[13],in->c[14],in->c[15],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[21],in->c[22],in->c[23]}};
}

void rearTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[8],in->c[9],in->c[10],in->c[11],in->c[12],in->c[21],in->c[22],in->c[23],in->c[16],in->c[17],in->c[18],in->c[19],in->c[20],in->c[13],in->c[14],in->c[15],in->c[31],in->c[30],in->c[29],in->c[28],in->c[27],in->c[26],in->c[25],in->c[24],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[45],in->c[46],in->c[47],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[37],in->c[38],in->c[39]}};
}

void topClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[18],in->c[20],in->c[23],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[2],in->c[9],in->c[10],in->c[1],in->c[12],in->c[0],in->c[14],in->c[15],in->c[16],in->c[17],in->c[24],in->c[19],in->c[25],in->c[21],in->c[22],in->c[26],in->c[13],in->c[11],in->c[8],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[37],in->c[35],in->c[32],in->c[38],in->c[33],in->c[39],in->c[36],in->c[34],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void topCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[13],in->c[11],in->c[8],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[26],in->c[9],in->c[10],in->c[25],in->c[12],in->c[24],in->c[14],in->c[15],in->c[16],in->c[17],in->c[0],in->c[19],in->c[1],in->c[21],in->c[22],in->c[2],in->c[18],in->c[20],in->c[23],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[34],in->c[36],in->c[39],in->c[33],in->c[38],in->c[32],in->c[35],in->c[37],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void topTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[24],in->c[25],in->c[26],in->c[3],in->c[4],in->c[5],in->c[6],in->c[7],in->c[23],in->c[9],in->c[10],in->c[20],in->c[12],in->c[18],in->c[14],in->c[15],in->c[16],in->c[17],in->c[13],in->c[19],in->c[11],in->c[21],in->c[22],in->c[8],in->c[0],in->c[1],in->c[2],in->c[27],in->c[28],in->c[29],in->c[30],in->c[31],in->c[39],in->c[38],in->c[37],in->c[36],in->c[35],in->c[34],in->c[33],in->c[32],in->c[40],in->c[41],in->c[42],in->c[43],in->c[44],in->c[45],in->c[46],in->c[47]}};
}

void bottomClock(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[15],in->c[12],in->c[10],in->c[8],in->c[9],in->c[31],in->c[11],in->c[30],in->c[13],in->c[14],in->c[29],in->c[5],in->c[17],in->c[18],in->c[6],in->c[20],in->c[7],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[16],in->c[19],in->c[21],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[45],in->c[43],in->c[40],in->c[46],in->c[41],in->c[47],in->c[44],in->c[42]}};
}

void bottomCounter(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[16],in->c[19],in->c[21],in->c[8],in->c[9],in->c[7],in->c[11],in->c[6],in->c[13],in->c[14],in->c[5],in->c[29],in->c[17],in->c[18],in->c[30],in->c[20],in->c[31],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[15],in->c[12],in->c[10],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[42],in->c[44],in->c[47],in->c[41],in->c[46],in->c[40],in->c[43],in->c[45]}};
}

void bottomTwice(state_t *in,state_t *out){
  *out=(state_t){{in->c[0],in->c[1],in->c[2],in->c[3],in->c[4],in->c[29],in->c[30],in->c[31],in->c[8],in->c[9],in->c[21],in->c[11],in->c[19],in->c[13],in->c[14],in->c[16],in->c[15],in->c[17],in->c[18],in->c[12],in->c[20],in->c[10],in->c[22],in->c[23],in->c[24],in->c[25],in->c[26],in->c[27],in->c[28],in->c[5],in->c[6],in->c[7],in->c[32],in->c[33],in->c[34],in->c[35],in->c[36],in->c[37],in->c[38],in->c[39],in->c[47],in->c[46],in->c[45],in->c[44],in->c[43],in->c[42],in->c[41],in->c[40]}};
}

void (*transformations[])(state_t*,state_t*)={faceClock,faceCounter,faceTwice,leftClock,leftCounter,leftTwice,rightClock,rightCounter,rightTwice,rearClock,rearCounter,rearTwice,topClock,topCounter,topTwice,bottomClock,bottomCounter,bottomTwice};

int compareStates(state_t *a,state_t *b){ //returns -1 if less, 1 if greater, and 0 if equal. This allows us to keep our state list in order.
  int i;
  for(i=0;i<48;++i)
	if(a->c[i]!=b->c[i])
		return a->c[i]<b->c[i]?-1:1;
  return 0;
}

void printStateList(stateList_t *list){
  stateList_t *pt;
  for(pt=list;pt;pt=pt->next)
	printState(pt->state);
}

state_t** addList(stateList_t *stateList,state_t *state){ //Returns pointer to place in the list where the pointer to the state_t should be stored
  stateList_t *pt; //The state_t pointer that we're passed is to a temporary variable in another function, so it is up to the calling function to save a permanent
  int comp;        //Pointer to state_t at the address returned, and then release the semaphore to the list.
  state_t **out=NULL;
  sem_wait(stateList->turn);
  for(pt=stateList;(comp=compareStates(pt->state,state))==-1&&pt->next;pt=pt->next)
	;
  if(comp==0){
	sem_post(stateList->turn);
	return out;
  }
  else if(comp==-1){
	pt->next=malloc(sizeof(stateList_t));
	pt=pt->next;
	out=&pt->state;
	pt->next=NULL;
  }
  else{	
	out=&pt->state;
	stateList_t *n=malloc(sizeof(stateList_t));
	n->state=pt->state;
	n->next=pt->next;
	pt->next=n;
  }
  return out;
}

void freeStateList(stateList_t *list){
  stateList_t *pt,*lead;
  for(pt=list;pt;pt=lead){
	lead=pt->next;
	free(pt);
  }
}

void treeQueueAdd(treeQueue_t *treeQueue,stateTreeNode_t *n){
  treeQueueNode_t *node=(treeQueueNode_t*)malloc(sizeof(treeQueueNode_t));
  node->node=n;
  node->next=NULL;
  sem_wait(&treeQueue->turn);
  if(treeQueue->tail){
	treeQueue->tail->next=node;
	treeQueue->tail=node;
  }
  else
	treeQueue->head=treeQueue->tail=node;
  sem_post(&treeQueue->turn);
}

stateTreeNode_t* treeQueueRemove(treeQueue_t *treeQueue){
  sem_wait(&treeQueue->turn);
  if(!treeQueue->head){
	sem_post(&treeQueue->turn);
	return NULL;
  }
  stateTreeNode_t *out=treeQueue->head->node;
  treeQueueNode_t *hold=treeQueue->head;
  treeQueue->head=hold->next;
  free(hold);
  if(!treeQueue->head)
	treeQueue->tail=NULL;
  sem_post(&treeQueue->turn);
  return out;
}

typedef struct volatileData{
  volatile state_t *state;
  volatile stateTreeNode_t **put;
  void (*transformation)(state_t*,state_t*);
  sem_t done,restart;
}volatileData_t;

typedef struct threadData{
  stateList_t *stateList;
  treeQueue_t *treeQueue;
  volatile volatileData_t *volatiles;
  sem_t done;
}threadData_t;

volatile int solutionFound=0;

void* threadBuild(void *data){
  volatileData_t volatiles;
  sem_init(&volatiles.done,0,0);
  sem_init(&volatiles.restart,0,0);
  ((threadData_t*)data)->volatiles=&volatiles;
  sem_post(&((threadData_t*)data)->done);
  stateList_t *stateList=((threadData_t*)data)->stateList;
  treeQueue_t *treeQueue=((threadData_t*)data)->treeQueue;
  state_t transformed;
  state_t *vstate;
  stateTreeNode_t **vput;
  state_t **listSlip;
  sem_wait(&volatiles.restart);
  int c;
  while(!solutionFound){
	vstate=(state_t*)volatiles.state;
	vput=(stateTreeNode_t**)volatiles.put;
  	(volatiles.transformation)(vstate,&transformed);
	if((listSlip=addList(stateList,&transformed))){
		(*vput)=malloc(sizeof(stateTreeNode_t));
		(*vput)->state=transformed;
		*listSlip=&(*vput)->state;
		sem_post(stateList->turn);
		if(compareStates(&solved,&transformed)){
			treeQueueAdd(treeQueue,*vput);
		}
		else{
			solutionFound=1;
			for(c=0;c<18;++c)
				(*vput)->children[c]=NULL;
		}
	}
	else
		*vput=NULL;
	sem_post(&volatiles.done);
	sem_wait(&volatiles.restart);
  }
  sem_wait(&((threadData_t*)data)->done);
  sem_destroy(&volatiles.done);
  sem_destroy(&volatiles.restart);
  return NULL;
}

#undef done
#undef restart

void buildTree(stateTreeNode_t *node,stateList_t *stateList){
  treeQueue_t treeQueue;
  treeQueue.head=NULL;
  treeQueue.tail=NULL;
  sem_init(&treeQueue.turn,0,1);
  //register treeQueue_t *treeQueue=&onStack;
  pthread_t pids[18];
  volatileData_t *volatiles[18];
  threadData_t threadData=(threadData_t){.stateList=stateList,.treeQueue=&treeQueue,.volatiles=NULL,};
  sem_init(&threadData.done,0,0);
  int c;
  for(c=0;c<18;++c){
	pthread_create(&pids[c],NULL,threadBuild,(void*)&threadData);
	sem_wait(&threadData.done);
	volatiles[c]=(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)(volatileData_t*)threadData.volatiles;
	volatiles[c]->state=&node->state;
	volatiles[c]->put=(volatile stateTreeNode_t**)&node->children[c];
	volatiles[c]->transformation=transformations[c];
	sem_post(&volatiles[c]->restart);
  }
  while(!solutionFound&&(node=treeQueueRemove(&treeQueue))){
	for(c=0;c<18;++c){
		sem_wait(&volatiles[c]->done);
		volatiles[c]->state=&node->state;
		volatiles[c]->put=(volatile stateTreeNode_t**)&node->children[c];
		sem_post(&volatiles[c]->restart);
	}
  }
  if(!solutionFound){
	printf("Solution not found\n");
	solutionFound=1;
  }
  else{
	printf("SOLUTION FOUND\n");
  	while((node=treeQueueRemove(&treeQueue)))
		for(c=0;c<18;++c)
			node->children[c]=NULL;
  }
  for(c=0;c<18;++c){
	sem_post(&volatiles[c]->restart);
	sem_post(&threadData.done);
	pthread_join(pids[c],NULL);
  }
  sem_destroy(&threadData.done);
}
/*  do{
	faceClock(node->state,&hold);
	if(addList(stateList,hold)){
		node->faceClock=malloc(sizeof(stateTreeNode_t));
		node->faceClock->state=hold;
		if(!compareStates(solved,hold)){
			node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->faceClock;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->faceClock);
	}
	else
		node->faceClock=NULL;
	faceCounter(node->state,&hold);
	if(addList(stateList,hold)){
		node->faceCounter=malloc(sizeof(stateTreeNode_t));
		node->faceCounter->state=hold;
		if(!compareStates(solved,hold)){
			node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->faceCounter;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->faceCounter);
	}
	else
		node->faceCounter=NULL;
	faceTwice(node->state,&hold);
	if(addList(stateList,hold)){
		node->faceTwice=malloc(sizeof(stateTreeNode_t));
		node->faceTwice->state=hold;
		if(!compareStates(solved,hold)){
			node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->faceTwice;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->faceTwice);
	}
	else
		node->faceTwice=NULL;
	leftClock(node->state,&hold);
	if(addList(stateList,hold)){
		node->leftClock=malloc(sizeof(stateTreeNode_t));
		node->leftClock->state=hold;
		if(!compareStates(solved,hold)){
			node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->leftClock;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->leftClock);
	}
	else
		node->leftClock=NULL;
	leftCounter(node->state,&hold);
	if(addList(stateList,hold)){
		node->leftCounter=malloc(sizeof(stateTreeNode_t));
		node->leftCounter->state=hold;
		if(!compareStates(solved,hold)){
			node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->leftCounter;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->leftCounter);
	}
	else
		node->leftCounter=NULL;
	leftTwice(node->state,&hold);
	if(addList(stateList,hold)){
		node->leftTwice=malloc(sizeof(stateTreeNode_t));
		node->leftTwice->state=hold;
		if(!compareStates(solved,hold)){
			node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->leftTwice;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->leftTwice);
	}
	else
		node->leftTwice=NULL;
	rightClock(node->state,&hold);
	if(addList(stateList,hold)){
		node->rightClock=malloc(sizeof(stateTreeNode_t));
		node->rightClock->state=hold;
		if(!compareStates(solved,hold)){
			node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->rightClock;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->rightClock);
	}
	else
		node->rightClock=NULL;
	rightCounter(node->state,&hold);
	if(addList(stateList,hold)){
		node->rightCounter=malloc(sizeof(stateTreeNode_t));
		node->rightCounter->state=hold;
		if(!compareStates(solved,hold)){
			node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->rightCounter;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->rightCounter);
	}
	else
		node->rightCounter=NULL;
	rightTwice(node->state,&hold);
	if(addList(stateList,hold)){
		node->rightTwice=malloc(sizeof(stateTreeNode_t));
		node->rightTwice->state=hold;
		if(!compareStates(solved,hold)){
			node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->rightTwice;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->rightTwice);
	}
	else
		node->rightTwice=NULL;
	rearClock(node->state,&hold);
	if(addList(stateList,hold)){
		node->rearClock=malloc(sizeof(stateTreeNode_t));
		node->rearClock->state=hold;
		if(!compareStates(solved,hold)){
			node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->rearClock;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->rearClock);
	}
	else
		node->rearClock=NULL;
	rearCounter(node->state,&hold);
	if(addList(stateList,hold)){
		node->rearCounter=malloc(sizeof(stateTreeNode_t));
		node->rearCounter->state=hold;
		if(!compareStates(solved,hold)){
			node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->rearCounter;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->rearCounter);
	}
	else
		node->rearCounter=NULL;
	rearTwice(node->state,&hold);
	if(addList(stateList,hold)){
		node->rearTwice=malloc(sizeof(stateTreeNode_t));
		node->rearTwice->state=hold;
		if(!compareStates(solved,hold)){
			node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->rearTwice;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->rearTwice);
	}
	else
		node->rearTwice=NULL;
	topClock(node->state,&hold);
	if(addList(stateList,hold)){
		node->topClock=malloc(sizeof(stateTreeNode_t));
		node->topClock->state=hold;
		if(!compareStates(solved,hold)){
			node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->topClock;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->topClock);
	}
	else
		node->topClock=NULL;
	topCounter(node->state,&hold);
	if(addList(stateList,hold)){
		node->topCounter=malloc(sizeof(stateTreeNode_t));
		node->topCounter->state=hold;
		if(!compareStates(solved,hold)){
			node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->topCounter;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->topCounter);
	}
	else
		node->topCounter=NULL;
	topTwice(node->state,&hold);
	if(addList(stateList,hold)){
		node->topTwice=malloc(sizeof(stateTreeNode_t));
		node->topTwice->state=hold;
		if(!compareStates(solved,hold)){
			node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			node=node->topTwice;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->topTwice);
	}
	else
		node->topTwice=NULL;
	bottomClock(node->state,&hold);
	if(addList(stateList,hold)){
		node->bottomClock=malloc(sizeof(stateTreeNode_t));
		node->bottomClock->state=hold;
		if(!compareStates(solved,hold)){
			node->bottomCounter=node->bottomTwice=NULL;
			node=node->bottomClock;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->bottomClock);
	}
	else
		node->bottomClock=NULL;
	bottomCounter(node->state,&hold);
	if(addList(stateList,hold)){
		node->bottomCounter=malloc(sizeof(stateTreeNode_t));
		node->bottomCounter->state=hold;
		if(!compareStates(solved,hold)){
			node->bottomTwice=NULL;
			node=node->bottomCounter;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->bottomCounter);
	}
	else
		node->bottomCounter=NULL;
	bottomTwice(node->state,&hold);
	if(addList(stateList,hold)){
		node->bottomTwice=malloc(sizeof(stateTreeNode_t));
		node->bottomTwice->state=hold;
		if(!compareStates(solved,hold)){
			node=node->bottomTwice;
			node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
			goto found;
		}
		treeQueueAdd(treeQueue,node->bottomTwice);
	}
	else
		node->bottomTwice=NULL;
	node=treeQueueRemove(treeQueue);
	if(!node){
		printf("Solution not found\n");
		printStateList(stateList);
		return;
	}
  }while(compareStates(solved,node->state));
found:
  printf("Solution found!!!\n");
  while((node=treeQueueRemove(treeQueue)))
  node->faceClock=node->faceCounter=node->faceTwice=node->leftClock=node->leftCounter=node->leftTwice=node->rightClock=node->rightCounter=node->rightTwice=node->rearClock=node->rearCounter=node->rearTwice=node->topClock=node->topCounter=node->topTwice=node->bottomClock=node->bottomCounter=node->bottomTwice=NULL;
  sem_destroy(&treeQueue->turn);
}*/

int searchTree(stateTreeNode_t *tree){
  if(!compareStates(&tree->state,&solved)){
	return 1;
  }
  if(tree->children[0])
	if(searchTree(tree->children[0])){
		printf("Face clockwise\n");
		return 1;
	}
  if(tree->children[1])
	if(searchTree(tree->children[1])){
		printf("Face counter-clockwise\n");
		return 1;
	}
  if(tree->children[2])
	if(searchTree(tree->children[2])){
		printf("Face twice\n");
		return 1;
	}
  if(tree->children[3])
	if(searchTree(tree->children[3])){
		printf("Left clockwise \n");
		return 1;
	}
  if(tree->children[4])
	if(searchTree(tree->children[4])){
		printf("Left counter-clockwise\n");
		return 1;
	}
  if(tree->children[5])
	if(searchTree(tree->children[5])){
		printf("Left twice\n");
		return 1;
	}
  if(tree->children[6])
	if(searchTree(tree->children[6])){
		printf("Right clockwise\n");
		return 1;
	}
  if(tree->children[7])
	if(searchTree(tree->children[7])){
		printf("Right counter-clockwise\n");
		return 1;
	}
  if(tree->children[8])
	if(searchTree(tree->children[8])){
		printf("Right twice\n");
		return 1;
	}
  if(tree->children[9])
	if(searchTree(tree->children[9])){
		printf("Rear clockwise\n");
		return 1;
	}
  if(tree->children[10])
	if(searchTree(tree->children[10])){
		printf("Rear counter-clockwise\n");
		return 1;
	}
  if(tree->children[11])
	if(searchTree(tree->children[11])){
		printf("Rear twice\n");
		return 1;
	}
  if(tree->children[12])
	if(searchTree(tree->children[12])){
		printf("Top clockwise\n");
		return 1;
	}
  if(tree->children[13])
	if(searchTree(tree->children[13])){
		printf("Top counter-clockwise\n");
		return 1;
	}
  if(tree->children[14])
	if(searchTree(tree->children[14])){
		printf("Top twice\n");
		return 1;
	}
  if(tree->children[15])
	if(searchTree(tree->children[15])){
		printf("Bottom clockwise\n");
		return 1;
	}
  if(tree->children[16])
	if(searchTree(tree->children[16])){
		printf("Bottom counter-clockwise\n");
		return 1;
	}
  if(tree->children[17])
	if(searchTree(tree->children[17])){
		printf("Botttom twice\n");
		return 1;
	}
  return 0;
}

void recursiveFreeTree(stateTreeNode_t *tree){
  if(!tree)
	return;
  int c;
  for(c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
  free(tree);
}

void freeTree(stateTreeNode_t *tree){
  int c;
  for(c=0;c<18;++c)
  	recursiveFreeTree(tree->children[c]);
}

int main(){
  stateTreeNode_t tree;
  tree.state=(state_t){{b,b,r,o,r,o,w,w,r,b,b,y,b,y,b,b,o,w,w,g,g,g,g,g,o,g,g,o,r,o,y,r,g,w,w,y,r,y,r,r,y,o,b,y,w,y,o,w}};
  stateList_t *stateList=(stateList_t*)malloc(sizeof(stateList_t)+sizeof(sem_t));
  stateList->state=&tree.state;
  stateList->next=NULL;
  sem_init(stateList->turn,0,1);
  printf("Building tree\n");
  buildTree(&tree,stateList);
  freeStateList(stateList);
  sem_destroy(stateList->turn);
  searchTree(&tree);
  freeTree(&tree);
}
