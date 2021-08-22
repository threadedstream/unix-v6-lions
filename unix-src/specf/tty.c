
/* general TTY subroutines */

 #include "../param.h"
 #include "../systm.h"
 #include "../user.h"
 #include "../tty.h"
 #include "../proc.h"
 #include "../inode.h"
 #include "../file.h"
 #include "../reg.h"
 #include "../conf.h"

 /* Input mapping table-- if an entry is non-zero, when the
  * corresponding character is typed preceded by "\" the escape
  * sequence is replaced by the table value.  Mostly used for
  * upper-case only terminals.
  */
 char    maptab[]
 {
         000,000,000,000,004,000,000,000,
         000,000,000,000,000,000,000,000,
         000,000,000,000,000,000,000,000,
         000,000,000,000,000,000,000,000,
         000,'|',000,'#',000,000,000,'`',
         '{','}',000,000,000,000,000,000,
         000,000,000,000,000,000,000,000,
         000,000,000,000,000,000,000,000,
         '@',000,000,000,000,000,000,000,
         000,000,000,000,000,000,000,000,
         000,000,000,000,000,000,000,000,
         000,000,000,000,000,000,'~',000,
         000,'A','B','C','D','E','F','G',
         'H','I','J','K','L','M','N','O',
         'P','Q','R','S','T','U','V','W',
         'X','Y','Z',000,000,000,000,000,
 };
 /* ---------------------------       */
 /* The actual structure of a clist block manipulated by
  * getc and putc (mch.s)
  */
 struct cblock {
         struct cblock *c_next;
         char info[6];
 };
 /* ---------------------------       */
 /* The character lists-- space for 6*NCLIST characters */
 struct cblock cfree[NCLIST];
 /* List head for unused character blocks. */
 struct cblock *cfreelist;

 /* structure of device registers for KL, DL, and DC
  * interfaces-- more particularly, those for which the
  * SSTART bit is off and can be treated by general routines
  * (that is, not DH).
  */
 struct {
         int ttrcsr;
         int ttrbuf;
         int tttcsr;
         int tttbuf;
 };
 /* ---------------------------       */
 /* The routine implementing the gtty system call.
  * Just call lower level routine and pass back values.
  */
 gtty()
 {
         int v[3];
         register *up, *vp;

         vp = v;
         sgtty(vp);
         if (u.u_error)
                 return;
         up = u.u_arg[0];
         suword(up, *vp++);
         suword(++up, *vp++);
         suword(++up, *vp++);
 }
 /* ---------------------------       */
 /* The routine implementing the stty system call.
  * Read in values and call lower level.
  */
 stty()
 {
         register int *up;

         up = u.u_arg[0];
         u.u_arg[0] = fuword(up);
         u.u_arg[1] = fuword(++up);
         u.u_arg[2] = fuword(++up);
         sgtty(0);
 }
 /* ---------------------------       */
 /* Stuff common to stty and gtty.
  * Check legality and switch out to individual
  * device routine.
  * v  is 0 for stty; the parameters are taken from u.u_arg[].
  * c  is non-zero for gtty and is the place in which the device
  * routines place their information.
  */
 sgtty(v)
 int *v;
 {
         register struct file *fp;
         register struct inode *ip;
         if ((fp = getf(u.u_ar0[R0])) == NULL)
                 return;
         ip = fp-&gt;f_inode;
         if ((ip-&gt;i_mode&amp;IFMT) != IFCHR) {
                 u.u_error = ENOTTY;
                 return;
         }
         (*cdevsw[ip-&gt;i_addr[0].d_major].d_sgtty)(ip-&gt;i_addr[0], v);
 }
 /* ---------------------------       */
 /* Wait for output to drain, then flush input waiting. */
 wflushtty(atp)
 struct tty *atp;
 {
         register struct tty *tp;
         tp = atp;
         spl5();
         while (tp-&gt;t_outq.c_cc) {
                 tp-&gt;t_state =| ASLEEP;
                 sleep(&amp;tp-&gt;t_outq, TTOPRI);
         }
         flushtty(tp);
         spl0();
 }
 /* ---------------------------       */
 /* Initialize clist by freeing all character blocks, then count
  * number of character devices. (Once-only routine)
  */
 cinit()
 {
         register int ccp;
         register struct cblock *cp;
         register struct cdevsw *cdp;
         ccp = cfree;
         for (cp=(ccp+07)&amp;~07; cp &lt;= &amp;cfree[NCLIST-1]; cp++) {
                 cp-&gt;c_next = cfreelist;
                 cfreelist = cp;
         }
         ccp = 0;
         for(cdp = cdevsw; cdp-&gt;d_open; cdp++)
                 ccp++;
         nchrdev = ccp;
 }
 /* ---------------------------       */
 /* flush all TTY queues
  */
 flushtty(atp)
 struct tty *atp;
 {
         register struct tty *tp;
         register int sps;
         tp = atp;
         while (getc(&amp;tp-&gt;t_canq) &gt;= 0);
         while (getc(&amp;tp-&gt;t_outq) &gt;= 0);
         wakeup(&amp;tp-&gt;t_rawq);
         wakeup(&amp;tp-&gt;t_outq);
         sps = PS-&gt;integ;
         spl5();
         while (getc(&amp;tp-&gt;t_rawq) &gt;= 0);
         tp-&gt;t_delct = 0;
         PS-&gt;integ = sps;
 }
 /* ---------------------------       */
 /* transfer raw input list to canonical list,
  * doing erase-kill processing and handling escapes.
  * It waits until a full line has been typed in cooked mode,
  * or until any character has been typed in raw mode.
  */
 canon(atp)
 struct tty *atp;
 {
         register char *bp;
         char *bp1;
         register struct tty *tp;
         register int c;

         tp = atp;
         spl5();
         while (tp-&gt;t_delct==0) {
                 if ((tp-&gt;t_state&amp;CARR_ON)==0)
                         return(0);
                 sleep(&amp;tp-&gt;t_rawq, TTIPRI);
         }
         spl0();
 loop:
         bp = &amp;canonb[2];
         while ((c=getc(&amp;tp-&gt;t_rawq)) &gt;= 0) {
                 if (c==0377) {
                         tp-&gt;t_delct--;
                         break;
                 }
                 if ((tp-&gt;t_flags&amp;RAW)==0) {
                         if (bp[-1]!='\\') {
                                 if (c==tp-&gt;t_erase) {
                                         if (bp &gt; &amp;canonb[2])
                                                 bp--;
                                         continue;
                                 }
                                 if (c==tp-&gt;t_kill)
                                         goto loop;
                                 if (c==CEOT)
                                         continue;
                         } else
                         if (maptab[c] &amp;&amp; (maptab[c]==c || (tp-&gt;t_flags&amp;LCASE))) {
                                 if (bp[-2] != '\\')
                                         c = maptab[c];
                                 bp--;
                         }
                 }
                 *bp++ = c;
                 if (bp&gt;=canonb+CANBSIZ)
                         break;
         }
         bp1 = bp;
         bp = &amp;canonb[2];
         c = &amp;tp-&gt;t_canq;
         while (bp&lt;bp1)
                 putc(*bp++, c);
         return(1);
 }
 /* ---------------------------       */
 /* Place a character on raw TTY input queue, putting in delimiters
  * and waking up top half as needed.
  * Also echo if required.
  * The arguments are the character and the appropriate
  * tty structure.
  */
 ttyinput(ac, atp)
 struct tty *atp;
 {
         register int t_flags, c;
         register struct tty *tp;

         tp = atp;
         c = ac;
         t_flags = tp-&gt;t_flags;
         if ((c =&amp; 0177) == '\r' &amp;&amp; t_flags&amp;CRMOD)
                 c = '\n';
         if ((t_flags&amp;RAW)==0 &amp;&amp; (c==CQUIT || c==CINTR)) {
                 signal(tp, c==CINTR? SIGINT:SIGQIT);
                 flushtty(tp);
                 return;
         }
         if (tp-&gt;t_rawq.c_cc&gt;=TTYHOG) {
                 flushtty(tp);
                 return;
         }
         if (t_flags&amp;LCASE &amp;&amp; c&gt;='A' &amp;&amp; c&lt;='Z')
                 c =+ 'a'-'A';
         putc(c, &amp;tp-&gt;t_rawq);
         if (t_flags&amp;RAW || c=='\n' || c==004) {
                 wakeup(&amp;tp-&gt;t_rawq);
                 if (putc(0377, &amp;tp-&gt;t_rawq)==0)
                         tp-&gt;t_delct++;
         }
         if (t_flags&amp;ECHO) {
                 ttyoutput(c, tp);
                 ttstart(tp);
         }
 }
 /* ---------------------------       */
 /* put character on TTY output queue, adding delays,
  * expanding tabs, and handling the CR/NL bit.
  * It is called both from the top half for output, and from
  * interrupt level for echoing.
  * The arguments are the character and the tty structure.
  */
 ttyoutput(ac, tp)
 struct tty *tp;
 {
         register int c;
         register struct tty *rtp;
         register char *colp;
         int ctype;

         rtp = tp;
         c = ac&amp;0177;
         /* Ignore EOT in normal mode to avoid hanging up
          * certain terminals.
          */
         if (c==004 &amp;&amp; (rtp-&gt;t_flags&amp;RAW)==0)
                 return;
         /* Turn tabs to spaces as required
          */
         if (c=='\t' &amp;&amp; rtp-&gt;t_flags&amp;XTABS) {
                 do
                         ttyoutput(' ', rtp);
                 while (rtp-&gt;t_col&amp;07);
                 return;
         }
         /* for upper-case-only terminals,
          * generate escapes.
          */
         if (rtp-&gt;t_flags&amp;LCASE) {
                 colp = "({)}!|^~'`";
                 while(*colp++)
                         if(c == *colp++) {
                                 ttyoutput('\\', rtp);
                                 c = colp[-2];
                                 break;
                         }
                 if ('a'&lt;=c &amp;&amp; c&lt;='z')
                         c =+ 'A' - 'a';
         }
         /* turn &lt;nl&gt; to &lt;cr&gt;&lt;lf&gt; if desired.
          */
         if (c=='\n' &amp;&amp; rtp-&gt;t_flags&amp;CRMOD)
                 ttyoutput('\r', rtp);
         if (putc(c, &amp;rtp-&gt;t_outq))
                 return;
         /* Calculate delays.
          * The numbers here represent clock ticks
          * and are not necessarily optimal for all terminals.
          * The delays are indicated by characters above 0200,
          * thus (unfortunately) restricting the transmission
          * path to 7 bits.
          */
         colp = &amp;rtp-&gt;t_col;
         ctype = partab[c];
         c = 0;
         switch (ctype&amp;077) {
         /* ordinary */
         case 0:
                 (*colp)++;
         /* non-printing */
         case 1:
                 break;
         /* backspace */
         case 2:
                 if (*colp)
                         (*colp)--;
                 break;
         /* newline */
         case 3:
                 ctype = (rtp-&gt;t_flags &gt;&gt; 8) &amp; 03;
                 if(ctype == 1) { /* tty 37 */
                         if (*colp)
                                 c = max((*colp&gt;&gt;4) + 3, 6);
                 } else
                 if(ctype == 2) { /* vt05 */
                         c = 6;
                 }
                 *colp = 0;
                 break;
         /* tab */
         case 4:
                 ctype = (rtp-&gt;t_flags &gt;&gt; 10) &amp; 03;
                 if(ctype == 1) { /* tty 37 */
                         c = 1 - (*colp | ~07);
                         if(c &lt; 5)
                                 c = 0;
                 }
                 *colp =| 07;
                 (*colp)++;
                 break;
         /* vertical motion */
         case 5:
                 if(rtp-&gt;t_flags &amp; VTDELAY) /* tty 37 */
                         c = 0177;
                 break;
         /* carriage return */
         case 6:
                 ctype = (rtp-&gt;t_flags &gt;&gt; 12) &amp; 03;
                 if(ctype == 1) { /* tn 300 */
                         c = 5;
                 } else
                 if(ctype == 2) { /* ti 700 */
                         c = 10;
                 }
                 *colp = 0;
         }
         if(c)
                 putc(c|0200, &amp;rtp-&gt;t_outq);
 }
 /* ---------------------------       */
 /* Restart typewriter output following a delay
  * timeout.
  * The name of the routine is passed to the timeout
  * subroutine and it is called during a clock interrupt.
  */
 ttrstrt(atp)
 {
         register struct tty *tp;

         tp = atp;
         tp-&gt;t_state =&amp; ~TIMEOUT;
         ttstart(tp);
 }
 /* ---------------------------       */
 /*
  * Start output on the typewriter. It is used from the top half
  * after some characters have been put on the output queue,
  * from the interrupt routine to transmit the next
  * character, and after a timeout has finished.
  * If the SSTART bit is off for the tty the work is done here,
  * using the protocol of the single-line interfaces (KL, DL, DC);
  * otherwise the address word of the tty structure is
  * taken to be the name of the device-dependent startup routine.
  */
 ttstart(atp)
 struct tty *atp;
 {
         register int *addr, c;
         register struct tty *tp;
         struct { int (*func)(); };

         tp = atp;
         addr = tp-&gt;t_addr;
         if (tp-&gt;t_state&amp;SSTART) {
                 (*addr.func)(tp);
                 return;
         }
         if ((addr-&gt;tttcsr&amp;DONE)==0 || tp-&gt;t_state&amp;TIMEOUT)
                 return;
         if ((c=getc(&amp;tp-&gt;t_outq)) &gt;= 0) {
                 if (c&lt;=0177)
                         addr-&gt;tttbuf = c | (partab[c]&amp;0200);
                 else {
                         timeout(ttrstrt, tp, c&amp;0177);
                         tp-&gt;t_state =| TIMEOUT;
                 }
         }
 }
 /* ---------------------------       */
 /* Called from device's read routine after it has
  * calculated the tty-structure given as argument.
  * The pc is backed up for the duration of this call.
  * In case of a caught interrupt, an RTI will re-execute.
  */
 ttread(atp)
 struct tty *atp;
 {
         register struct tty *tp;

         tp = atp;
         if ((tp-&gt;t_state&amp;CARR_ON)==0)
                 return;
         if (tp-&gt;t_canq.c_cc || canon(tp))
                 while (tp-&gt;t_canq.c_cc &amp;&amp; passc(getc(&amp;tp-&gt;t_canq))&gt;=0);
 }
 /* ---------------------------       */
 /* Called from the device's write routine after it has
  * calculated the tty-structure given as argument.
  */
 ttwrite(atp)
 struct tty *atp;
 {
         register struct tty *tp;
         register int c;
         tp = atp;
         if ((tp-&gt;t_state&amp;CARR_ON)==0)
                 return;
         while ((c=cpass())&gt;=0) {
                 spl5();
                 while (tp-&gt;t_outq.c_cc &gt; TTHIWAT) {
                         ttstart(tp);
                         tp-&gt;t_state =| ASLEEP;
                         sleep(&amp;tp-&gt;t_outq, TTOPRI);
                 }
                 spl0();
                 ttyoutput(c, tp);
         }
         ttstart(tp);
 }
 /* ---------------------------       */
 /* Common code for gtty and stty functions on typewriters.
  * If v is non-zero then gtty is being done and information is
  * passed back therein;
  * if it is zero stty is being done and the input information is in the
  * u_arg array.
  */
 ttystty(atp, av)
 int *atp, *av;
 {
         register  *tp, *v;
         tp = atp;
         if(v = av) {
                 *v++ = tp-&gt;t_speeds;
                 v-&gt;lobyte = tp-&gt;t_erase;
                 v-&gt;hibyte = tp-&gt;t_kill;
                 v[1] = tp-&gt;t_flags;
                 return(1);
         }
         wflushtty(tp);
         v = u.u_arg;
         tp-&gt;t_speeds = *v++;
         tp-&gt;t_erase = v-&gt;lobyte;
         tp-&gt;t_kill = v-&gt;hibyte;
         tp-&gt;t_flags = v[1];
         return(0);
 }
 /* ---------------------------       */




