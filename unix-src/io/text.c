#include "../param.h"
#include "../systm.h"
#include "../user.h"
#include "../proc.h"
#include "../text.h"
#include "../inode.h"

 /* Swap out process p.
  * The ff flag causes its core to be freed--
  * it may be off when called to create an image for a
  * child process in newproc.
  * Os is the old size of the data area of the process,
  * and is supplied during core expansion swaps.
  *
  * panic: out of swap space
  * panic: swap error -- IO error
  */
 xswap(p, ff, os)
 int *p;
 {       register *rp, a;

         rp = p;
         if(os == 0)
                 os = rp-&gt;p_size;
         a = malloc(swapmap, (rp-&gt;p_size+7)/8);
         if(a == NULL)
                 panic("out of swap space");
         xccdec(rp-&gt;p_textp);
         rp-&gt;p_flag =| SLOCK;
         if(swap(a, rp-&gt;p_addr, os, 0))
                 panic("swap error");
         if(ff)
                 mfree(coremap, os, rp-&gt;p_addr);
         rp-&gt;p_addr = a;
         rp-&gt;p_flag =&amp; ~(SLOAD|SLOCK);
         rp-&gt;p_time = 0;
         if(runout) {
                 runout = 0;
                 wakeup(&amp;runout);
         }
 }
 * ---------------------------       */

 /*
  * relinquish use of the shared text segment
  * of a process.
  */
 xfree()
 {
         register *xp, *ip;

         if((xp=u.u_procp-&gt;p_textp) != NULL) {
                 u.u_procp-&gt;p_textp = NULL;
                 xccdec(xp);
                 if(--xp-&gt;x_count == 0) {
                         ip = xp-&gt;x_iptr;
                         if((ip-&gt;i_mode&amp;ISVTX) == 0) {
                                 xp-&gt;x_iptr = NULL;
                                 mfree(swapmap, (xp-&gt;x_size+7)/8, xp-&gt;x_daddr);
                                 ip-&gt;i_flag =&amp; ~ITEXT;
                                 iput(ip);
                         }
                 }
         }
 }
 /* ---------------------------       */

 /* Attach to a shared text segment.
  * If there is no shared text, just return.
  * If there is, hook up to it:
  * if it is not currently being used, it has to be read
  * in from the inode (ip) and established in the swap space.
  * If it is being used, but is not currently in core,
  * a swap has to be done to get it back.
  * The full coroutine glory has to be invoked--
  * see slp.c-- because if the calling process
  * is misplaced in core the text image might not fit.
  * Quite possibly the code after "out:" could check to
  * see if the text does fit and simply swap it in.
  *
  * panic: out of swap space
  */
 xalloc(ip)
 int *ip;
 {
         register struct text *xp;
         register *rp, ts;

         if(u.u_arg[1] == 0) return;
         rp = NULL;
         for(xp = &amp;text[0]; xp &lt; &amp;text[NTEXT]; xp++)
                 if(xp-&gt;x_iptr == NULL) {
                         if(rp == NULL)
                                 rp = xp;
                 } else
                         if(xp-&gt;x_iptr == ip) {
                                 xp-&gt;x_count++;
                                 u.u_procp-&gt;p_textp = xp;
                                 goto out;
                         }
         if((xp=rp) == NULL) panic("out of text");
         xp-&gt;x_count = 1;
         xp-&gt;x_ccount = 0;
         xp-&gt;x_iptr = ip;
         ts = ((u.u_arg[1]+63)&gt;&gt;6) &amp; 01777;
         xp-&gt;x_size = ts;
         if((xp-&gt;x_daddr = malloc(swapmap, (ts+7)/8)) == NULL)
                 panic("out of swap space");
         expand(USIZE+ts);
         estabur(0, ts, 0, 0);
         u.u_count = u.u_arg[1];
         u.u_offset[1] = 020;
         u.u_base = 0;
         readi(ip);
         rp = u.u_procp;
         rp-&gt;p_flag =| SLOCK;
         swap(xp-&gt;x_daddr, rp-&gt;p_addr+USIZE, ts, 0);
         rp-&gt;p_flag =&amp; ~SLOCK;
         rp-&gt;p_textp = xp;
         rp = ip;
         rp-&gt;i_flag =| ITEXT;
         rp-&gt;i_count++;
         expand(USIZE);
 out:
         if(xp-&gt;x_ccount == 0) {
                 savu(u.u_rsav);
                 savu(u.u_ssav);
                 xswap(u.u_procp, 1, 0);
                 u.u_procp-&gt;p_flag =| SSWAP;
                 swtch();
                 /* no return */
         }
         xp-&gt;x_ccount++;
 }
 /* ---------------------------       */

 /* Decrement the in-core usage count of a shared text segment.
  * When it drops to zero, free the core space.
  */
 xccdec(xp)
 int *xp;
 {
         register *rp;

         if((rp=xp)!=NULL &amp;&amp; rp-&gt;x_ccount!=0)
                 if(--rp-&gt;x_ccount == 0)
                         mfree(coremap, rp-&gt;x_size, rp-&gt;x_caddr);
 }




