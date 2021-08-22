 /*
  */
 
 /*
  *      Memory special file
  *      minor device 0 is physical memory
  *      minor device 1 is kernel memory
  *      minor device 2 is EOF/RATHOLE
  */
 
 #include "../param.h"
 #include "../user.h"
 #include "../conf.h"
 #include "../seg.h"
 
 mmread(dev)
 {
         register c, bn, on;
         int a, d;
 
         if(dev.d_minor == 2)
                 return;
         do {
                 bn = lshift(u.u_offset, -6);
                 on = u.u_offset[1] &amp; 077;
                 a = UISA-&gt;r[0];
                 d = UISD-&gt;r[0];
                 spl7();
                 UISA-&gt;r[0] = bn;
                 UISD-&gt;r[0] = 077406;
                 if(dev.d_minor == 1)
                         UISA-&gt;r[0] = (ka6-6)-&gt;r[(bn&gt;&gt;7)&amp;07] + (bn &amp; 0177);
 
                 c = fuibyte(on);
                 UISA-&gt;r[0] = a;
                 UISD-&gt;r[0] = d;
                 spl0();
         } while(u.u_error==0 &amp;&amp; passc(c)&gt;=0);
 }
 /* ---------------------------       */
 
 mmwrite(dev)
 {
         register c, bn, on;
         int a, d;
 
         if(dev.d_minor == 2) {
                 c = u.u_count;
                 u.u_count = 0;
                 u.u_base =+ c;
                 dpadd(u.u_offset, c);
                 return;
         }
         for(;;) {
                 bn = lshift(u.u_offset, -6);
                 on = u.u_offset[1] &amp; 077;
                 if ((c=cpass())&lt;0 || u.u_error!=0)
                         break;
                 a = UISA-&gt;r[0];
                 d = UISD-&gt;r[0];
                 spl7();
                 UISA-&gt;r[0] = bn;
                 UISD-&gt;r[0] = 077406;
                 if(dev.d_minor == 1)
                         UISA-&gt;r[0] = (ka6-6)-&gt;r[(bn&gt;&gt;7)&amp;07] + (bn &amp; 0177);
 
                 suibyte(on, c);
                 UISA-&gt;r[0] = a;
                 UISD-&gt;r[0] = d;
                 spl0();
         }
 }
 /* ---------------------------       */




