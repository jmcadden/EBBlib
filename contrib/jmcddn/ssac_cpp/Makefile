classes = SSACSimpleSharedArray.C CacheSimple.C Test.C 
hdrs = $(patsubst %.c,%.h,$(classes))
src = eosSSACTest.C $(classes)
std_objs = $(patsubst %.C,%_std.o,$(src))
cnk_objs = $(patsubst %.C,%_cnk.o,$(src))
libs = -lpthread -lm
CXXFLAGS=-g -O0 -Wall
CXX=g++
CNK_CXXFLAGS=-D_BG_ -I. -I/bgsys/drivers/ppcfloor/arch/include
#CNK_CXX=mpixlcxx_r
CNK_CXX=mpicxx-4.3.2

all: cnk

%_std.o: %.C
	${CXX} -c $(CXXFLAGS) $*.C -o $*_std.o 

%_cnk.o: %.C
	${CNK_CXX} -c $(CNK_CXXFLAGS) $*.C -o $*_cnk.o 

std: $(std_objs) $(hdrs)
	${CXX} ${CXXFLAGS} $(std_objs) $(libs) -o eosSSACTest 

cnk: $(cnk_objs) $(hdrs)
	${CNK_CXX} ${CNK_CXXFLAGS} $(cnk_objs) $(libs) -o eosSSACTest_cnk

runbg: 
	qsub -q default -A I12_faultOblivious -n 10 -t 15 eosSSACTest_cnk

clean:
	rm -f *.o *.d eosSSACTest eosSSACTest_cnk *.cobaltlog *.error *.output




