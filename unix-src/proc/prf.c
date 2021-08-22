 /*
  */
 
 #include "../param.h"
 #include "../seg.h"
 #include "../buf.h"
 #include "../conf.h"
 
 /*
  * Address and structure of the
  * KL-11 console device registers.
  */
 struct
 {
         int     rsr;
         int     rbr;
         int     xsr;
         int     xbr;
 };
 /* ---------------------------       */
 
 /*
  * In case console is off,
  * panicstr contains argument to last
  * call to panic.
  */
 
 char    *panicstr;
 
 /*
  * Scaled down version of C Library printf.
  * Only %s %l %d (==%l) %o are recognized.
  * Used to print diagnostic information
  * directly on console tty.
  * Since it is not interrupt driven,
  * all system activities are pretty much
  * suspended.
  * Printf should not be used for chit-chat.
  */
 printf(fmt,x1,x2,x3,x4,x5,x6,x7,x8,x9,xa,xb,xc)
 char fmt[];
 {
         register char *s;
         register *adx, c;
 
         adx = &amp;x1;
 loop:
         while((c = *fmt++) != '%') {
                 if(c == '\0')
                         return;
                 putchar(c);
         }
         c = *fmt++;
         if(c == 'd' || c == 'l' || c == 'o')
                 printn(*adx, c=='o'? 8: 10);
         if(c == 's') {
                 s = *adx;
                 while(c = *s++)
                         putchar(c);
         }
         adx++;
         goto loop;
 }
 /* ---------------------------       */
 
 /*
  * Print an unsigned integer in base b.
  */
 printn(n, b)
 {
         register a;
 
         if(a = ldiv(n, b))
                 printn(a, b);
         putchar(lrem(n, b) + '0');
 }
 /* ---------------------------       */
 
 /*
  * Print a character on console.
  * Attempts to save and restore device
  * status.
  * If the switches are 0, all
  * printing is inhibited.
  */
 putchar(c)
 {
         register rc, s;
 
         rc = c;
         if(SW-&gt;integ == 0)
                 return;
         while((KL-&gt;xsr&amp;0200) == 0)
                 ;
         if(rc == 0)
                 return;
         s = KL-&gt;xsr;
         KL-&gt;xsr = 0;
         KL-&gt;xbr = rc;
         if(rc == '\n') {
                 putchar('\r');
                 putchar(0177);
                 putchar(0177);
         }
         putchar(0);
         KL-&gt;xsr = s;
 }
 /* ---------------------------       */
 
 /*
  * Panic is called on unresolvable
  * fatal errors.
  * It syncs, prints "panic: mesg" and
  * then loops.
  */
 panic(s)
 char *s;
 {
         panicstr = s;
         update();
         printf("panic: %s\n", s);
         for(;;)
                 idle();
 }
 /* ---------------------------       */
 
 /*
  * prdev prints a warning message of the
  * form "mesg on dev x/y".
  * x and y are the major and minor parts of
  * the device argument.
  */
 prdev(str, dev)
 {
 
         printf("%s on dev %l/%l\n", str, dev.d_major, dev.d_minor);
 }
 /* ---------------------------       */
 
 /*
  * deverr prints a diagnostic from
  * a device driver.
  * It prints the device, block number,
  * and an octal word (usually some error
  * status register) passed as argument.
  */
 deverror(bp, o1, o2)
 int *bp;
 {
         register *rbp;
 
         rbp = bp;
         prdev("err", rbp-&gt;b_dev);
         printf("bn%l er%o %o\n", rbp-&gt;b_blkno, o1, o2);
 }
 /* ---------------------------       */




