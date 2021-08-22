 #include "../param.h"
 #include "../systm.h"
 #include "../user.h"
 #include "../proc.h"
 
 #define UMODE   0170000
 #define SCHMAG  10
 
 /*
  * clock is called straight from
  * the real time clock interrupt.
  *
  * Functions:
  *      reprime clock
  *      copy *switches to display
  *      implement callouts
  *      maintain user/system times
  *      maintain date
  *      profile
  *      tout wakeup (sys sleep)
  *      lightning bolt wakeup (every 4 sec)
  *      alarm clock signals
  *      jab the scheduler
  */
 clock(dev, sp, r1, nps, r0, pc, ps)
 {
         register struct callo *p1, *p2;
         register struct proc *pp;
 
         /*
          * restart clock
          */
 
         *lks = 0115;
 
         /*
          * display register
          */
 
         display();
 
         /*
          * callouts
          * if none, just return
          * else update first non-zero time
          */
 
         if(callout[0].c_func == 0)
                 goto out;
         p2 = &amp;callout[0];
         while(p2-&gt;c_time&lt;=0 &amp;&amp; p2-&gt;c_func!=0)
                 p2++;
         p2-&gt;c_time--;
 
         /*
          * if ps is high, just return
          */
 
         if((ps&amp;0340) != 0)
                 goto out;
 
         /*
          * callout
          */
 
         spl5();
         if(callout[0].c_time &lt;= 0) {
                 p1 = &amp;callout[0];
                 while(p1-&gt;c_func != 0 &amp;&amp; p1-&gt;c_time &lt;= 0) {
                         (*p1-&gt;c_func)(p1-&gt;c_arg);
                         p1++;
                 }
                 p2 = &amp;callout[0];
                 while(p2-&gt;c_func = p1-&gt;c_func) {
                         p2-&gt;c_time = p1-&gt;c_time;
                         p2-&gt;c_arg = p1-&gt;c_arg;
                         p1++;
                         p2++;
                 }
         }
 
         /*
          * lightning bolt time-out
          * and time of day
          */
 
 out:
         if((ps&amp;UMODE) == UMODE) {
                 u.u_utime++;
                 if(u.u_prof[3])
                         incupc(pc, u.u_prof);
         } else
                 u.u_stime++;
         pp = u.u_procp;
         if(++pp-&gt;p_cpu == 0)
                 pp-&gt;p_cpu--;
         if(++lbolt &gt;= HZ) {
                 if((ps&amp;0340) != 0)
                         return;
                 lbolt =- HZ;
                 if(++time[1] == 0)
                         ++time[0];
                 spl1();
                 if(time[1]==tout[1] &amp;&amp; time[0]==tout[0])
                         wakeup(tout);
                 if((time[1]&amp;03) == 0) {
                         runrun++;
                         wakeup(&amp;lbolt);
                 }
                 for(pp = &amp;proc[0]; pp &lt; &amp;proc[NPROC]; pp++)
                 if (pp-&gt;p_stat) {
                         if(pp-&gt;p_time != 127)
                                 pp-&gt;p_time++;
                         if((pp-&gt;p_cpu &amp; 0377) &gt; SCHMAG)
                                 pp-&gt;p_cpu =- SCHMAG; else
                                 pp-&gt;p_cpu = 0;
                         if(pp-&gt;p_pri &gt; PUSER)
                                 setpri(pp);
                 }
                 if(runin!=0) {
                         runin = 0;
                         wakeup(&amp;runin);
                 }
                 if((ps&amp;UMODE) == UMODE) {
                         u.u_ar0 = &amp;r0;
                         if(issig())
                                 psig();
                         setpri(u.u_procp);
                 }
         }
 }
 /* ---------------------------       */
 
 /*
  * timeout is called to arrange that
  * fun(arg) is called in tim/HZ seconds.
  * An entry is sorted into the callout
  * structure. The time in each structure
  * entry is the number of HZ's more
  * than the previous entry.
  * In this way, decrementing the
  * first entry has the effect of
  * updating all entries.
  */
 timeout(fun, arg, tim)
 {
         register struct callo *p1, *p2;
         register t;
         int s;
 
         t = tim;
         s = PS-&gt;integ;
         p1 = &amp;callout[0];
         spl7();
         while(p1-&gt;c_func != 0 &amp;&amp; p1-&gt;c_time &lt;= t) {
                 t =- p1-&gt;c_time;
                 p1++;
         }
         p1-&gt;c_time =- t;
         p2 = p1;
         while(p2-&gt;c_func != 0)
                 p2++;
         while(p2 &gt;= p1) {
                 (p2+1)-&gt;c_time = p2-&gt;c_time;
                 (p2+1)-&gt;c_func = p2-&gt;c_func;
                 (p2+1)-&gt;c_arg = p2-&gt;c_arg;
                 p2--;
         }
         p1-&gt;c_time = t;
         p1-&gt;c_func = fun;
         p1-&gt;c_arg = arg;
         PS-&gt;integ = s;
 }
 /* ---------------------------       */




