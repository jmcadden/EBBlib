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

mini: ${OBJS} ${HDRS} 
	${CPP} mini.c ${CXXFLAGS} $(OBJS) $(LIBS) -o mini

sg: ${OBJS} ${HDRS} 
	${CPP} sg-app.c ${CXXFLAGS} $(OBJS) $(LIBS) -o sg-app

dmem: ${OBJS} ${HDRS} 
	${CPP} dmem.c ${CXXFLAGS} $(OBJS) $(LIBS) -o dmem

# Test Runs ##############################################

run: 
	make clean
	make sg
	qsub -q default -A ${PROJ} -n 16 -t 5 sg-app
	qstat

runmini: 
	make clean
	make mini
	qsub -q default -A ${PROJ} -n 512 -t 15 mini
	qstat

runfull: 
	make clean
	make sg
	qsub -q default -A ${PROJ} -n 512 -t 15 sg-app
	qstat

rundmem:
	make clean
	make dmem
	qsub -q default -A ${PROJ} -n 16 -t 10 dmem
	qstat

clean:
	rm -f *.o *.d *.cobaltlog *.error *.output dmem  mini sg-app .*.*.sw*




