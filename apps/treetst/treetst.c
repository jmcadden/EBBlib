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

#include <config.h>
#include <stdint.h>

#include <l0/lrt/types.h>
#include <l0/cobj/cobj.h>
#include <lrt/io.h>
#include <lrt/exit.h>
#include <lrt/string.h>
#include <l0/lrt/trans.h>
#include <lrt/assert.h>
#include <l0/cobj/CObjEBB.h>
#include <l0/EBBMgrPrim.h>
#include <l0/cobj/CObjEBBUtils.h>
#include <l0/cobj/CObjEBBRoot.h>
#include <l0/cobj/CObjEBBRootShared.h>
#include <l0/cobj/CObjEBBRootMulti.h>
#include <l0/cobj/CObjEBBRootMultiImp.h>
#include <l0/EventMgrPrim.h>
#include <l0/EventMgrPrimImp.h>
#include <l0/MemMgr.h>
#include <l0/MemMgrPrim.h>
#include <l1/App.h>
#include <l1/startinfo.h>

#include <l0/lrt/bare/arch/ppc32/bg_tree.h>
#include <l0/lrt/bare/arch/ppc32/link.h>
#include <l0/lrt/bare/arch/ppc32/debug.h>
#include <l0/lrt/event_irq_def.h>



/* TreeTst App Object */

CObject(TreeTst) {
  CObjInterface(App) *ft;
};

EBBRC
Treetst_start(AppRef _self){
  lrt_printf("Run treetest\n");
  /*needed values
   * tree
   * channel
   * dest
   * dest.p2p.pclass
   * count
   * send_id
   * link_protocol
   * tree_route
   *

  // let's do a write 
  struct bglink_hdr_tree lnkhdr __attribute__((aligned(16)));
  union bgtree_header dest = { raw : 0 }
  int rc;

  lnkhdr.dst_key = send_id;
  lnkhdr.lnk_proto = link_protocol;

  dest = tty_route.dest;
  dest.p2p.pclass = tree_route;

  //link header init
  lnkhdr.src_key = tree->nodeid;
  lnkhdr.conn_id = tree->curr_conn++;
  lnkhdr.total_pkt = 1;  // ((len - 1) / TREE_FRAGPAYLOAD) + 1;
  lnkhdr.this_pkt = 0;
  lnkhdr.opt.opt_con.len = count;
  // transmit
   * */
  
  return EBBRC_OK;
}

CObjInterface(App) TreeTst_ftable = {
  .start = Treetst_start
};

APP_START_ONE(TreeTst);





