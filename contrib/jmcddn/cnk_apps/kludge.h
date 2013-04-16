#ifndef _KLUDGE_H_
#define _KLUDE_H_

#define _bgp_mfspr( SPRN )\
({\
   unsigned int tmp;\
   do {\
      asm volatile ("mfspr %0,%1" : "=&r" (tmp) : "i" (SPRN) : "memory" );\
      }\
      while(0);\
   tmp;\
})\

inline uint64_t _bgp_GetTimeBase( void )
{
   union {
         uint32_t ul[2];
         uint64_t ull;
         }
         hack;
   uint32_t utmp;

   do {
      utmp       = _bgp_mfspr( SPRN_TBRU );
      hack.ul[1] = _bgp_mfspr( SPRN_TBRL );
      hack.ul[0] = _bgp_mfspr( SPRN_TBRU );
      }
      while( utmp != hack.ul[0] );

   return( hack.ull );
}
#endif
