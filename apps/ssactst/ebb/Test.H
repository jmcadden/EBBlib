#ifndef __TEST_H__
#define __TEST_H__

#define OUTPUT_CSV

class Test;
typedef Test **TestId;

/* This Test class will initilize a test session run 'n' iterations of calls to the init(), work() and end() functions. Optionally, cores will barrier before running the test concurrently (NOT YET IMPLEMENTED).*/
class Test : public CPlusEBB { 
  protected:
    int iterations; 
    static struct barrier_s bar;
    static volatile int bar_init;
    int bar_sense;
    //TODO: Barrier YES/NO?
    virtual EBBRC setup(void);  /* setup test session (for new test objects) */
    // virtual EBBRC setup_vars(int);
    virtual EBBRC init(void);   /* run at the start of each test iteration */
    virtual EBBRC work(int id); /* actual test operation */
    virtual EBBRC end(void);
  public:
    static EBBRC create(TestId &testrep);
    void * operator new(size_t size);
    void operator delete(void * p, size_t size);
    virtual EBBRC run(void); 
};

#endif
