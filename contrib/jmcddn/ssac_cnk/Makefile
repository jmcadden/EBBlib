classes = SSACSimpleSharedArray.C CacheSimple.C Test.C 
hdrs = $(patsubst %.c,%.h,$(classes))
src = eosSSACTest.C $(classes)
objs = $(patsubst %.C,%.o,$(src))
libs = -lpthread 
CXXFLAGS=-D_BG_ -g -O0 -Wall -I/bgsys/drivers/ppcfloor/arch/include
CXX=mpicxx
#CXX=g++

all: eosSSACTest

-include $(objs:.o=.d)

# compile and generate dependency info
%.o: %.C
	${CXX} -c $(CXXFLAGS) $*.C -o $*.o 
	${CXX} -MM $(CXXFLAGS) $*.C > $*.d 

eosSSACTest.o: Test.H 

eosSSACTest: $(objs) $(hdrs)
	${CXX} ${CXXFLAGS} $(objs) $(libs) -o eosSSACTest 

clean:
	rm -f *.o *.d eosSSACTest

