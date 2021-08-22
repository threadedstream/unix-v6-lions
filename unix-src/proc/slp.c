 /*
  */
 
 #include "../param.h"
 #include "../user.h"
 #include "../proc.h"
 #include "../text.h"
 #include "../systm.h"
 #include "../file.h"
 #include "../inode.h"
 #include "../buf.h"
 /* ---------------------------       */
 /*
  * Create a new process-- the internal version of
  * sys fork.
  * It returns 1 in the new process.
  * How this happens is rather hard to understand.
  * The essential fact is that the new process is created
  * in such a way that appears to have started executing
  * in the same call to newproc as the parent;
  * but in fact the code that runs is that of swtch.
  * The subtle implication of the returned value of swtch
  * (see above) is that this is the value that newproc's
  * caller in the new process sees.
  */
 newproc()
 {
         int a1, a2;
         struct proc *p, *up;
         register struct proc *rpp;
         register *rip, n;
 
         p = NULL;
         /*
          * First, just locate a slot for a process
          * and copy the useful info from this process into it.
          * The panic "cannot happen" because fork has already
          * checked for the existence of a slot.
          */
 retry:
         mpid++;
         if(mpid &lt; 0) {
                 mpid = 0;
                 goto retry;
         }
         for(rpp = &amp;proc[0]; rpp &lt; &amp;proc[NPROC]; rpp++) {
                 if(rpp-&gt;p_stat == NULL &amp;&amp; p==NULL)
                         p = rpp;
                 if (rpp-&gt;p_pid==mpid)
                         goto retry;
         }
         if ((rpp = p)==NULL)
                 panic("no procs");
 
         /*
          * make proc entry for new proc
          */
 
         rip = u.u_procp;
         up = rip;
         rpp-&gt;p_stat = SRUN;
         rpp-&gt;p_flag = SLOAD;
         rpp-&gt;p_uid = rip-&gt;p_uid;
         rpp-&gt;p_ttyp = rip-&gt;p_ttyp;
         rpp-&gt;p_nice = rip-&gt;p_nice;
         rpp-&gt;p_textp = rip-&gt;p_textp;
         rpp-&gt;p_pid = mpid;
         rpp-&gt;p_ppid = rip-&gt;p_pid;
         rpp-&gt;p_time = 0;
 
         /*
          * make duplicate entries
          * where needed
          */
 
         for(rip = &amp;u.u_ofile[0]; rip &lt; &amp;u.u_ofile[NOFILE];)
                 if((rpp = *rip++) != NULL)
                         rpp-&gt;f_count++;
         if((rpp=up-&gt;p_textp) != NULL) {
                 rpp-&gt;x_count++;
                 rpp-&gt;x_ccount++;
         }
         u.u_cdir-&gt;i_count++;
         /*
          * Partially simulate the environment
          * of the new process so that when it is actually
          * created (by copying) it will look right.
          */
         savu(u.u_rsav);
         rpp = p;
         u.u_procp = rpp;
         rip = up;
         n = rip-&gt;p_size;
         a1 = rip-&gt;p_addr;
         rpp-&gt;p_size = n;
         a2 = malloc(coremap, n);
         /*
          * If there is not enough core for the
          * new process, swap out the current process to generate the
          * copy.
          */
         if(a2 == NULL) {
                 rip-&gt;p_stat = SIDL;
                 rpp-&gt;p_addr = a1;
                 savu(u.u_ssav);
                 xswap(rpp, 0, 0);
                 rpp-&gt;p_flag =| SSWAP;
                 rip-&gt;p_stat = SRUN;
         } else {
         /*
          * There is core, so just copy.
          */
                 rpp-&gt;p_addr = a2;
                 while(n--)
                         copyseg(a1++, a2++);
         }
         u.u_procp = rip;
         return(0);
 }
 /* ---------------------------       */
 
 /*
  * The main loop of the scheduling (swapping)
  * process.
  * The basic idea is:
  *  see if anyone wants to be swapped in;
  *  swap out processes until there is room;
  *  swap him in;
  *  repeat.
  * Although it is not remarkably evident, the basic
  * synchronization here is on the runin flag, which is
  * slept on and is set once per second by the clock routine.
  * Core shuffling therefore takes place once per second.
  *
  * panic: swap error -- IO error while swapping.
  *      this is the one panic that should be
  *      handled in a less drastic way. Its
  *      very hard.
  */
 sched()
 {
         struct proc *p1;
         register struct proc *rp;
         register a, n;
 
         /*
          * find user to swap in
          * of users ready, select one out longest
          */
 
         goto loop;
 
 sloop:
         runin++;
         sleep(&amp;runin, PSWP);
 
 loop:
         spl6();
         n = -1;
         for(rp = &amp;proc[0]; rp &lt; &amp;proc[NPROC]; rp++)
         if(rp-&gt;p_stat==SRUN &amp;&amp; (rp-&gt;p_flag&amp;SLOAD)==0 &amp;&amp;
             rp-&gt;p_time &gt; n) {
                 p1 = rp;
                 n = rp-&gt;p_time;
         }
         if(n == -1) {
                 runout++;
                 sleep(&amp;runout, PSWP);
                 goto loop;
         }
 
         /*
          * see if there is core for that process
          */
 
         spl0();
         rp = p1;
         a = rp-&gt;p_size;
         if((rp=rp-&gt;p_textp) != NULL)
                 if(rp-&gt;x_ccount == 0)
                         a =+ rp-&gt;x_size;
         if((a=malloc(coremap, a)) != NULL)
                 goto found2;
 
         /*
          * none found,
          * look around for easy core
          */
 
         spl6();
         for(rp = &amp;proc[0]; rp &lt; &amp;proc[NPROC]; rp++)
         if((rp-&gt;p_flag&amp;(SSYS|SLOCK|SLOAD))==SLOAD &amp;&amp;
             (rp-&gt;p_stat == SWAIT || rp-&gt;p_stat==SSTOP))
                 goto found1;
 
         /*
          * no easy core,
          * if this process is deserving,
          * look around for
          * oldest process in core
          */
 
         if(n &lt; 3)
                 goto sloop;
         n = -1;
         for(rp = &amp;proc[0]; rp &lt; &amp;proc[NPROC]; rp++)
         if((rp-&gt;p_flag&amp;(SSYS|SLOCK|SLOAD))==SLOAD &amp;&amp;
            (rp-&gt;p_stat==SRUN || rp-&gt;p_stat==SSLEEP) &amp;&amp;
             rp-&gt;p_time &gt; n) {
                 p1 = rp;
                 n = rp-&gt;p_time;
         }
         if(n &lt; 2)
                 goto sloop;
         rp = p1;
 
         /*
          * swap user out
          */
 
 found1:
         spl0();
         rp-&gt;p_flag =&amp; ~SLOAD;
         xswap(rp, 1, 0);
         goto loop;
 
         /*
          * swap user in
          */
 
 found2:
         if((rp=p1-&gt;p_textp) != NULL) {
                 if(rp-&gt;x_ccount == 0) {
                         if(swap(rp-&gt;x_daddr, a, rp-&gt;x_size, B_READ))
                                 goto swaper;
                         rp-&gt;x_caddr = a;
                         a =+ rp-&gt;x_size;
                 }
                 rp-&gt;x_ccount++;
         }
         rp = p1;
         if(swap(rp-&gt;p_addr, a, rp-&gt;p_size, B_READ))
                 goto swaper;
         mfree(swapmap, (rp-&gt;p_size+7)/8, rp-&gt;p_addr);
         rp-&gt;p_addr = a;
         rp-&gt;p_flag =| SLOAD;
         rp-&gt;p_time = 0;
         goto loop;
 
 swaper:
         panic("swap error");
 }
 /* ---------------------------       */
 
 /*
  * Give up the processor till a wakeup occurs
  * on chan, at which time the process
  * enters the scheduling queue at priority pri.
  * The most important effect of pri is that when
  * pri&lt;0 a signal cannot disturb the sleep;
  * if pri&gt;=0 signals will be processed.
  * Callers of this routine must be prepared for
  * premature return, and check that the reason for
  * sleeping has gone away.
  */
 sleep(chan, pri)
 {
         register *rp, s;
 
         s = PS-&gt;integ;
         rp = u.u_procp;
         if(pri &gt;= 0) {
                 if(issig())
                         goto psig;
                 spl6();
                 rp-&gt;p_wchan = chan;
                 rp-&gt;p_stat = SWAIT;
                 rp-&gt;p_pri = pri;
                 spl0();
                 if(runin != 0) {
                         runin = 0;
                         wakeup(&amp;runin);
                 }
                 swtch();
                 if(issig())
                         goto psig;
         } else {
                 spl6();
                 rp-&gt;p_wchan = chan;
                 rp-&gt;p_stat = SSLEEP;
                 rp-&gt;p_pri = pri;
                 spl0();
                 swtch();
         }
         PS-&gt;integ = s;
         return;
 
         /*
          * If priority was low (&gt;=0) and
          * there has been a signal,
          * execute non-local goto to
          * the qsav location.
          * (see trap1/trap.c)
          */
 psig:
         aretu(u.u_qsav);
 }
 /* ---------------------------       */
 
 /*
  * Wake up all processes sleeping on chan.
  */
 wakeup(chan)
 {
         register struct proc *p;
         register c, i;
 
         c = chan;
         p = &amp;proc[0];
         i = NPROC;
         do {
                 if(p-&gt;p_wchan == c) {
                         setrun(p);
                 }
                 p++;
         } while(--i);
 }
 /* ---------------------------       */
 
 /*
  * Set the process running;
  * arrange for it to be swapped in if necessary.
  */
 setrun(p)
 {
         register struct proc *rp;
 
         rp = p;
         rp-&gt;p_wchan = 0;
         rp-&gt;p_stat = SRUN;
         if(rp-&gt;p_pri &lt; curpri)
                 runrun++;
         if(runout != 0 &amp;&amp; (rp-&gt;p_flag&amp;SLOAD) == 0) {
                 runout = 0;
                 wakeup(&amp;runout);
         }
 }
 /* ---------------------------       */
 
 /*
  * Set user priority.
  * The rescheduling flag (runrun)
  * is set if the priority is higher
  * than the currently running process.
  */
 setpri(up)
 {
         register *pp, p;
 
         pp = up;
         p = (pp-&gt;p_cpu &amp; 0377)/16;
         p =+ PUSER + pp-&gt;p_nice;
         if(p &gt; 127)
                 p = 127;
         if(p &gt; curpri)
                 runrun++;
         pp-&gt;p_pri = p;
 }
 /* ---------------------------       */
 
 
 /*
  * This routine is called to reschedule the CPU.
  * if the calling process is not in RUN state,
  * arrangements for it to restart must have
  * been made elsewhere, usually by calling via sleep.
  */
 swtch()
 {
         static struct proc *p;
         register i, n;
         register struct proc *rp;
 
         if(p == NULL)
                 p = &amp;proc[0];
         /*
          * Remember stack of caller
          */
         savu(u.u_rsav);
         /*
          * Switch to scheduler's stack
          */
         retu(proc[0].p_addr);
 
 loop:
         runrun = 0;
         rp = p;
         p = NULL;
         n = 128;
         /*
          * Search for highest-priority runnable process
          */
         i = NPROC;
         do {
                 rp++;
                 if(rp &gt;= &amp;proc[NPROC])
                         rp = &amp;proc[0];
                 if(rp-&gt;p_stat==SRUN &amp;&amp; (rp-&gt;p_flag&amp;SLOAD)!=0) {
                         if(rp-&gt;p_pri &lt; n) {
                                 p = rp;
                                 n = rp-&gt;p_pri;
                         }
                 }
         } while(--i);
         /*
          * If no process is runnable, idle.
          */
         if(p == NULL) {
                 p = rp;
                 idle();
                 goto loop;
         }
         rp = p;
         curpri = n;
         /* Switch to stack of the new process and set up
          * his segmentation registers.
          */
         retu(rp-&gt;p_addr);
         sureg();
         /*
          * If the new process paused because it was
          * swapped out, set the stack level to the last call
          * to savu(u_ssav).  This means that the return
          * which is executed immediately after the call to aretu
          * actually returns from the last routine which did
          * the savu.
          *
          * You are not expected to understand this.
          */
         if(rp-&gt;p_flag&amp;SSWAP) {
                 rp-&gt;p_flag =&amp; ~SSWAP;
                 aretu(u.u_ssav);
         }
         /* The value returned here has many subtle implications.
          * See the newproc comments.
          */
         return(1);
 }
 /* ---------------------------       */
 
 /*
  * Change the size of the data+stack regions of the process.
  * If the size is shrinking, it's easy-- just release the extra core.
  * If it's growing, and there is core, just allocate it
  * and copy the image, taking care to reset registers to account
  * for the fact that the system's stack has moved.
  *
  * If there is no core, arrange for the process to be swapped
  * out after adjusting the size requirement-- when it comes
  * in, enough core will be allocated.
  * Because of the ssave and SSWAP flags, control will
  * resume after the swap in swtch, which executes the return
  * from this stack level.
  *
  * After the expansion, the caller will take care of copying
  * the user's stack towards or away from the data area.
  */
 expand(newsize)
 {
         int i, n;
         register *p, a1, a2;
 
         p = u.u_procp;
         n = p-&gt;p_size;
         p-&gt;p_size = newsize;
         a1 = p-&gt;p_addr;
         if(n &gt;= newsize) {
                 mfree(coremap, n-newsize, a1+newsize);
                 return;
         }
         savu(u.u_rsav);
         a2 = malloc(coremap, newsize);
         if(a2 == NULL) {
                 savu(u.u_ssav);
                 xswap(p, 1, n);
                 p-&gt;p_flag =| SSWAP;
                 swtch();
                 /* no return */
         }
         p-&gt;p_addr = a2;
         for(i=0; i&lt;n; i++)
                 copyseg(a1+i, a2++);
         mfree(coremap, n, a1);
         retu(p-&gt;p_addr);
         sureg();
 }
 /* ---------------------------       */




