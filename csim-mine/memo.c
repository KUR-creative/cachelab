trace file -> trace arr
	trace -> Trace
//		I 인스트럭션 로드 무시
//			don't alloc I line
//			don't read I line too!
//		[char, unsigned, int]
//		[형식, 메모리주소, 접근 사이즈]
		
		결과: ?? 나중에 캐시에 넣을 건데 모르겠다 어떻게 될지
			- 일단 트래이스 배열을 만든다.
//	traceArr는 동적할당한다.
//	얼마나 동적할당해야하는가? 파일에서 I를 뺀만큼 해야 한다.
//	I의 경우 앞에 스페이스가 없는데 이 때는 fscanf가 어떻게 작동할까??

	빈 trace file인 케이스는 일단 생각하지 않는다..
	I line 단 한줄인 trace file 케이스 또한 생각하지 않는다.


//매개변수를 받아서 (s,E,b)로
	//use getopt 

“L” a data load,	//means: read
“S” a data store, and 
“M” a data modify (i.e., a data load followed by a data store). 

select Set: set index 
select Line: tag bit matching
	if matched:
		if "line" is invalid:
			if act is 'L':
				change proper "set" means: update "valid bit", "tag", "queue"
				return MISS
			else if act is 'S':
			else if act is 'M':
		else "line" is valid:	// can be a cache hit!
			if act is 'L':
				change proper "set" means: update "valid bit", "tag", "queue"
				return MISS
			else if act is 'S':
			else if act is 'M':

HIT:
MISS:
	tag unmatched
	invalid line
	full set(all line is valid, but tag unmatched)
EVICT:
	full set(MISS -> EVICT)

말하자면 M = L->S다... same address TWICE simulation
if Modify flag is true: ...

000L
split address -> tag, index, offset
select a Set with index;
select a Line with tag
	tag matching...
	if matched: 
		if valid: 
			"hit"
		else: 
			"miss"
			vbit <- true;
			lineTag <- addrTag;
	no matched:
		miss

(s,E,b)를 받아서 캐시(2차원 배열)를 만든다
	현재 set의 모든 line들이 valid할 때, 가장 오래동안 참조되지 않은 line 교체
		큐에 각 line들의 참조를 넣음으로써, 가능
		how to know "least recently used" line??
			each line has "last referecing" time. 
			가장 작은 "마지막 참조"시간을 갖는 line이 least recently used line
	line = [v, tag] // size를 따지지 않음!!!
	set  = [queue, [line,line,line, ... ,line] ]
	cache= [set, set, set ... set]

-v -s 4 -E 1 -b 4 -t traces/dave.trace
L 10,4 miss 
S 18,4 hit 
L 20,4 miss 
S 28,4 hit 
S 50,4 miss 
hits:2 misses:3 evictions:0

캐시 시뮬레이터는 csim-ref와 동일한 문자열을 출력해야 한다.
캐시 시뮬레이터는 갖가지 옵션을 받아서 적절한 문자열을 출력한다

	옵션들 -> char*, printSummary(X,X,X);
		filePath 옵션을 받으면 자동으로 스트림 열고 긁어와서 
		traceFile -> traceArr 하는 함수 필요

	(s,E,b) -> Cache(se) // se means sideEffects.
	createCache: 캐시를 일단 동적 할당하여 메모리에 올려놓고 그걸 갖고 논다.
		Cache를 초기화한다
		Set들을 초기화한다
		line들을 초기화한다.
	freeCache: 캐시 할당을 해제하는데 이때 연관된 모든 것을 해제해야 한다.
		//I can't directly test this function!!
		//I can only test other functions that call this function...
	
	Cache, TraceArr[len] -> "linked lists"[len]
	getResult: Trace 한 줄에서 result 한 줄을 얻는 함수를 반복하여 resultArr 반환
		<inner loop>
		Cache, (act, addr, size) -> "linked list" of ENUM[miss, hit, eviction]
		run: Trace를 받은 Cache로 해석하고 Cache의 상태를 바꾼 후 결과 반환
			//이 함수에서는 캐시가 필요하다.
			//캐시의 구조(s,E,b)와 상태에 따라 또 달라진다.

//"linked list"
//	하나를 만들고 거기에 결과값(hit/miss/evict)들이 계속 연결되어 추가된다.
//	HeadNode는 헤드에 달려있는 걸로 판단하고, 
//	tailNode는 next가 NULL인 놈이다.
//	get length
//	get tail
	Node free function!!!!	-> need to test(via valgrind!)

//캐시를 만들려면 먼저 큐를 만들어야 한다!(현재 dave 케이스 중)
//"queue"
	//연결리스트는 큐로도 사용된다. 이 때 SIM_ACT가 아닌 Line*로 해석한다
		//-> union 을 도입한다! (나중에...)
//	queue용 함수들 만든다
//		넣기
//		빼기
//	빈 큐는 그냥 NULL 그 자체다...!

//(s,E,b)로 Trace의 addr을 3개로 쪼갠다
//	Trace.addr -> (tag, setind, offset)



-v -s 4 -E 1 -b 4 -t traces/yi.trace
L 10,1 miss 
M 20,1 miss hit 
L 22,1 hit 
S 18,1 hit 
L 110,1 miss eviction 
L 210,1 miss eviction 
M 12,1 miss eviction hit 
hits:4 misses:5 evictions:3

-v -s 4 -E 1 -b 4 -t traces/dave.trace
L 10,4 miss 
S 18,4 hit 
L 20,4 miss 
S 28,4 hit 
S 50,4 miss 
hits:2 misses:3 evictions:0

MISS 
HIT 
MISS 
HIT 
MISS 
-v -s 4 -E 1 -b 4 -t traces/yi2.trace
L 0,1 miss 
L 1,1 hit 
L 2,1 hit 
L 3,1 hit 
S 4,1 hit 
L 5,1 hit 
S 6,1 hit 
L 7,1 hit 
S 8,1 hit 
L 9,1 hit 
S a,1 hit 
L b,1 hit 
S c,1 hit 
L d,1 hit 
S e,1 hit 
M f,1 hit hit 
hits:16 misses:1 evictions:0


./csim-ref -v -s 4 -E 1 -b 1 -t traces/yi2.trace
L 0,1 miss 
L 1,1 hit 
L 2,1 miss 
L 3,1 hit 
S 4,1 miss 
L 5,1 hit 
S 6,1 miss 
L 7,1 hit 
S 8,1 miss 
L 9,1 hit 
S a,1 miss 
L b,1 hit 
S c,1 miss 
L d,1 hit 
S e,1 miss 
M f,1 hit hit 
hits:9 misses:8 evictions:0

./csim-ref -v -s 4 -E 1 -b 1 -t traces/dave.trace
L 10,4 miss 
S 18,4 miss 
L 20,4 miss 
S 28,4 miss 
S 50,4 miss eviction 
hits:0 misses:5 evictions:1

어떻게 작동하는거지?
메모리 릭 쩌는 거 같은데..?

L 10 miss 
L 20 miss 
L 30 miss 
L 40 miss 
L 50 miss eviction 
L 60 miss eviction 
L 51 miss eviction 
L 61 miss eviction 
L 31 hit 
L 41 hit 
L 70 miss eviction 
L 51 miss eviction 

 [dequeueAndFree] 
---enqueue(&queue, line1);---
|empty list!
line: : 0xa 
what? :|0xa 
|0xa 
---enqueue(&queue, line2);---
line: : 0x14 
|0xa 0x14 
---extracted = dequeueAndFree(&queue);---
head: 0x18f7de0 
head->ptr: 0x18f7de0 
retLinePtr: 0x7f760000000a 
ret: 0x7f760000000a 
ret: 0x7f760000000a 
|0x14 

 [successful_enqueue_notEmptyQueue]
|empty list!
line: : 0xa 
|0xa 
line: : 0x14 
|0xa 0x14 

 [whenQueueIsFullAndTagNotMatched_vtq_101L__EVICT]
run cache caller before
+empty list!
		why null?
		|0x2232e00 
	cold cache!!!
	|0x2232e00 
	sEb:2,1,2 | act,set,lineTag:L 0 0 | |miss 
run cache caller after
+empty list!
now?
|empty list!
head: (nil) 
|empty list!

./csim-ref -v -s 2 -E 4 -b 2 -t traces/myEvictionTest.trace 
L 10,1 miss 
L 20,1 miss 
L 30,1 miss 
L 40,1 miss 
L 50,1 miss eviction 
L 60,1 miss eviction 
L 51,1 hit 
L 61,1 hit 
L 31,1 hit 
L 41,1 hit 
L 70,1 miss eviction 
L 51,1 miss eviction 
hits:4 misses:8 evictions:4


L 10 |miss 
L 20 |miss 
L 30 |miss 
L 40 |miss 

L 50 |miss eviction 
L 60 |miss eviction 

L 51 |hit 
L 61 |hit 
L 31 |hit 
L 41 |hit 

L 70 |miss eviction 
L 51 |hit	//why?

[evictionTest1_sEb_242_LRU]
0: 0: 0: 0: 
cold cache!
|empty list!
|20 
1: 0: 0: 0: 

1: 0: 0: 0: 
cold cache!
|20 
|20 28 
1: 2: 0: 0: 

1: 2: 0: 0: 
cold cache!
|20 28 
|20 28 30 
1: 2: 3: 0: 

1: 2: 3: 0: 
cold cache!
|20 28 30 
|20 28 30 38 
1: 2: 3: 4: 

1: 2: 3: 4: 
cache miss -> eviction!
|20 28 30 38 
|28 30 38 
evicted: 0x2205520 
|28 30 38 20 
5: 2: 3: 4: 
5: 2: 3: 4: 

5: 2: 3: 4: 
cache miss -> eviction!
|28 30 38 20 
|30 38 20 
evicted: 0x2205528 
|30 38 20 28 
5: 6: 3: 4: 
5: 6: 3: 4: 

5: 6: 3: 4: 
cache hit!
|30 38 20 28 
|30 38 20 28 20 
5: 6: 3: 4: 

5: 6: 3: 4: 
cache hit!
|30 38 20 28 20 
|30 38 20 28 20 28 
5: 6: 3: 4: 

5: 6: 3: 4: 
cache hit!
|30 38 20 28 20 28 
|30 38 20 28 20 28 30 
5: 6: 3: 4: 

5: 6: 3: 4: 
cache hit!
|30 38 20 28 20 28 30 
|30 38 20 28 20 28 30 38 
5: 6: 3: 4: 

5: 6: 3: 4: 
cache miss -> eviction!
|30 38 20 28 20 28 30 38 
|38 20 28 20 28 30 38 
evicted: 0x2205530 
|38 20 28 20 28 30 38 30 
5: 6: 7: 4: 
5: 6: 7: 4: 

5: 6: 7: 4: 
cache hit!
|38 20 28 20 28 30 38 30 
|38 20 28 20 28 30 38 30 20 
5: 6: 7: 4: 

---- show time! ----
L 10 |miss 
L 20 |miss 
L 30 |miss 
L 40 |miss 
L 50 |miss eviction 
L 60 |miss eviction 
L 51 |hit 
L 61 |hit 
L 31 |hit 
L 41 |hit 
L 70 |miss eviction 
L 51 |hit 
----    end    ----
[----] csim.c:1222: Assertion failed: actual HIT: 5 != 4 :exp
[----] csim.c:1224: Assertion failed: actual MISS: 7 != 8 :exp
[----] csim.c:1226: Assertion failed: actual EVICT: 3 != 4 :exp
[FAIL] cacheAndTraceIntergration::evictionTest1_sEb_242_LRU: (0.00s)

./csim-ref -v -s 2 -E 4 -b 2 -t traces/myEvictionTest2.trace 
L 10,1 miss 
L 20,1 miss 
L 30,1 miss 
S 40,1 miss 
M 50,1 miss eviction hit 
M 60,1 miss eviction hit 
M 70,1 miss eviction hit 
M 80,1 miss eviction hit 
hits:4 misses:8 evictions:4


 S 50,1 A
 S 60,1 B
 S 30,1 C
 S 40,1 D  warmup
 L 71,1 A
 L 51,1 B  evict
 S 52,1 B
 S 42,1 D
 S 72,1 A  hit
 L a0,1 C
 L b0,1 B
 L c0,1	D  evict
 S 40,1 A
 S 70,1 C
 S a0,1 B  evict
 L 40,1 A
 L a0,1 B
 L 70,1 C
 L c0,1 D  hit( all line confirm )

 [evictionTest3_sEb_242_LRU]
0: 0: 0: 0: 
cold cache!
|empty list!
|20 
5: 0: 0: 0: 

5: 0: 0: 0: 
cold cache!
|20 
|20 28 
5: 6: 0: 0: 

5: 6: 0: 0: 
cold cache!
|20 28 
|20 28 30 
5: 6: 3: 0: 

5: 6: 3: 0: 
cold cache!
|20 28 30 
|20 28 30 38 
5: 6: 3: 4: 

5: 6: 3: 4: 
cache miss -> eviction!
|20 28 30 38 
|28 30 38 
evicted: 0x2084520 
|28 30 38 20 
7: 6: 3: 4: 
7: 6: 3: 4: 

7: 6: 3: 4: 
cache miss -> eviction!
|28 30 38 20 
|30 38 20 
evicted: 0x2084528 
|30 38 20 28 
7: 5: 3: 4: 
7: 5: 3: 4: 

7: 5: 3: 4: 
cache hit!
|30 38 20 28 
|30 38 20 28 28 
7: 5: 3: 4: 

7: 5: 3: 4: 
cache hit!
|30 38 20 28 28 
|30 38 20 28 28 38 
7: 5: 3: 4: 

7: 5: 3: 4: 
cache hit!
|30 38 20 28 28 38 
|30 38 20 28 28 38 20 
7: 5: 3: 4: 

7: 5: 3: 4: 
cache miss -> eviction!
|30 38 20 28 28 38 20 
|38 20 28 28 38 20 
evicted: 0x2084530 
|38 20 28 28 38 20 30 
7: 5: A: 4: 
7: 5: A: 4: 

7: 5: A: 4: 
cache miss -> eviction!
|38 20 28 28 38 20 30 
|20 28 28 38 20 30 
evicted: 0x2084538 
|20 28 28 38 20 30 38 
7: 5: A: B: 
7: 5: A: B: 

7: 5: A: B: 
cache miss -> eviction!
|20 28 28 38 20 30 38 
|28 28 38 20 30 38 
evicted: 0x2084520 
|28 28 38 20 30 38 20 
C: 5: A: B: 
C: 5: A: B: 

C: 5: A: B: 
cache miss -> eviction!
|28 28 38 20 30 38 20 
|28 38 20 30 38 20 
evicted: 0x2084528 
|28 38 20 30 38 20 28 
C: 4: A: B: 
C: 4: A: B: 

C: 4: A: B: 
cache miss -> eviction!
|28 38 20 30 38 20 28 
|38 20 30 38 20 28 
evicted: 0x2084528 
|38 20 30 38 20 28 28 
C: 7: A: B: 
C: 7: A: B: 

C: 7: A: B: 
cache hit!
|38 20 30 38 20 28 28 
|38 20 30 38 20 28 28 30 
C: 7: A: B: 

C: 7: A: B: 
cache miss -> eviction!
|38 20 30 38 20 28 28 30 
|20 30 38 20 28 28 30 
evicted: 0x2084538 
|20 30 38 20 28 28 30 38 
C: 7: A: 4: 
C: 7: A: 4: 

C: 7: A: 4: 
cache hit!
|20 30 38 20 28 28 30 38 
|20 30 38 20 28 28 30 38 30 
C: 7: A: 4: 

C: 7: A: 4: 
cache hit!
|20 30 38 20 28 28 30 38 30 
|20 30 38 20 28 28 30 38 30 28 
C: 7: A: 4: 

C: 7: A: 4: 
cache hit!
|20 30 38 20 28 28 30 38 30 28 
|20 30 38 20 28 28 30 38 30 28 20 
C: 7: A: 4: 

---- show time! ----
S 50 |miss 
S 60 |miss 
S 30 |miss 
S 40 |miss 
L 71 |miss eviction 
L 51 |miss eviction 
S 52 |hit 
S 42 |hit 
S 72 |hit 
L a0 |miss eviction 
L b0 |miss eviction 
L c0 |miss eviction 
S 40 |miss eviction 
S 70 |miss eviction 
S a0 |hit 
L 40 |miss eviction 
L a0 |hit 
L 70 |hit 
L c0 |hit 


./csim-ref -v -s 2 -E 4 -b 2 -t traces/myEvictionTest3.trace 
S 50,1 miss 
S 60,1 miss 
S 30,1 miss 
S 40,1 miss 
L 71,1 miss eviction 
L 51,1 miss eviction 
S 52,1 hit 
S 42,1 hit 
S 72,1 hit 
L a0,1 miss eviction 
L b0,1 miss eviction 
L c0,1 miss eviction 
S 40,1 miss eviction 
S 70,1 miss eviction 
S a0,1 miss eviction 
L 40,1 hit 
L a0,1 hit 
L 70,1 hit 
L c0,1 hit 
hits:7 misses:12 evictions:8


 S 50,1 A
 S 60,1 B
 S 30,1 C
 S 40,1 D  warmup
 L 71,1 A
 L 51,1 B  evict
 S 52,1 B
 S 42,1 D
 S 72,1 A  hit
 L a0,1 C
 L b0,1 B
 L c0,1	D  evict
 S 40,1 A
 S 70,1 C
 S a0,1 B  evict
 L 40,1 A
 L a0,1 B
 L 70,1 C
 L c0,1 D  hit( all line confirm )


----
0|a b a b b a b a 
0|b a b b a b a 
  ^
1|b a b b a b a 
1|b a b b a b a 
    ^
2|b a b b a b a 
2|b b b a b a 
      ^
3|b b b a b a 
3|b b b a b a 
        ^
4|b b b a b a 
4|b b b a b a 

5|b b b a b a 
5|b b a 

6|b b a 
6|b b a 
----
