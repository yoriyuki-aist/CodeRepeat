
CXX      = g++
CXXFLAGS = -O3 -W -Wall -Wdisabled-optimization 
# -DDEBUG=0

PROGS  = bwt ibwt dnabwt bwtsa


.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCL) -c $<

all: $(PROGS)

ADD_OBJS5 = bwt.o
ALL_OBJS5 = $(ALL_OBJS4) $(ADD_OBJS5)
OBJS5 = $(ADD_OBJS5) difference_cover.o
bwt: $(OBJS5)
	$(CXX) $(CXXFLAGS) $(INCL) -o $@ $(OBJS5)

ADD_OBJS6 = ibwt.o
ALL_OBJS6 = $(ALL_OBJS5) $(ADD_OBJS6)
OBJS6 = $(ADD_OBJS6)
ibwt: $(OBJS6)
	$(CXX) $(CXXFLAGS) $(INCL) -o $@ $(OBJS6)

ADD_OBJS7 = dnabwt.o 
ALL_OBJS7 = $(ALL_OBJS6) $(ADD_OBJS7)
OBJS7 = $(ADD_OBJS7) difference_cover.o
dnabwt: $(OBJS7)
	$(CXX) $(CXXFLAGS) $(INCL) -o $@ $(OBJS7)

ADD_OBJS8 = bwtsa.o
ALL_OBJS8 = $(ALL_OBJS7) $(ADD_OBJS8)
OBJS8 = $(ADD_OBJS8) difference_cover.o
bwtsa: $(OBJS8)
	$(CXX) $(CXXFLAGS) $(INCL) -o $@ $(OBJS8)

#--------------------------------------------------------------------------
# cleaning up
#--------------------------------------------------------------------------
clean: depend
	rm -f *.o core


#--------------------------------------------------------------------------
# dependencies
#--------------------------------------------------------------------------

ALL_OBJS = $(ALL_OBJS8)
SOURCES = $(ALL_OBJS:.o=.cpp)

depend:
	$(CXX) -MM $(SOURCES) > dependencies.mk

include dependencies.mk
