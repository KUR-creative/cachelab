#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include "cachelab.h"

//#define	SUCCESS		0
//#define	MALLOC_ERR	1

// 캐스팅 하지 않음: 캐스팅이 필요한 num은 넣지 마라.
#define i32uPRINT(num)		printf("%X ", (num));
#define i64uPRINT(num)		printf("%lX ", (num));

#define i32sPRINT(num)		printf("%d ", (num));
#define i64sPRINT(num)		printf("%ld ", (num));

#define i32sPRINTn(num)		printf("%d: \n", (num));
#define i64sPRINTn(num)		printf("%ld: \n", (num));

#define	pPRINTn(str,ptr)	printf(str": %p \n",(ptr))
#define	dPRINTn(str,num)	printf(str": %d \n",(num))


#define lxEXPECT_EQ(actual,expected)\
	cr_expect_eq((actual),(expected),\
	#actual":0x%lX != 0x%lX:"#expected"\n",(int64_t)(actual),(int64_t)(expected))

#define lxASSERT_EQ(actual,expected)\
	cr_assert_eq((actual),(expected),\
	#actual":0x%lx != 0x%lx:"#expected"\n",(int64_t)(actual),(int64_t)(expected))

#define dASSERT_EQ(actual,expected)\
	cr_assert_eq((actual),(expected),\
	#actual":%d != %d:"#expected"\n",(int32_t)(actual),(int32_t)(expected))


#define dEXPECT_EQ(actual,expected)\
	cr_expect_eq((actual),(expected),\
	#actual":%d != %d:"#expected"\n",(int32_t)(actual),(int32_t)(expected))

#define pASSERT_EQ(actual,expected)\
	cr_assert_eq((actual),(expected),\
	#actual":%p != %p:"#expected"\n",(void**)(actual),(void**)(expected))

struct Node;

typedef enum SIM_ACT { 
	HIT, 
	MISS, 
	EVICT,
	ACT_NUM,	// number of simulation act
} SIM_ACT;


typedef struct {
	char			act;
	unsigned long	addr;
	int				size;
} Trace;

typedef struct {
	int				s;	// number of sets in cache.
	int				E;	// number of lines in one set.
	int				b;	// number of block bits.
} Tuple_sEb;


typedef struct Line {
	bool			isValid;
	unsigned int	tag;
} Line;

typedef struct Set {
	struct Node*	queue;
	Line*			lineArr;
} Set;

typedef struct Cache {
	Set*			setArr;
	int				S;	// number of sets.
	int				E;	// number of lines.
} Cache;

typedef struct Node {
	union 
	{
		int64_t		val;
		SIM_ACT		act;
		Line*		ptr;
	};
	struct Node*	next;
} Node;

// linked list
Node*		createLinkedList(int64_t firstValue);
Node*		addNode(Node* tail, int64_t nodeValue);
Node*		getTail(Node* listHead);
int			getLength(Node* listHead);
void		printList(Node* listHead);
void		freeList(Node** listHead);

// queue
Node*		enqueue(Node** queue, Line* line);
Line*		peek(Node* queue);
Node*		dequeue(Node** queue);
Line*		dequeueAndFree(Node** queue);
void		deleteNodesDuplicateToTail(Node** queue);

// trace file to trace arr
int			getNumOfValidLines(FILE* pFile);
void		traceFileToTraceArr(FILE* traceFile, Trace traceArr[]);

// address bit split
unsigned	getOffset(unsigned long address, size_t b);
unsigned	getSetIndex(unsigned long address, size_t s, size_t b);
unsigned	getTag(unsigned long address, size_t s, size_t b);

// other interface
Tuple_sEb	interpretOptions(int argc, char* argv[],	
							char** traceFilePath,	
							int* helpFlag, 
							int* verboseFlag);
void		getSummary(SIM_ACT summary[ACT_NUM], 
						Node* resultListArr[],
						int resArrLen);
FILE*		traceFileOpen(char* path);

// cache
Cache*		createCache(Tuple_sEb sEb);
Node*		runCache(Cache* cache, Trace trace, Tuple_sEb sEb);


/****************************************************
 *		linked list functions
 ****************************************************/
//if malloc error occurred, then return NULL
Node*		createLinkedList(int64_t firstValue)
{
	Node*	ret = malloc( sizeof(Node) );
	ret->val = firstValue;
	ret->next = NULL; 
	return ret;
}

// return tail node ptr of list
Node*		addNode(Node* tail, int64_t nodeValue)
{
	Node*	tmp = malloc( sizeof(Node) );
	if(tmp == NULL){
		printf("heap allocation error occurred! \n");
		exit(EXIT_FAILURE);
	}
	tmp->val = nodeValue; 
	tmp->next = tail->next;
	tail->next = tmp;
	return tmp;
}

// if list were not allocated, then return NULL
Node*		getTail(Node* listHead)
{
	Node*	cursor	= listHead;
	if(cursor == NULL){
		return NULL;
	}
	while(cursor->next != NULL){
		cursor = cursor->next;		
		//printf(">!> %p << \n", cursor->next);		
	}			//printf(">>> %p << \n", cursor);	
	return cursor;
}

int			getLength(Node* listHead)
{
	Node*	cursor	= listHead;
	int		len		= 0;
	while(cursor != NULL){
		len++;
		cursor = cursor->next;
	}
	return len;
}

void		printList(Node* listHead)
{
	putchar('|');
	if(listHead == NULL){
		printf("empty list!");
	}

	Node*	cursor	= listHead;
	while(cursor != NULL){
		switch(cursor->act){
		case HIT:
			printf("hit ");
			break;
		case MISS:
			printf("miss ");
			break;
		case EVICT:
			printf("eviction ");
			break;
		default:
			printf( "%lx ", (long unsigned)(cursor->ptr) % 0x100LU );
			//printf("%x ", cursor->ptr->tag); //왜 큐 에러?
		}
		cursor = cursor->next;
	}
	putchar('\n');
}

// free all of Nodes in list
// and head = NULL;
void		freeList(Node** listHead)
{
	Node*	cursor = *listHead;
	while(cursor != NULL){
		Node* next = cursor->next;
		free(cursor);
		cursor = next;
	}

	*listHead = NULL;
}


/****************************************************
 *		queue functions
 *EMPTY QUEUE means:	Node* queue = NULL; 
 ****************************************************/

// it return pointer that refer to tail node of queue
Node*		enqueue(Node** queue, Line* line)
{	
	//pPRINTn("line: ", line);
	Node*	tail		= getTail(*queue);
	int64_t	converted	= (int64_t)line; 
	if(tail == NULL){ // queue is empty!
		*queue = createLinkedList( converted );
			 //printf("len? = %d \n", getLength(*queue));//DBG:
			 //printf("what? :"); 
			 //puts("why null?");
			 //printList(*queue);
		return *queue;
	}
	tail = addNode(tail, converted);
	return tail;
}

Line*		peek(Node* queue)
{
	return queue->ptr; 
}

// avoid this. instead, use dequeueAndFree.
// you NEED to FREE returned Node YOURSELF..!
Node*		dequeue(Node** queue)
{	
	Node*	head	= *queue;
	*queue = head->next;
	return head;
}

Line*		dequeueAndFree(Node** queue)
{
	
	Node*	head		= *queue;		//puts("1_?");
			//pPRINTn("head", head);// why nil??
			//printList(head);
			//pPRINTn("head->ptr", head->ptr);
	Line*	retLinePtr	= head->ptr;	//puts("2_?");
			//pPRINTn("retLinePtr", retLinePtr);
	*queue = head->next;				//puts("3_?");
			//pPRINTn("ret", retLinePtr);
	free(head);							//puts("4_?");
			//pPRINTn("ret", retLinePtr);
	return retLinePtr;
}

// undulpicated queue.
// use this after enqueue something to delete duplicated nodes.
void		deleteNodesDuplicateToTail(Node** queue){
	int		len		= getLength(*queue);
	if(*queue == NULL || len == 1){
		return;	// length of q must be greater than 1.
	}

	int64_t	tailVal	= getTail(*queue)->val;
	Node*	prev	= *queue;
	Node*	cursor	= *queue;

	for(int i = 0; i < len-1; i++){	
								//printf("%d",i);	printList(*queue);
		if(cursor->val == tailVal){ //deletion!
			if(cursor == *queue){// head
				*queue = cursor->next;	
				prev = cursor->next;
			}else{	// not head
				prev->next = cursor->next;
			}
		}
		else{// no deletion
			if(cursor != *queue){	// not head
				prev = prev->next;

			}
		}
		cursor = cursor->next;	//printf("%d",i);	printList(*queue);
	}
}
	

/****************************************************
 *		trace file to Trace array functions
 ****************************************************/
int			getNumOfValidLines(FILE* pFile)
{
	int len = 0;
	int c;

	while((c = fgetc(pFile)) != EOF){
		if(c == 'L' || c == 'S' || c == 'M'){
			len++;
		}
	}
	rewind(pFile);

	return len;
}

void		traceFileToTraceArr(FILE* traceFile, Trace traceArr[])
{
	int	i = 0;
	char act; void** addr; int size;
	while(fscanf(traceFile, " %c %p,%d", &act, &addr, &size) != EOF)
	{
		if(act != 'I'){
			traceArr[i].act  = act;
			traceArr[i].addr = (unsigned long)addr;
			traceArr[i].size = size;
			i++;
		}
	}
}


/****************************************************
 *		Trace address split functions
 * Trace.addr -> [tag, set_index, block_offset]
 * s,b are number of bits
 ****************************************************/
unsigned	getOffset(unsigned long address, size_t b)
{
	size_t shiftVal		= 8 * sizeof(unsigned long) - b;
	address <<= shiftVal;
	address >>= shiftVal;
	return address;
}
unsigned	getSetIndex(unsigned long address, size_t s, size_t b)
{
	size_t shiftUpVal	= 8 * sizeof(unsigned long) - (s + b);
	size_t shiftDownVal	= 8 * sizeof(unsigned long) - s;
	address <<= shiftUpVal;
	address >>= shiftDownVal;
	return address;
}
unsigned	getTag(unsigned long address, size_t s, size_t b)
{
	size_t shiftVal		= s + b;
	address >>= shiftVal;
	return address;
}

/****************************************************
 *		other interfacing functions
 ****************************************************/
//ex) int h,v; char* path = "blabla.."
//interpretOptions(argc, argv, &path, h, v);
Tuple_sEb	interpretOptions(int argc, char* argv[],	
							char** traceFilePath,	
							int* helpFlag, 
							int* verboseFlag)
{
	Tuple_sEb	ret;

	*helpFlag = 0;
	*verboseFlag = 0;

	int opt;
	while((opt = getopt(argc, argv, "sEbt:vh::")) != -1){
		switch(opt){
		case 's':
			ret.s = atoi(argv[optind]);
			break;
		case 'E':
			ret.E = atoi(argv[optind]);
			break;
		case 'b':
			ret.b = atoi(argv[optind]);
			break;
		case 't':
			//printf(">> %s \n", optarg);
			//printf(">> %s \n", argv[optind]);
			//*traceFilePath = argv[optind];
			*traceFilePath = optarg;
			break;
		case 'v':
			//printf(" v ");
			*verboseFlag = 1;
			break;
		case 'h':
			*helpFlag = 1;
		default:
			ret.s = 0;
			ret.E = 0;
			ret.b = 0;

			*helpFlag = 0;
			*verboseFlag = 0;
			*traceFilePath = NULL;
			return ret;
		}
	}

	return ret;
}

/*
			printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
			printf("Options:\n");
			printf("-h         Print this help message.\n");
			printf("-v         Optional verbose flag.\n");
			printf("-s <num>   Number of set index bits.\n");
			printf("-E <num>   Number of lines per set.\n");
			printf("-b <num>   Number of block offset bits.\n");
			printf("-t <file>  Trace file.\n");
			printf("\n");
			printf("Examples:\n");
			printf("linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
			printf("linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
			*/

void		getSummary(SIM_ACT summary[ACT_NUM], 
						Node* resultListArr[],
						int resArrLen)
{
	summary[MISS]	= 0;
	summary[HIT] 	= 0;
	summary[EVICT]	= 0;	
	for(int i = 0; i < resArrLen; i++){
		Node* cursor = resultListArr[i];
		while(cursor != NULL){
			switch(cursor->act){
			case HIT:
				summary[HIT]++;
				break;
			case MISS:
				summary[MISS]++;
				break;
			case EVICT:
				summary[EVICT]++;
				break;
			default:
				printf("wtf in summary");
			}
			cursor = cursor->next;
		}
	}
}

FILE*		traceFileOpen(char* path)
{
	FILE*	retFile	= fopen(path, "r");
	if(retFile == NULL) {
		perror("File opening failed!");
		exit(EXIT_FAILURE);
	}
	return retFile;
}

/****************************************************
 *		Cache functions
 ****************************************************/
Cache*		createCache(Tuple_sEb sEb)
{
	// init Cache 
	Cache*	cache	= malloc( sizeof(Cache) );
	cache->S = (1 << sEb.s);	// S = 2^s
	cache->E = sEb.E;
	cache->setArr = malloc( cache->S * sizeof(Set) );

	// init Set(s) 
	for(int i = 0; i < cache->S; i++){
		cache->setArr[i].queue = NULL;	// init Empty queue(s)
		cache->setArr[i].lineArr 
			= malloc( cache->E * sizeof(Line) );
		// init Line(s)
		for(int j = 0; j < sEb.E; j++){
			cache->setArr[i].lineArr[j].isValid = false;
			cache->setArr[i].lineArr[j].tag = 0U;
		}
	}

	return cache;
}
		
Node*		runCache(Cache* cache, Trace trace, Tuple_sEb sEb)
{
	size_t			s			= (size_t)sEb.s;
	size_t			E			= (size_t)sEb.E;
	size_t			b			= (size_t)sEb.b;

	char			act			= trace.act;
	unsigned long	addr		= trace.addr;

	unsigned		S			= cache->S;

	Node*			retListHead	= NULL;
	Node*			retListTail	= NULL;

	// select a Set in Cache.
	unsigned		idx			= getSetIndex(addr,s,b) % S;
	Set*			set			= cache->setArr + idx;
	Node**			queue		= &set->queue;
		
	// select a Line in Set: 
	Line*			line		= NULL;
	unsigned		lineTag		= 0;
	//TODO: need to move ^ this variables later...
	unsigned		traceTag	= getTag(addr,s,b);

	// line search: valid & tag matched 
	int i;
	int validLineNum;
	for(i = 0, validLineNum = 0; i < E; i++){
		bool		isLineValid	= false;
		line		= set->lineArr + i;
		lineTag		= line->tag;	
		isLineValid	= line->isValid;

		if(isLineValid){
			if(traceTag == lineTag){ // cache hit!
				retListTail = retListHead = createLinkedList(HIT);
				enqueue(queue, line);
				deleteNodesDuplicateToTail(queue);
				break; 
			}
			//valid but tag isn't matched! 
			//continue iteration.
			validLineNum++; 
		}
		else{// COLD cache MISS!
			retListTail = retListHead = createLinkedList(MISS);
			//line update
			line->tag = traceTag;
			line->isValid = true;
			enqueue(queue, line);
			deleteNodesDuplicateToTail(queue);
			break;
		}
	}
	// ALL lines are valid but no mathed line for trace!
	if(validLineNum == E){
		// so cache MISS!
		retListTail = retListHead = createLinkedList(MISS);
		// and EVICTION occurred!
		retListTail = addNode(retListTail, EVICT);
		//eviction process 
		Line* evictedLine = dequeueAndFree(queue); 
		evictedLine->tag = traceTag;
		enqueue(queue, evictedLine);
		deleteNodesDuplicateToTail(queue);
	}

	if(act == 'M'){ // same set, same tag, 100% valid, 100% HIT
		// cache hit!
		retListTail = addNode(retListTail, HIT);
		enqueue(queue, line);
		deleteNodesDuplicateToTail(queue);
	}
	else if(act != 'L' && act != 'S'){
		printf("critical error! invalid Trace input: %c", act);
		exit(EXIT_FAILURE);
	}

	return retListHead;
}


/*--------------------------*/
#ifdef RELEASE
int main(int argc, char* argv[])
{
	int			hflag		= -1;
	int			vflag		= -1; 
	char*		tFilePath	= NULL;
	Tuple_sEb	sEb			= interpretOptions(argc, argv, 
												&tFilePath, 
												&hflag, &vflag);

	FILE*		pTraceFile	= traceFileOpen(tFilePath);
	int			len			= getNumOfValidLines(pTraceFile);
	Node**		resLists	= malloc( len*sizeof(Node*) ); 
	Trace*		traceArr	= malloc( len*sizeof(Trace) ); 		

	Cache*		cache		= createCache(sEb);
	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

    printSummary( summary[HIT], summary[MISS], summary[EVICT] );
	free(resLists);
	free(traceArr);

    return 0;
}
#endif
/*--------------------------*/


/*--------------------------*/
#ifndef RELEASE

#include <criterion/criterion.h>


Test(traceFileToTraceArr, daveTraceRow1){
	//given
	FILE*	pTraceFile	= traceFileOpen("traces/dave.trace");
	Trace*	traceArr	= malloc(5 * sizeof(Trace));

	//when
	char act; void** addr; int size;
	int e = fscanf(pTraceFile, " %c %p,%d", 
				&act, &addr, &size);
	traceArr[0].act  = act;
	traceArr[0].addr = (unsigned long)addr;
	traceArr[0].size = size;

	printf(">>%d \n", e);
	//then
	cr_assert_eq(traceArr[0].act, 'L', 
			"act = %d != %d", traceArr[0].act, 'L');
	cr_assert_eq(traceArr[0].addr, 0x10, 
			"addr = %d != 10", traceArr[0].addr);
	cr_assert_eq(traceArr[0].size, 4 );

	fclose(pTraceFile);
	free(traceArr);
}

Test(traceFileToTraceArr, inCaseOfI){
	//given
	FILE*	pTraceFile	= traceFileOpen("traces/trans.trace");
	int		len			= getNumOfValidLines(pTraceFile);
	Trace	traceArr[len];
	
	//when
	traceFileToTraceArr(pTraceFile, traceArr);

	//then: omit I line!!
	//cr_assert_eq(traceArr[1].act, 'S');
	cr_assert_eq(traceArr[1].act, 'S', 
			"act = %c != %c", traceArr[1].act, 'S');
	cr_assert_eq(traceArr[1].addr, 0x7ff000398, 
			"addr = %x != 0x7ff000398", traceArr[1].addr);
	cr_assert_eq(traceArr[1].size, 8 );
	//read file upto end!
	cr_assert_eq(traceArr[len-1].act, 'L', 
			"act = %c != %c", traceArr[len-1].act, 'L');
	cr_assert_eq(traceArr[len-1].addr, 0x00600aa0, 
			"addr = %x != 0x00600aa0", traceArr[len-1].addr);
	cr_assert_eq(traceArr[len-1].size, 1 );

	fclose(pTraceFile);
}

Test(traceFileToTraceArr, getNumOfValidLines){
	//given
	FILE*	pTraceFile	= traceFileOpen("traces/dave.trace");
	FILE*	pTraceFile2	= traceFileOpen("traces/dummy.trace");

	//when
	int		len			= getNumOfValidLines(pTraceFile);
	int		len2		= getNumOfValidLines(pTraceFile2);
	
	//then
	cr_assert_eq(len, 5);
	cr_assert_eq(len2,4);

	fclose(pTraceFile);
	fclose(pTraceFile2);
}


Test(addressToThreePart, getXXXfunctions){
	//given
	Trace			trace	= {'X', 0xba987654321UL, 8};

	//when
	unsigned		offset	= getOffset(trace.addr, 8);
	unsigned		index	= getSetIndex(trace.addr, 12, 8);
	unsigned long	tag		= getTag(trace.addr, 12, 8);

	//then
	cr_assert_eq(offset, 0x21, "offset = 0x%x != 0x21", offset);
	cr_assert_eq(index, 0x543, "index = 0x%x != 0x543", index);
	cr_assert_eq(tag, 0xba9876,"tag = 0x%lx != 0xba9876", tag);
}


void expect_interpretation_eq(Tuple_sEb actual_sEb, 
							int actual_hflag, int actual_vflag,
							char* actual_trace_file_path,
							Tuple_sEb expect_sEb,
							int expect_hflag, int expect_vflag,
							char* expect_trace_file_path)
{
	cr_expect_str_eq(actual_trace_file_path, actual_trace_file_path);
	cr_expect_eq(actual_hflag, expect_hflag);
	cr_expect_eq(actual_vflag, expect_vflag);

	cr_expect_eq(actual_sEb.s, expect_sEb.s,
			"actual_sEb != expect_sEb \n s: %d != %d \n", 
			actual_sEb.s, expect_sEb.s);
	cr_expect_eq(actual_sEb.E, expect_sEb.E,
			"actual_sEb != expect_sEb \n E: %d != %d \n",
			actual_sEb.E, expect_sEb.E);
	cr_expect_eq(actual_sEb.b, expect_sEb.b,
			"actual_sEb != expect_sEb \n b: %d != %d \n",
			actual_sEb.b, expect_sEb.b);
}

Test(argvToOptions, interpretOptions){
	//given
	char*		expFilePath	= "traces/trans.trace";
	int			argc		= 9;
	char*		argv[9]		= {"./csim", 
								"-s", "2",
								"-E", "4",
								"-b", "3",
								"-t", expFilePath};
	Tuple_sEb	exp_sEb		= {2, 4, 3};

	//when
	int			hflag		= -1;
	int			vflag		= -1; 
	char*		tFilePath	= NULL;
	Tuple_sEb	act_sEb		= interpretOptions(argc, argv, 
												&tFilePath, 
												&hflag, &vflag);

	//then
	expect_interpretation_eq(act_sEb, hflag, vflag, tFilePath,
							exp_sEb, 0, 0, expFilePath);
}


Test(argvToOptions, interpretOptions2){
	//given
	char*		expFilePath	= "traces/yi.trace";
	Tuple_sEb	exp_sEb		= {4, 1, 4};

	int			argc		= 10;
	char*		argv[10]	= {"./csim-ref", 
								"-v",
								"-s", "4",
								"-E", "1",
								"-b", "4",
								"-t", expFilePath};

	//when
	char*		tFilePath	= NULL;
	int			hflag		= -1;
	int			vflag		= -1; 
	Tuple_sEb	act_sEb		= interpretOptions(argc, argv, 
												&tFilePath, 
												&hflag, &vflag);

	//then
	expect_interpretation_eq(act_sEb, hflag, vflag, tFilePath,
							exp_sEb, 0, 1, expFilePath);
}

Test(argvToOptions, returnEmptyTupleIfErrorOccurred_invalidOption){
	//given
	char*		expFilePath	= "traces/yi.trace";
	Tuple_sEb	exp_sEb		= {0, 0, 0};

	int			argc		= 11;
	char*		argv[11]	= {"./csim-ref", 
								"-v",
								"-error",	//invalid option	
								"-s", "4",
								"-E", "1",
								"-b", "4",
								"-t", expFilePath};

	//when
	char*		tFilePath	= NULL;
	int			hflag		= -1;
	int			vflag		= -1; 
	Tuple_sEb	act_sEb		= interpretOptions(argc, argv, 
												&tFilePath, 
												&hflag, &vflag);

	//then
	cr_expect_eq(act_sEb.s, exp_sEb.s,
			"act_sEb != exp_sEb \n s: %d != %d \n", 
			act_sEb.s, exp_sEb.s);
	cr_expect_eq(act_sEb.E, exp_sEb.E,
			"act_sEb != exp_sEb \n E: %d != %d \n",
			act_sEb.E, exp_sEb.E);
	cr_expect_eq(act_sEb.b, exp_sEb.b,
			"act_sEb != exp_sEb \n b: %d != %d \n",
			act_sEb.b, exp_sEb.b);
}


Test(linkedList, createNodeAndLink){
	//given
	Node	node3 = { {EVICT}, NULL };
	Node	node2 = { {HIT}, &node3 };
	Node	head = { {MISS}, &node2 };
	
	//then
	cr_assert_eq(head.act, MISS);
	cr_assert_eq(head.next->act, HIT);
	cr_assert_eq(head.next->next->act, EVICT);
}

Test(linkedList, createLinkedListAndAddNodesAndFree){
	//given
	Node*	head		= NULL;
	SIM_ACT	firstVal	= MISS;

	//when
	head = createLinkedList(firstVal);	
	if(head == NULL){
		printf("heap allocation error occurred! \n");
		exit(EXIT_FAILURE);
	}
	Node*	tail = addNode(head, HIT);	// add HIT node

	tail = addNode(tail, EVICT);	// add EVICT node to tail and update tail
	//then
	cr_assert_eq(head->act, MISS);
	cr_assert_eq(head->next->act, HIT);
	cr_assert_eq(head->next->next->act, EVICT);

	//free 
	freeList(&head);
	cr_assert_eq(head, NULL);
}

Test(linkedList, printList){
	Node*	head		= NULL;
	SIM_ACT	firstVal	= MISS;
	//printList(head); // "empty list"

	head = createLinkedList(firstVal);	
	if(head == NULL){
		printf("heap allocation error occurred! \n");
		exit(EXIT_FAILURE);
	}
	Node*	tail = addNode(head, HIT);	// add HIT node
	tail = addNode(tail, EVICT);	// add EVICT node to tail
	//printList(head); // "HIT HIT EVICT"

	freeList(&head);
}

Test(linkedList, getLength_Empty){
	//given
	Node*	list = NULL;
	//when
	int len = getLength(list);
	//then
	cr_assert_eq(len, 0);
}

Test(linkedList, getLength_notEmpty){
	//given
	Node*	list = createLinkedList(MISS);
	//when
	int len = getLength(list);
	//then
	cr_assert_eq(len, 1);
}

Test(linkedList, getTail){
	//given
	Node*	list		= createLinkedList(MISS);
	Node*	tail		= addNode(list, HIT);
	tail = addNode(tail, EVICT);
	tail = addNode(tail, HIT);
	//when
	Node*	gottenTail	= getTail(list);
	SIM_ACT	actOfTail	= gottenTail->act;
	//then
	cr_assert_eq(actOfTail, tail->act);
	cr_assert_eq(gottenTail, tail);
}

Test(linkedList, ifListDidntAllocatedThen_getTail_returnNULL){
	//given
	Node*	head	= NULL;
	//when
	Node*	tail	= getTail(head);
	//then
	cr_assert_null(tail);
}

Test(queue, emptyQto10filledQ){
	//given
	Node*	queue	= NULL;
	
	//when
	long i;		//printList(queue);
	for(i = 0; i < 10; i++){
		i64sPRINT(i);
		enqueue(&queue, (Line*)(i+4));
		//printList(queue);
	}
	
	//then
	i = 4;
	Node*	cursor	= queue;
	Line*	expected;
	while(cursor->next != NULL){
		expected = (Line*)i;
		lxEXPECT_EQ(cursor->ptr,expected);
		i++;
		cursor = cursor->next;
	}

	dPRINTn("len", getLength(queue));
	//printList(queue);
	dASSERT_EQ(getLength(queue),10);
}

Test(queue, successful_enqueue_inCaseOfEmptyQueue){
	//given
	Node*	queue	= NULL;
	Line*	line	= NULL;
	int		len		= 0;

	//when
	Node*	endOfQ	= enqueue(&queue, line);
	len = getLength(queue);
				//DBG: printf(">>? %p \n", queue);
	//then
	cr_expect_eq(len, 1, "len = %d", len);
	cr_assert_eq( peek(queue), line );
	cr_assert_eq( endOfQ->ptr, line );
}

Test(queue, successful_enqueue_notEmptyQueue){
	puts(" [successful_enqueue_notEmptyQueue]");
	//given
	Line*	line1	= (Line*)10;
	Line*	line2	= (Line*)20;
	Node*	queue	= NULL;
	Node*	endOfQ	= NULL;
	int		len		= 0;

	//when
										
	endOfQ	= enqueue(&queue, line1);	
	endOfQ	= enqueue(&queue, line2);	
	len = getLength(queue);

	//then
	cr_expect_eq(len, 2, "len = %d", len);
	pASSERT_EQ(endOfQ->ptr, line2);
}

Test(queue, dequeue){
	//given
	int		len			= 0;
	Node*	extracted	= NULL;

	Node*	queue		= NULL;
	Line*	line1		= (Line*)1;
	Line*	line2		= (Line*)2;

	enqueue(&queue, line1);
	enqueue(&queue, line2);
	int before = len = getLength(queue);
	
	//when
	extracted = dequeue(&queue);
	len = getLength(queue);

	//then
	cr_assert_eq(extracted->ptr, line1);
	cr_expect_eq(queue->ptr, line2);
	cr_assert_eq(len, before-1, "len = %d != %d", len, before-1);

	free(extracted); // it's annoying!
}

Test(queue, ifOneNodeInQueueThen_dequeue_makeQueuePtrNULL){
	//given
	Node*	extracted	= NULL;
	Node*	queue		= NULL;
	Line*	line1		= (Line*)1;

	enqueue(&queue, line1);
	cr_assert_eq(queue->ptr, line1);

	//when
	extracted = dequeue(&queue);

	//then
	cr_assert_eq(extracted->ptr, line1);
	cr_assert_eq(queue, NULL);
	cr_assert_eq(getLength(queue), 0);

	free(extracted); // it's annoying!
}

Test(queue, dequeueAndFree){
	printf(" [dequeueAndFree] \n");
	//given
	int		len			= 0;
	Line*	extracted	= NULL;

	Node*	queue		= NULL;
	Line*	line1		= (Line*)10;
	Line*	line2		= (Line*)20;
	puts("---enqueue(&queue, line1);---");
											
	enqueue(&queue, line1);					//printList(queue);
	puts("---enqueue(&queue, line2);---");
	enqueue(&queue, line2);					//printList(queue);
	int before = len = getLength(queue);	
	
	//when
	puts("---extracted = dequeueAndFree(&queue);---");
	extracted = dequeueAndFree(&queue);		//printList(queue);
	len = getLength(queue);

	//then
	cr_assert_eq(extracted, line1, "ext:%p != %p:line1\n", extracted, line1);
	cr_assert_eq(queue->ptr, line2);
	cr_assert_eq(len, before-1, "len = %d != %d", len, before-1);
}

Test(queue, dequeueAndFree_makeQueueNULL){
	//given
	Line*	extracted	= NULL;

	Node*	queue		= NULL;
	Line*	line1		= (Line*)1;

	enqueue(&queue, line1);
	cr_assert_eq(queue->ptr, line1);
	
	//when
	extracted = dequeueAndFree(&queue);

	//then
	cr_assert_eq(extracted, line1);
	cr_assert_eq(queue, NULL);
}

Test(queue, ifEmptyQueueThen_deleteAndFreeOneDuplicate_dontDoAnything){
	//given
	Node*	queue	= NULL;
	//then
	deleteNodesDuplicateToTail(&queue);
	cr_assert_eq( getLength(queue), 0 );
}

Test(queue, ifQueueLengthIsOneThen_deleteAndFreeOneDuplicate_dontDoAnything){
	//given
	Line*	line	= (Line*)150;
	Node*	queue	= NULL;
	//when
	enqueue(&queue, line);
	//then
	deleteNodesDuplicateToTail(&queue);	
	cr_assert_eq( getLength(queue), 1 );
}

Test(queue, deleteAndFreeOneDuplicate_AAA){
	puts(" [AAA]");
	//given
	Line*	line	= (Line*)0xA;
	Node*	queue	= NULL;
	//when
	enqueue(&queue, line);
	enqueue(&queue, line);
	enqueue(&queue, line);
	
	//then
	deleteNodesDuplicateToTail(&queue);	//printList(queue);
	dASSERT_EQ( getLength(queue), 1 );
}

Test(queue,	deleteNodesDuplicateToTail_deleteHead){
	puts(" [ABA]");
	//given
	Line*	lineA	= (Line*)0xA;
	Line*	lineB	= (Line*)0xB;
	Node*	queue	= NULL;
	//when
	enqueue(&queue, lineA);
	enqueue(&queue, lineB);
	enqueue(&queue, lineA);
	//then
			//printList(queue);
	deleteNodesDuplicateToTail(&queue);
			//printList(queue);
	dASSERT_EQ( getLength(queue), 2 );	
	dASSERT_EQ( queue->val, (int64_t)lineB ); 
	dASSERT_EQ( queue->next->val, (int64_t)lineA );
}

Test(queue,	deleteNodesDuplicateToTail_deleteMany){
	puts(" [ABABA]");
	//given
	Line*	lineA	= (Line*)0xA;
	Line*	lineB	= (Line*)0xB;
	Node*	queue	= NULL;
	//when
	enqueue(&queue, lineA);//'
	enqueue(&queue, lineB);
	enqueue(&queue, lineA);//'
	enqueue(&queue, lineB);
	enqueue(&queue, lineA);
	//then
			//printList(queue);
	deleteNodesDuplicateToTail(&queue);
			//printList(queue);
	dASSERT_EQ( getLength(queue), 3 );
	dASSERT_EQ( queue->val, (int64_t)lineB );
	dASSERT_EQ( queue->next->val, (int64_t)lineB );
	dEXPECT_EQ( queue->next->next->val, (int64_t)lineA );
}

Test(queue,	deleteNodesDuplicateToTail_deleteMany2){
	puts(" [ABABBABA]");
	//given
	Line*	lineA	= (Line*)0xA;
	Line*	lineB	= (Line*)0xB;
	Node*	queue	= NULL;
	//when
	enqueue(&queue, lineA);//'
	enqueue(&queue, lineB);
	enqueue(&queue, lineA);//'
	enqueue(&queue, lineB);
	enqueue(&queue, lineB);
	enqueue(&queue, lineA);//'
	enqueue(&queue, lineB);
	enqueue(&queue, lineA);
	//then
			//printList(queue);
	deleteNodesDuplicateToTail(&queue);
			//printList(queue);
	dASSERT_EQ( getLength(queue), 5 );
	dASSERT_EQ( queue->val, (int64_t)lineB );
	dASSERT_EQ( queue->next->val, (int64_t)lineB );
	dEXPECT_EQ( queue->next->next->val, (int64_t)lineB );
	dEXPECT_EQ( queue->next->next->next->val, (int64_t)lineB );
	dEXPECT_EQ( queue->next->next->next->next->val, (int64_t)lineA );
}
Test(queue,	deleteNodesDuplicateToTail_deleteMiddle){
	puts(" [CAACBAA]");
	//given
	Line*	lineA	= (Line*)0xA;
	Line*	lineB	= (Line*)0xB;
	Line*	lineC	= (Line*)0xC;
	Node*	queue	= NULL;
	//when
	enqueue(&queue, lineC);
	enqueue(&queue, lineA);// <--
	enqueue(&queue, lineA);// <--
	enqueue(&queue, lineC);
	enqueue(&queue, lineB);
	enqueue(&queue, lineA);// <--
	enqueue(&queue, lineA);
	//then
			//printList(queue);
	deleteNodesDuplicateToTail(&queue);
			//printList(queue);
	dEXPECT_EQ( getLength(queue), 4 );
	dEXPECT_EQ( queue->val, (int64_t)lineC );
	dEXPECT_EQ( queue->next->val, (int64_t)lineC );
	dEXPECT_EQ( queue->next->next->val, (int64_t)lineB );
	dEXPECT_EQ( queue->next->next->next->val, (int64_t)lineA );
}
Test(cacheSimulator, createCache){
	//given
	Tuple_sEb	sEb		= { 3, 4, 5 };
	int			S		= (1 << sEb.s);

	//when
	Cache*		cache	= createCache(sEb);

	//then
	cr_assert_eq(cache->S, S);
	cr_assert_eq(cache->E, sEb.E);

	for(int i = 0; i < S; i++){
		// assert Set(s) allocation
		cr_assert_null(cache->setArr[i].queue);
		// assert Line(s) allocation
		for(int j = 0; j < sEb.E; j++){
			cr_assert_eq( cache->setArr[i].lineArr[j].isValid, false );
			cr_assert_eq( cache->setArr[i].lineArr[j].tag, 0U);
		}
	}
	
	//TODO: FREE CACHE!!!
}



Test(cacheSimulator, optionsToResultString){
	cr_skip_test("create Cache first!");
	//given
	char* resultStr = "nope";
	
	//when
	
	//then
	cr_expect_str_eq(resultStr,"L 10,4 miss\nS 18,4 hit\nL 20,4 miss\nS 28,4 hit\nS 50,4 miss\n");
}


// vtq means: valid? / tag mathced? / full queue? / act
Test(cacheSimulator, whenColdCacheLineLoad_vtq_000L){
	//given
	//-v -s 2 -E 1 -b 2 -t traces/dave.trace
	Tuple_sEb	sEb			= { 2, 1, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		trace		= { 'L', 0, 1 };
	Node*		resLists[1]; 
	//when
	resLists[0] = runCache(cache, trace, sEb);
	//then
	cr_expect_eq(resLists[0]->act, MISS);
	//cr_expect_eq(resLists[1]->act, HIT);

	//TODO: FREE CACHE!!!
	//TODO: FREE resLists!
}

Test(cacheSimulator, whenFullSetAndWarmedLineLoad_vtq_111L){
	//given
	puts(" [whenFullSetAndWarmedLineLoad_vtq_111L]");
	//-v -s 2 -E 1 -b 2 -t traces/
	Tuple_sEb	sEb			= { 2, 1, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		warmup		= { 'L', 0x10, 1 };
	Trace		trace		= { 'L', 0x12, 1 };
	Node*		resLists[2]; 
	//when
	resLists[0] = runCache(cache, warmup, sEb);
	resLists[1] = runCache(cache, trace, sEb);
	//then
	cr_expect_eq(resLists[0]->act, MISS, 
			"%d != 1 = MISSexp", resLists[0]->act);
	cr_expect_eq(resLists[1]->act, HIT, 
			"%d != 0 = HITexp", resLists[1]->act);

	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheSimulator, whenColdCacheLineModify_vtq_000M){
	//given
	Tuple_sEb	sEb			= { 2, 1, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		trace		= { 'M', 0x20, 1 };
	Node*		resLists[2]; 
	//when
	resLists[0] = runCache(cache, trace, sEb);
	//then
	cr_expect_eq(resLists[0]->act, MISS, 
			"MISS = 1 != %d", resLists[0]->act);
	cr_expect_eq(resLists[0]->next->act, HIT, 
			"HIT = 0 != %d", resLists[0]->next->act);

	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheSimulator, whenQueueIsNotEmptyAndTagNotMatching_vtq_100L){
	//given
	Tuple_sEb	sEb			= { 2, 3, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		warmup		= { 'L', 0x10, 1 };
	Trace		trace		= { 'L', 0x20, 1 };
	Node*		resLists[2]; 
	//when
	resLists[0] = runCache(cache, warmup, sEb);
	resLists[1] = runCache(cache, trace, sEb);
	//then
	cr_expect_eq(resLists[0]->act, MISS, 
			"MISS = 1 != %d", resLists[0]->act);
	cr_expect_eq(resLists[1]->act, MISS, 
			"MISS = 1 != %d", resLists[1]->act);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheSimulator, whenQueueIsNotEmptyAndTagNotMatching_vtq_100M){
	//given
	Tuple_sEb	sEb			= { 2, 3, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		warmup		= { 'L', 0x10, 1 };
	Trace		trace		= { 'M', 0x20, 1 };
	Node*		resLists[2]; 
	//when
	resLists[0] = runCache(cache, warmup, sEb);
	resLists[1] = runCache(cache, trace, sEb);
	//then
	cr_expect_eq(resLists[0]->act, MISS, 
			"MISS = 1 != %d", resLists[0]->act);
	cr_expect_eq(resLists[1]->act, MISS, 
			"MISS = 1 != %d", resLists[1]->act);
	cr_expect_eq(resLists[1]->next->act, HIT, 
			"HIT = 0 != %d", resLists[1]->next->act);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheSimulator, whenQueueIsNotEmptyAndTagMatched_vtq_110L){
	//given
	Tuple_sEb	sEb			= { 2, 3, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		warmup		= { 'L', 0x10, 1 };
	Trace		trace		= { 'L', 0x10, 1 };
	Node*		resLists[2]; 
	//when
	resLists[0] = runCache(cache, warmup, sEb);
	resLists[1] = runCache(cache, trace, sEb);
	//then
	cr_expect_eq(resLists[0]->act, MISS, 
			"MISS = 1 != %d", resLists[0]->act);
	cr_expect_eq(resLists[1]->act, HIT, 
			"HIt = 0 != %d", resLists[1]->act);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheSimulator, whenQueueIsNotEmptyAndTagMatched_vtq_110M){
	//given
	Tuple_sEb	sEb			= { 2, 3, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		warmup		= { 'L', 0x10, 1 };
	Trace		trace		= { 'M', 0x12, 1 };
	Node*		resLists[2]; 
	//when
	resLists[0] = runCache(cache, warmup, sEb);
	resLists[1] = runCache(cache, trace, sEb);
	//then
	cr_expect_eq(resLists[0]->act, MISS, 
			"MISS = 1 != %d", resLists[0]->act);
	cr_expect_eq(resLists[1]->act, HIT, 
			"HIT = 0 != %d", resLists[1]->act);
	cr_expect_eq(resLists[1]->next->act, HIT, 
			"HIT = 0 != %d", resLists[1]->next->act);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheSimulator, whenQueueIsFullAndTagNotMatched_vtq_101L__EVICT){
	puts(" [whenQueueIsFullAndTagNotMatched_vtq_101L__EVICT]");
	//given
	Tuple_sEb	sEb			= { 2, 1, 2 };
	Cache*		cache		= createCache(sEb);
	Trace		warmup		= { 'L', 0x10, 1 };
	Trace		trace		= { 'L', 0x20, 1 };
	Node*		resLists[2]; 
	//when
		//puts("run cache caller before");
		//printList(cache->setArr[0].queue);	// queue has not be changed! ptr..
	resLists[0] = runCache(cache, warmup, sEb);		
		//puts("run cache caller after");
		//printList(cache->setArr[0].queue);	// queue has not be changed! ptr..
	resLists[1] = runCache(cache, trace, sEb);		//printList(resLists[1]);
	//then
	cr_expect_eq(resLists[0]->act, MISS, 
			"MISS = 1 != %d", resLists[0]->act);
	cr_expect_eq(resLists[1]->act, MISS, 
			"MISS = 1 != %d", resLists[1]->act);
	cr_expect_eq(resLists[1]->next->act, EVICT, 
			"EVICT= 2 != %d", resLists[1]->next->act);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

void assertSummary(SIM_ACT summary[ACT_NUM], 
				int hit, int miss, int evict)
{
	cr_expect_eq(summary[HIT], hit,
			"actual HIT: %d != %d :exp", summary[HIT], hit);
	cr_expect_eq(summary[MISS], miss,
			"actual MISS: %d != %d :exp", summary[MISS], miss);
	cr_expect_eq(summary[EVICT], evict,
			"actual EVICT: %d != %d :exp", summary[EVICT], evict);
}

Test(cacheAndTraceIntergration, dave_sEb_414_noEvictionCase){
	//given
	// -v -s 4 -E 1 -b 4 -t traces/dave.trace
	FILE*		pTraceFile		= traceFileOpen("traces/dave.trace");
	int			len				= getNumOfValidLines(pTraceFile);
	Trace		traceArr[len];

	Tuple_sEb	sEb				= { 4, 1, 4 };
	Cache*		cache			= createCache(sEb);
	Node*		resLists[len]; 
	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	//when
	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

	//then
	puts("---- show time! ----");
	for(int i = 0; i < len; i++){
		//printList(resLists[i]);
	}
	puts("----    end    ----");

	assertSummary(summary, 2,3,0); 
	
	fclose(pTraceFile);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheAndTraceIntergration, yi2_sEb_411_noEvictionCase){
	//given
	// -v -s 4 -E 1 -b 1 -t traces/yi2.trace
	FILE*		pTraceFile		= traceFileOpen("traces/yi2.trace");
	int			len				= getNumOfValidLines(pTraceFile);
	Trace		traceArr[len];

	Tuple_sEb	sEb				= { 4, 1, 1 };
	Cache*		cache			= createCache(sEb);
	Node*		resLists[len]; 
	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	//when
	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

	//then
	puts("---- show time! ----");
	for(int i = 0; i < len; i++){
		//printList(resLists[i]);
	}
	puts("----    end    ----");

	assertSummary(summary, 9, 8, 0);

	fclose(pTraceFile);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheAndTraceIntergration, evictionTest1_sEb_242_LRU){
	//cr_skip_test("q first");
	puts(" [evictionTest1_sEb_242_LRU]");
	//given
	// -v -s 2 -E 4 -b 2 -t traces/myEvictionTest.trace 
	FILE*		pTraceFile		= traceFileOpen("traces/myEvictionTest.trace");
	int			len				= getNumOfValidLines(pTraceFile);
	Node*		resLists[len]; 
	Trace		traceArr[len];

	Tuple_sEb	sEb				= { 2, 4, 2 };
	Cache*		cache			= createCache(sEb);

	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	//when
	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

	//then
	puts("---- show time! ----");
	for(int i = 0; i < len; i++){
		Trace trace = traceArr[i];
		printf("%c %lx ", trace.act, trace.addr);
		printList(resLists[i]);
	}
	puts("----    end    ----");

	assertSummary(summary, 4, 8, 4);

	fclose(pTraceFile);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheAndTraceIntergration, evictionTest2_sEb_242_LRU){
	puts(" [evictionTest2_sEb_242_LRU]");
	//given
	// -v -s 2 -E 4 -b 2 -t traces/myEvictionTest.trace 
	FILE*		pTraceFile		= traceFileOpen("traces/myEvictionTest2.trace");
	int			len				= getNumOfValidLines(pTraceFile);
	Node*		resLists[len]; 
	Trace		traceArr[len];

	Tuple_sEb	sEb				= { 2, 4, 2 };
	Cache*		cache			= createCache(sEb);

	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	//when
	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

	//then
	puts("---- show time! ----");
	for(int i = 0; i < len; i++){
		Trace trace = traceArr[i];
		printf("%c %lx ", trace.act, trace.addr);
		printList(resLists[i]);
	}
	puts("----    end    ----");

	assertSummary(summary, 3, 7, 3);

	fclose(pTraceFile);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheAndTraceIntergration, trans_sEb_243_LRU){
	//cr_skip_test("later");
	puts(" [trans_sEb_243_LRU]");
	//given
	// -v -s 2 -E 4 -b 3 -t traces/trans.trace 
	FILE*		pTraceFile		= traceFileOpen("traces/trans.trace");
	int			len				= getNumOfValidLines(pTraceFile);
	Node*		resLists[len]; 
	Trace		traceArr[len];

	Tuple_sEb	sEb				= { 2, 4, 3 };
	Cache*		cache			= createCache(sEb);

	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	//when
	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

	//then
	puts("---- show time! ----");
	for(int i = 0; i < len; i++){
		//Trace trace = traceArr[i];
		//printf("%c %lx ", trace.act, trace.addr);
		//printList(resLists[i]);
	}
	puts("----    end    ----");

	assertSummary(summary, 212, 26, 10);

	fclose(pTraceFile);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}


Test(cacheAndTraceIntergration, evictionTest3_sEb_242_LRU){
	//cr_skip_test("later LRU");
	puts(" [evictionTest3_sEb_242_LRU]");
	//given
	// -v -s 2 -E 4 -b 2 -t traces/myEvictionTest.trace 
	FILE*		pTraceFile		= traceFileOpen("traces/myEvictionTest3.trace");
	int			len				= getNumOfValidLines(pTraceFile);
	Node*		resLists[len]; 
	Trace		traceArr[len];		

	Tuple_sEb	sEb				= { 2, 4, 2 };
	Cache*		cache			= createCache(sEb);

	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	//when
	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

	//then
	puts("---- show time! ----");
	for(int i = 0; i < len; i++){
		Trace trace = traceArr[i];
		printf("%c %lx ", trace.act, trace.addr);
		printList(resLists[i]);
	}
	puts("----    end    ----");

	assertSummary(summary, 7, 12, 8);

	fclose(pTraceFile);
	//TODO: FREE CACHE!!!
	//TODO: FREE ENTRIES of resLists!
}

Test(cacheAndTraceIntergration, longTrace){
	puts(" [long]");
	//given
	FILE*		pTraceFile	= traceFileOpen("traces/long.trace");
	int			len			= getNumOfValidLines(pTraceFile);
	Node**		resLists	= malloc( len*sizeof(Node*) ); 
	Trace*		traceArr	= malloc( len*sizeof(Trace) ); 		

	Tuple_sEb	sEb			= { 5, 1, 5 };
	Cache*		cache		= createCache(sEb);

	SIM_ACT		summary[ACT_NUM];

	traceFileToTraceArr(pTraceFile, traceArr);

	//when
	for(int i = 0; i < len; i++){
		resLists[i] = runCache(cache, traceArr[i], sEb);
	}
	getSummary(summary, resLists, len);

	//then
	/*
	puts("---- show time! ----");
	for(int i = 0; i < len; i++){
		Trace trace = traceArr[i];
		printf("%c %lx ", trace.act, trace.addr);
		printList(resLists[i]);
	}
	puts("----    end    ----");
	*/

	free(resLists);
	free(traceArr);
	assertSummary(summary, 265180, 21775, 21743);
}
#endif
/*--------------------------*/
