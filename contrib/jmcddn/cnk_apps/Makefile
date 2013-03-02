PROJ=OS_Exascale
CXX=mpicxx
CPP=mpicc
CXXFLAGS=-D_CNK_ -O0  -I. -I/bgsys/drivers/ppcfloor/arch/include
###
DEPS= sg.c

###
HDRS = $(patsubst %.c,%.h,$(DEPS))
OBJS = $(patsubst %.c,%.o,$(DEPS))
LIBS = -lpthread -lm

all: sg

%.o: %.c
	${CPP} -c $(CXXFLAGS) $*.c -o $*.o 
	${CPP} -MM $(CXXFLAGS) $*.c > $*.d 

mini2: mini2.c 
	${CPP} mini2.c ${CXXFLAGS} $(OBJS) $(LIBS) -o mini2

sg: ${OBJS} ${HDRS} 
	${CPP} sg-app.c ${CXXFLAGS} $(OBJS) $(LIBS) -o sg-app


# Test Runs ##############################################

run: 
	make clean
	make sg
	qsub -q default -A ${PROJ} -n 16 -t 5 sg-app
	qstat

runfull: 
	make clean
	make sg
	qsub -q default -A ${PROJ} -n 512 -t 15 sg-app
	qstat

clean:
	rm -f *.o *.d *.cobaltlog *.error *.output mini2 sg-app .*.*.sw*




