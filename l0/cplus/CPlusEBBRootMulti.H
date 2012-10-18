#ifndef __CPLUS_EBB_ROOT_MULTI_H__
#define __CPLUS_EBB_ROOT_MULTI_H__
/*
 * Copyright (C) 2011 by Project SESA, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <l0/cplus/CPlusEBBRoot.h>

/*
CObject(CObjEBBRootMulti) {
  CObjInterface(CObjEBBRootMulti) *ft;
};

CObjInterface(CObjEBBRootMulti)
{
  CObjImplements(CObjEBBRoot);
  void (*addRepOn)(CObjEBBRootMultiRef _self, uintptr_t el, EBBRep *rep);
  RepListNode *(*nextRep) (CObjEBBRootMultiRef _self,
                           RepListNode *curr, EBBRep **rep);
  void (*setKey)(CObjEBBRootMultiRef _self, uintptr_t key);
  uintptr_t (*getKey)(CObjEBBRootMultiRef _self);
};
*/

typedef EBBRep *(*CreateRepFunc) (CObjEBBRootMultiRef _self);
typedef struct RepListNode_s RepListNode;

class CPlusEBBRootMulti : public CPlusEBBRoot {
  CPlusEBB *theRep;
private:
  virtual EBBRC handleMiss(CPlusEBB **obj, EBBLTrans *lt, EBBFuncNum fnum);
  virtual EBBRC addRepOn(CPlusEBB **obj, uintptr_t el, EBBRep *rep);
  virtual EBBRC nextRep(CCPlusEBB **obj, uintptr_t el, EBBRep *rep);
  virtual EBBRC setKey(CCPlusEBB **obj, uintptr_t key);
  virtual EBBRC getKey(CCPlusEBB **obj);
  CreateRepFunc createRep;
  RepListNode *head;
  uintptr_t lock;
  uintptr_t key;
public:
  void *operator new(size_t size);
  void operator delete(void * p, size_t size);
  // ??? 
  virtual EBBRC init(CPlusEBB *rep) { theRep = rep; return EBBRC_OK;}
  static EBBRC staticInit(CObjEBBRootMultiImpRef o, CreateRepFunc func);
  static EBBRC create(CObjEBBRootMultiImpRef o, CreateRepFunc func);
  virtual ~CPlusEBBRootMulti();
};
#endif

/*
extern void CObjEBBRootMultiImpDestroy(CObjEBBRootMultiImpRef o,
  DestroyRepFunc func);
  */
