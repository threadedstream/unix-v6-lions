 /*
  */
 
 /*
  * Structure of the coremap and swapmap
  * arrays. Consists of non-zero count
  * and base address of that many
  * contiguous units.
  * (The coremap unit is 64 bytes,
  * the swapmap unit is 512 bytes)
  * The addresses are increasing and
  * the list is terminated with the
  * first zero count.
  */
 struct map
 {
         char *m_size;
         char *m_addr;
 };
 /* ---------------------------       */
 
 /*
  * Allocate size units from the given
  * map. Return the base of the allocated
  * space.
  * Algorithm is first fit.
  */
 malloc(mp, size)
 struct map *mp;
 {
         register int a;
         register struct map *bp;
 
         for (bp = mp; bp-&gt;m_size; bp++) {
                 if (bp-&gt;m_size &gt;= size) {
                         a = bp-&gt;m_addr;
                         bp-&gt;m_addr =+ size;
                         if ((bp-&gt;m_size =- size) == 0)
                                 do {
                                         bp++;
                                         (bp-1)-&gt;m_addr = bp-&gt;m_addr;
                                 } while ((bp-1)-&gt;m_size = bp-&gt;m_size);
                         return(a);
                 }
         }
         return(0);
 }
 /* ---------------------------       */
 
 /*
  * Free the previously allocated space aa
  * of size units into the specified map.
  * Sort aa into map and combine on
  * one or both ends if possible.
  */
 mfree(mp, size, aa)
 struct map *mp;
 {
         register struct map *bp;
         register int t;
         register int a;
 
         a = aa;
         for (bp = mp; bp-&gt;m_addr&lt;=a &amp;&amp; bp-&gt;m_size!=0; bp++);
         if (bp&gt;mp &amp;&amp; (bp-1)-&gt;m_addr+(bp-1)-&gt;m_size == a) {
                 (bp-1)-&gt;m_size =+ size;
                 if (a+size == bp-&gt;m_addr) {
                         (bp-1)-&gt;m_size =+ bp-&gt;m_size;
                         while (bp-&gt;m_size) {
                                 bp++;
                                 (bp-1)-&gt;m_addr = bp-&gt;m_addr;
                                 (bp-1)-&gt;m_size = bp-&gt;m_size;
                         }
                 }
         } else {
                 if (a+size == bp-&gt;m_addr &amp;&amp; bp-&gt;m_size) {
                         bp-&gt;m_addr =- size;
                         bp-&gt;m_size =+ size;
                 } else if (size) do {
                         t = bp-&gt;m_addr;
                         bp-&gt;m_addr = a;
                         a = t;
                         t = bp-&gt;m_size;
                         bp-&gt;m_size = size;
                         bp++;
                 } while (size = t);
         }
 }
 /* ---------------------------       */




