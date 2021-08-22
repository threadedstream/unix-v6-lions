 /*   KL/DL-11 driver */
 #include "../param.h"
 #include "../conf.h"
 #include "../user.h"
 #include "../tty.h"
 #include "../proc.h"
 /* base address */
 #define KLADDR  0177560 /* console */
 #define KLBASE  0176500 /* kl and dl11-a */
 #define DLBASE  0175610 /* dl-e */
 #define NKL11   1
 #define NDL11   0
 #define DSRDY   02
 #define RDRENB  01
 struct  tty kl11[NKL11+NDL11];
 struct klregs {
         int klrcsr;
         int klrbuf;
         int kltcsr;
         int kltbuf;
 }
 /* ---------------------------       */
 klopen(dev, flag)
 {       register char *addr;
         register struct tty *tp;
         if(dev.d_minor &gt;= NKL11+NDL11) {
                 u.u_error = ENXIO;
                 return;
         }
         tp = &amp;kl11[dev.d_minor];
         if (u.u_procp-&gt;p_ttyp == 0) {
                 u.u_procp-&gt;p_ttyp = tp;
                 tp-&gt;t_dev = dev;
         }
         /* set up minor 0 to address KLADDR
          * set up minor 1 thru NKL11-1 to address from KLBASE
          * set up minor NKL11 on to address from DLBASE
          */
         addr = KLADDR + 8*dev.d_minor;
         if(dev.d_minor)
                 addr =+ KLBASE-KLADDR-8;
         if(dev.d_minor &gt;= NKL11)
                 addr =+ DLBASE-KLBASE-8*NKL11+8;
         tp-&gt;t_addr = addr;
         if ((tp-&gt;t_state&amp;ISOPEN) == 0) {
                 tp-&gt;t_state = ISOPEN|CARR_ON;
                 tp-&gt;t_flags = XTABS|LCASE|ECHO|CRMOD;
                 tp-&gt;t_erase = CERASE;
                 tp-&gt;t_kill = CKILL;
         }
         addr-&gt;klrcsr =| IENABLE|DSRDY|RDRENB;
         addr-&gt;kltcsr =| IENABLE;
 }
 /* ---------------------------       */
 klclose(dev)
 {       register struct tty *tp;
         tp = &amp;kl11[dev.d_minor];
         wflushtty(tp);
         tp-&gt;t_state = 0;
 }
 /* ---------------------------       */
 klread(dev)
 {       ttread(&amp;kl11[dev.d_minor]);
 }
 /* ---------------------------       */
 klwrite(dev)
 {       ttwrite(&amp;kl11[dev.d_minor]);
 }
 /* ---------------------------       */
 klxint(dev)
 {       register struct tty *tp;
         tp = &amp;kl11[dev.d_minor];
         ttstart(tp);
         if (tp-&gt;t_outq.c_cc == 0 || tp-&gt;t_outq.c_cc == TTLOWAT)
                 wakeup(&amp;tp-&gt;t_outq);
 }
 /* ---------------------------       */
 klrint(dev)
 {       register int c, *addr;
         register struct tty *tp;
         tp = &amp;kl11[dev.d_minor];
         addr = tp-&gt;t_addr;
         c = addr-&gt;klrbuf;
         addr-&gt;klrcsr =| RDRENB;
         if ((c&amp;0177)==0)
                 addr-&gt;kltbuf = c;       /* hardware botch */
         ttyinput(c, tp);
 }
 /* ---------------------------       */
 klsgtty(dev, v)
 int *v;
 {       register struct tty *tp;
         tp = &amp;kl11[dev.d_minor];
         ttystty(tp, v);
 }
 /* ---------------------------       */




