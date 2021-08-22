 #include "../param.h"
 #include "../systm.h"
 #include "../user.h"
 #include "../proc.h"
 #include "../buf.h"
 #include "../reg.h"
 #include "../inode.h"
 
 /*
  * exec system call.
  * Because of the fact that an I/O buffer is used
  * to store the caller's arguments during exec,
  * and more buffers are needed to read in the text file,
  * deadly embraces waiting for free buffers are possible.
  * Therefore the number of processes simultaneously
  * running in exec has to be limited to NEXEC.
  */
 #define EXPRI   -1
 
 exec()
 {
         int ap, na, nc, *bp;
         int ts, ds, sep;
         register c, *ip;
         register char *cp;
         extern uchar;
 
         /*
          * pick up file names
          * and check various modes
          * for execute permission
          */
 
         ip = namei(&amp;uchar, 0);
         if(ip == NULL)
                 return;
         while(execnt &gt;= NEXEC)
                 sleep(&amp;execnt, EXPRI);
         execnt++;
         bp = getblk(NODEV);
         if(access(ip, IEXEC) || (ip-&gt;i_mode&amp;IFMT)!=0)
                 goto bad;
 
         /*
          * pack up arguments into
          * allocated disk buffer
          */
 
         cp = bp-&gt;b_addr;
         na = 0;
         nc = 0;
         while(ap = fuword(u.u_arg[1])) {
                 na++;
                 if(ap == -1)
                         goto bad;
                 u.u_arg[1] =+ 2;
                 for(;;) {
                         c = fubyte(ap++);
                         if(c == -1)
                                 goto bad;
                         *cp++ = c;
                         nc++;
                         if(nc &gt; 510) {
                                 u.u_error = E2BIG;
                                 goto bad;
                         }
                         if(c == 0)
                                 break;
                 }
         }
         if((nc&amp;1) != 0) {
                 *cp++ = 0;
                 nc++;
         }
 
         /* read in first 8 bytes
          * of file for segment
          * sizes:
          * w0 = 407/410/411 (410 implies RO text) (411 implies sep ID)
          * w1 = text size
          * w2 = data size
          * w3 = bss size
          */
 
         u.u_base = &amp;u.u_arg[0];
         u.u_count = 8;
         u.u_offset[1] = 0;
         u.u_offset[0] = 0;
         u.u_segflg = 1;
         readi(ip);
         u.u_segflg = 0;
         if(u.u_error)
                 goto bad;
         sep = 0;
         if(u.u_arg[0] == 0407) {
                 u.u_arg[2] =+ u.u_arg[1];
                 u.u_arg[1] = 0;
         } else
         if(u.u_arg[0] == 0411)
                 sep++; else
         if(u.u_arg[0] != 0410) {
                 u.u_error = ENOEXEC;
                 goto bad;
         }
         if(u.u_arg[1]!=0 &amp;&amp; (ip-&gt;i_flag&amp;ITEXT)==0 &amp;&amp; ip-&gt;i_count!=1) {
                 u.u_error = ETXTBSY;
                 goto bad;
         }
 
         /*
          * find text and data sizes
          * try them out for possible
          * exceed of max sizes
          */
 
         ts = ((u.u_arg[1]+63)&gt;&gt;6) &amp; 01777;
         ds = ((u.u_arg[2]+u.u_arg[3]+63)&gt;&gt;6) &amp; 01777;
         if(estabur(ts, ds, SSIZE, sep))
                 goto bad;
 
         /*
          * allocate and clear core
          * at this point, committed
          * to the new image
          */
 
         u.u_prof[3] = 0;
         xfree();
         expand(USIZE);
         xalloc(ip);
         c = USIZE+ds+SSIZE;
         expand(c);
         while(--c &gt;= USIZE)
                 clearseg(u.u_procp-&gt;p_addr+c);
 
         /* read in data segment */
 
         estabur(0, ds, 0, 0);
         u.u_base = 0;
         u.u_offset[1] = 020+u.u_arg[1];
         u.u_count = u.u_arg[2];
         readi(ip);
 
         /*
          * initialize stack segment
          */
 
         u.u_tsize = ts;
         u.u_dsize = ds;
         u.u_ssize = SSIZE;
         u.u_sep = sep;
         estabur(u.u_tsize, u.u_dsize, u.u_ssize, u.u_sep);
         cp = bp-&gt;b_addr;
         ap = -nc - na*2 - 4;
         u.u_ar0[R6] = ap;
         suword(ap, na);
         c = -nc;
         while(na--) {
                 suword(ap=+2, c);
                 do
                         subyte(c++, *cp);
                 while(*cp++);
         }
         suword(ap+2, -1);
 
         /*
          * set SUID/SGID protections, if no tracing
          */
 
         if ((u.u_procp-&gt;p_flag&amp;STRC)==0) {
                 if(ip-&gt;i_mode&amp;ISUID)
                         if(u.u_uid != 0) {
                                 u.u_uid = ip-&gt;i_uid;
                                 u.u_procp-&gt;p_uid = ip-&gt;i_uid;
                         }
                 if(ip-&gt;i_mode&amp;ISGID)
                         u.u_gid = ip-&gt;i_gid;
         }
 
         /* clear sigs, regs and return */
 
         c = ip;
         for(ip = &amp;u.u_signal[0]; ip &lt; &amp;u.u_signal[NSIG]; ip++)
                 if((*ip &amp; 1) == 0)
                         *ip = 0;
         for(cp = &amp;regloc[0]; cp &lt; &amp;regloc[6];)
                 u.u_ar0[*cp++] = 0;
         u.u_ar0[R7] = 0;
         for(ip = &amp;u.u_fsav[0]; ip &lt; &amp;u.u_fsav[25];)
                 *ip++ = 0;
         ip = c;
 
 bad:
         iput(ip);
         brelse(bp);
         if(execnt &gt;= NEXEC)
                 wakeup(&amp;execnt);
         execnt--;
 }
 /* ---------------------------       */
 
 /* exit system call:
  * pass back caller's r0
  */
 rexit()
 {
 
         u.u_arg[0] = u.u_ar0[R0] &lt;&lt; 8;
         exit();
 }
 /* ---------------------------       */
 
 /* Release resources.
  * Save u. area for parent to look at.
  * Enter zombie state.
  * Wake up parent and init processes,
  * and dispose of children.
  */
 exit()
 {
         register int *q, a;
         register struct proc *p;
 
         u.u_procp-&gt;p_flag =&amp; ~STRC;
         for(q = &amp;u.u_signal[0]; q &lt; &amp;u.u_signal[NSIG];)
                 *q++ = 1;
         for(q = &amp;u.u_ofile[0]; q &lt; &amp;u.u_ofile[NOFILE]; q++)
                 if(a = *q) {
                         *q = NULL;
                         closef(a);
                 }
         iput(u.u_cdir);
         xfree();
         a = malloc(swapmap, 1);
         if(a == NULL)
                 panic("out of swap");
         p = getblk(swapdev, a);
         bcopy(&amp;u, p-&gt;b_addr, 256);
         bwrite(p);
         q = u.u_procp;
         mfree(coremap, q-&gt;p_size, q-&gt;p_addr);
         q-&gt;p_addr = a;
         q-&gt;p_stat = SZOMB;
 
 loop:
         for(p = &amp;proc[0]; p &lt; &amp;proc[NPROC]; p++)
         if(q-&gt;p_ppid == p-&gt;p_pid) {
                 wakeup(&amp;proc[1]);
                 wakeup(p);
                 for(p = &amp;proc[0]; p &lt; &amp;proc[NPROC]; p++)
                 if(q-&gt;p_pid == p-&gt;p_ppid) {
                         p-&gt;p_ppid  = 1;
                         if (p-&gt;p_stat == SSTOP)
                                 setrun(p);
                 }
                 swtch();
                 /* no return */
         }
         q-&gt;p_ppid = 1;
         goto loop;
 }
 /* ---------------------------       */
 
 /* Wait system call.
  * Search for a terminated (zombie) child,
  * finally lay it to rest, and collect its status.
  * Look also for stopped (traced) children,
  * and pass back status from them.
  */
 wait()
 {
         register f, *bp;
         register struct proc *p;
 
         f = 0;
 loop:
         for(p = &amp;proc[0]; p &lt; &amp;proc[NPROC]; p++)
         if(p-&gt;p_ppid == u.u_procp-&gt;p_pid) {
                 f++;
                 if(p-&gt;p_stat == SZOMB) {
                         u.u_ar0[R0] = p-&gt;p_pid;
                         bp = bread(swapdev, f=p-&gt;p_addr);
                         mfree(swapmap, 1, f);
                         p-&gt;p_stat = NULL;
                         p-&gt;p_pid = 0;
                         p-&gt;p_ppid = 0;
                         p-&gt;p_sig = 0;
                         p-&gt;p_ttyp = 0;
                         p-&gt;p_flag = 0;
                         p = bp-&gt;b_addr;
                         u.u_cstime[0] =+ p-&gt;u_cstime[0];
                         dpadd(u.u_cstime, p-&gt;u_cstime[1]);
                         dpadd(u.u_cstime, p-&gt;u_stime);
                         u.u_cutime[0] =+ p-&gt;u_cutime[0];
                         dpadd(u.u_cutime, p-&gt;u_cutime[1]);
                         dpadd(u.u_cutime, p-&gt;u_utime);
                         u.u_ar0[R1] = p-&gt;u_arg[0];
                         brelse(bp);
                         return;
                 }
                 if(p-&gt;p_stat == SSTOP) {
                         if((p-&gt;p_flag&amp;SWTED) == 0) {
                                 p-&gt;p_flag =| SWTED;
                                 u.u_ar0[R0] = p-&gt;p_pid;
                                 u.u_ar0[R1] = (p-&gt;p_sig&lt;&lt;8) | 0177;
                                 return;
                         }
                         p-&gt;p_flag =&amp; ~(STRC|SWTED);
                         setrun(p);
                 }
         }
         if(f) {
                 sleep(u.u_procp, PWAIT);
                 goto loop;
         }
         u.u_error = ECHILD;
 }
 /* ---------------------------       */
 
 /* fork system call.
  */
 fork()
 {
         register struct proc *p1, *p2;
 
         p1 = u.u_procp;
         for(p2 = &amp;proc[0]; p2 &lt; &amp;proc[NPROC]; p2++)
                 if(p2-&gt;p_stat == NULL)
                         goto found;
         u.u_error = EAGAIN;
         goto out;
 
 found:
         if(newproc()) {
                 u.u_ar0[R0] = p1-&gt;p_pid;
                 u.u_cstime[0] = 0;
                 u.u_cstime[1] = 0;
                 u.u_stime = 0;
                 u.u_cutime[0] = 0;
                 u.u_cutime[1] = 0;
                 u.u_utime = 0;
                 return;
         }
         u.u_ar0[R0] = p2-&gt;p_pid;
 
 out:
         u.u_ar0[R7] =+ 2;
 }
 /* ---------------------------       */
 
 /* break system call.
  *  -- bad planning: "break" is a dirty word in C.
  */
 sbreak()
 {
         register a, n, d;
         int i;
 
         /* set n to new data size
          * set d to new-old
          * set n to new total size
          */
 
         n = (((u.u_arg[0]+63)&gt;&gt;6) &amp; 01777);
         if(!u.u_sep)
                 n =- nseg(u.u_tsize) * 128;
         if(n &lt; 0)
                 n = 0;
         d = n - u.u_dsize;
         n =+ USIZE+u.u_ssize;
         if(estabur(u.u_tsize, u.u_dsize+d, u.u_ssize, u.u_sep))
                 return;
         u.u_dsize =+ d;
         if(d &gt; 0)
                 goto bigger;
         a = u.u_procp-&gt;p_addr + n - u.u_ssize;
         i = n;
         n = u.u_ssize;
         while(n--) {
                 copyseg(a-d, a);
                 a++;
         }
         expand(i);
         return;
 
 bigger:
         expand(n);
         a = u.u_procp-&gt;p_addr + n;
         n = u.u_ssize;
         while(n--) {
                 a--;
                 copyseg(a-d, a);
         }
         while(d--)
                 clearseg(--a);
 }
 /* ---------------------------       */




