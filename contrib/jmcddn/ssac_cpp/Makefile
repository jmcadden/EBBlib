classes = SSACSimpleSharedArray.C CacheSimple.C Test.C 
hdrs = $(patsubst %.c,%.h,$(classes))
src = eosSSACTest.C $(classes)
objs = $(patsubst %.C,%.o,$(src))
libs = -lpthread 
CXXFLAGS=-g -O0 -Wall
CXX=g++
CNK_CXXFLAGS=-D_BG_ -g -O0 -Wall -I/bgsys/drivers/ppcfloor/arch/include
CNK_CXX=mpicxx

all: eosSSACTest

-include $(objs:.o=.d)

# compile and generate dependency info
%.o: %.C
	${CXX} -c $(CXXFLAGS) $*.C -o $*.o 
	${CXX} -MM $(CXXFLAGS) $*.C > $*.d 

eosSSACTest.o: Test.H 

eosSSACTest: $(objs) $(hdrs)
	${CXX} ${CXXFLAGS} $(objs) $(libs) -o eosSSACTest 

cnk: $(objs) $(hdrs)
	${CNK_CXX} ${CNK_CXXFLAGS} $(objs) $(libs) -o eosSSACTest_cnk

clean:
	rm -f *.o *.d eosSSACTest eosSSACTest_cnk




