 /*
  */
 
 /*
  * LP-11 Line printer driver
  */
 
 #include "../param.h"
 #include "../conf.h"
 #include "../user.h"
 
 #define LPADDR  0177514
 
 #define IENABLE 0100
 #define DONE    0200
 
 #define LPPRI   10
 #define LPLWAT  50
 #define LPHWAT  100
 #define EJLINE  60
 #define MAXCOL  80
 
 struct {
         int lpsr;
         int lpbuf;
 };
 /* ---------------------------       */
 
 struct  {
         int     cc;
         int     cf;
         int     cl;
         int     flag;
         int     mcc;
         int     ccc;
         int     mlc;
 } lp11;
 /* ---------------------------       */
 
 #define CAP     01              /* Set to 0 for 96-char printer, else to 01 */
 
 #define EJECT   02
 #define OPEN    04
 #define IND     010             /* Set to 0 for no indent, else to 010 */
 
 
 #define FORM    014
 
 
 lpopen(dev, flag)
 {
 
         if(lp11.flag &amp; OPEN || LPADDR-&gt;lpsr &lt; 0) {
                 u.u_error = EIO;
                 return;
         }
         lp11.flag =| (IND|EJECT|OPEN);
         LPADDR-&gt;lpsr =| IENABLE;
         lpcanon(FORM);
 }
 /* ---------------------------       */
 
 lpclose(dev, flag)
 {
         lpcanon(FORM);
         lp11.flag = 0;
 }
 /* ---------------------------       */
 
 lpwrite()
 {
         register int c;
 
         while ((c=cpass())&gt;=0)
                 lpcanon(c);
 }
 /* ---------------------------       */
 
 lpcanon(c)
 {
         register c1, c2;
 
         c1 = c;
         if(lp11.flag&amp;CAP) {
                 if(c1&gt;='a' &amp;&amp; c1&lt;='z')
                         c1 =+ 'A'-'a'; else
                 switch(c1) {
 
                 case '{':
                         c2 = '(';
                         goto esc;
 
                 case '}':
                         c2 = ')';
                         goto esc;
 
                 case '`':
                         c2 = '\'';
                         goto esc;
 
                 case '|':
                         c2 = '!';
                         goto esc;
 
                 case '~':
                         c2 = '^';
 
                 esc:
                         lpcanon(c2);
                         lp11.ccc--;
                         c1 = '-';
                 }
         }
 
         switch(c1) {
 
         case '\t':
                 lp11.ccc = (lp11.ccc+8) &amp; ~7;
                 return;
 
         case FORM:
         case '\n':
                 if((lp11.flag&amp;EJECT) == 0 ||
                    lp11.mcc!=0 || lp11.mlc!=0) {
                         lp11.mcc = 0;
                         lp11.mlc++;
                         if(lp11.mlc &gt;= EJLINE &amp;&amp; lp11.flag&amp;EJECT)
                                 c1 = FORM;
                         lpoutput(c1);
                         if(c1 == FORM)
                                 lp11.mlc = 0;
                 }
 
         case '\r':
                 lp11.ccc = 0;
                 if(lp11.flag&amp;IND)
                         lp11.ccc = 8;
                 return;
 
         case 010:
                 if(lp11.ccc &gt; 0)
                         lp11.ccc--;
                 return;
 
         case ' ':
                 lp11.ccc++;
                 return;
 
         default:
                 if(lp11.ccc &lt; lp11.mcc) {
                         lpoutput('\r');
                         lp11.mcc = 0;
                 }
                 if(lp11.ccc &lt; MAXCOL) {
                         while(lp11.ccc &gt; lp11.mcc) {
                                 lpoutput(' ');
                                 lp11.mcc++;
                         }
                         lpoutput(c1);
                         lp11.mcc++;
                 }
                 lp11.ccc++;
         }
 }
 /* ---------------------------       */
 
 lpstart()
 {
         register int c;
 
         while (LPADDR-&gt;lpsr&amp;DONE &amp;&amp; (c = getc(&amp;lp11)) &gt;= 0)
                 LPADDR-&gt;lpbuf = c;
 }
 /* ---------------------------       */
 
 lpint()
 {
         register int c;
 
         lpstart();
         if (lp11.cc == LPLWAT || lp11.cc == 0)
                 wakeup(&amp;lp11);
 }
 /* ---------------------------       */
 
 lpoutput(c)
 {
         if (lp11.cc &gt;= LPHWAT)
                 sleep(&amp;lp11, LPPRI);
         putc(c, &amp;lp11);
         spl4();
         lpstart();
         spl0();
 }
 /* ---------------------------       */




