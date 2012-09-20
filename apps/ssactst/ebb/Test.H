#ifndef __TEST_H__
#define __TEST_H__

#define OUTPUT_CSV

class Test;
typedef Test **TestId;

/* This simple test class will run work a specified number of times across a
 * specficed number of event locations. Cores will barrier before running the
 * test concurrently.*/
class Test : public CPlusEBB { 
  protected:
    int iterations; 
    int num_locations; 
    static struct barrier_s bar;
    static volatile int bar_init;
    int bar_sense;
    struct WArgs {
      int id;
      unsigned long long start;
      unsigned long long end;
      Test *theTest;
    } *wargs;
    virtual EBBRC setup_test(void)=0;
    virtual EBBRC setup_iteration(void)=0;
    virtual EBBRC work(int id)=0;
    virtual EBBRC end(void)=0;
  public:
    static EBBRC Create(TestId &testrep);
    EBBRC run_test(void); //must be implemented
    void * operator new(size_t size);
    void operator delete(void * p, size_t size);
};

#endif
