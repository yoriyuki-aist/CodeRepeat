CC=g++
CFLAGS=-O9 
#CFLAGS=-msse4.2
#FLAGS=

all: findmaxrep 

test: test_wt test_bwt_2_lcp test_bv test_bt test_maxrep test_report test_huffcode

#the whole program: given bwt, output repeats 
findmaxrep: main.o arg.o report.o maxrep.o bt.o bv.o bwt_2_lcp.o uti.o huffcode.o
	$(CC) $(CFLAGS) -o findmaxrep main.o arg.o report.o maxrep.o bt.o bv.o bwt_2_lcp.o uti.o huffcode.o

main.o: main.cpp main.h arg.h bt.h bv.h maxrep.h report.h wt_node.h wt.h uti.h bwt_2_lcp.h uti.h huffcode.h
	$(CC) $(CFLAGS) -c main.cpp

arg.o: arg.cpp arg.h
	$(CC) $(CFLAGS) -c arg.cpp



#huffman wavelet tree
test_huffcode: test_huffcode.o huffcode.o uti.o
	${CC} $(CFLAGS) -o test_huffcode test_huffcode.o huffcode.o uti.o

test_huffcode.o: test_huffcode.cpp huffcode.h uti.h
	${CC} $(CFLAGS) -c test_huffcode.cpp

huffcode.o: huffcode.cpp huffcode.h 
	${CC} $(CFLAGS) -c huffcode.cpp

#repeat reporting
test_report: test_report.o report.o bv.o uti.o bwt_2_lcp.o huffcode.o arg.o
	${CC} $(CFLAGS) -o test_report test_report.o report.o bwt_2_lcp.o bv.o uti.o huffcode.o arg.o

test_report.o: test_report.cpp report.h uti.h bv.h wt.h wt_node.h huffcode.h
	${CC} $(CFLAGS) -c test_report.cpp

report.o: report.cpp report.h bt.h bv.h wt.h wt_node.h bwt_2_lcp.h huffcode.h arg.h
	${CC} $(CFLAGS) -c report.cpp 

#repeat finding
test_maxrep: test_maxrep.o maxrep.o bt.o bv.o uti.o huffcode.o arg.o
	$(CC) $(CFLAGS) -o test_maxrep test_maxrep.o maxrep.o bt.o bv.o uti.o huffcode.o arg.o

test_maxrep.o: test_maxrep.cpp maxrep.h huffcode.h
	$(CC) $(CFLAGS) -c test_maxrep.cpp

maxrep.o: maxrep.cpp maxrep.h bt.h bv.h wt.h wt_node.h uti.h bwt_2_lcp.h huffcode.h arg.h 
	$(CC) $(CFLAGS) -c maxrep.cpp

#binary bit tree
test_bt: test_bt.o bt.o uti.o bv.o
	$(CC) $(CFLAGS) -o test_bt test_bt.o bt.o uti.o bv.o

test_bt.o: test_bt.cpp bt.h bv.h
	$(CC) $(CFLAGS) -c test_bt.cpp

bt.o: bt.cpp bt.h uti.h
	$(CC) $(CFLAGS) -c bt.cpp 



#bit vector
test_bv: test_bv.o bv.o uti.o
	$(CC) $(CFLAGS) -o test_bv test_bv.o bv.o uti.o

test_bv.o: test_bv.cpp bv.h
	$(CC) $(CFLAGS) -c test_bv.cpp 

bv.o: bv.cpp bv.h uti.h
	$(CC) $(CFLAGS) -c bv.cpp 




#bwt to lcp 

test_bwt_2_lcp: test_bwt_2_lcp.o bwt_2_lcp.o uti.o huffcode.o
	$(CC) $(CFLAGS) -o test_bwt_2_lcp test_bwt_2_lcp.o bwt_2_lcp.o uti.o huffcode.o

test_bwt_2_lcp.o: test_bwt_2_lcp.cpp wt.h wt_node.h uti.h bwt_2_lcp.h huffcode.h
	$(CC) $(CFLAGS) -c test_bwt_2_lcp.cpp

bwt_2_lcp.o: bwt_2_lcp.cpp wt.h wt_node.h uti.h bwt_2_lcp.h huffcode.h
	$(CC) $(CFLAGS) -c bwt_2_lcp.cpp



#wavelet tree

test_wt: test_wt.o uti.o huffcode.o
	$(CC) $(CFLAGS) -o test_wt test_wt.o uti.o huffcode.o

test_wt.o: test_wt.cpp wt.h wt_node.h uti.h huffcode.h
	$(CC) $(CFLAGS) -c test_wt.cpp

uti.o: uti.cpp uti.h
	$(CC) $(CFLAGS) -c uti.cpp


#clean 

clean:
	-rm *~ *.o core
