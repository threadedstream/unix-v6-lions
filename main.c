


main.c





1500: #
1501: #include "../param.h"
1502: #include "../user.h"
1503: #include "../systm.h"
1504: #include "../proc.h"
1505: #include "../text.h"
1506: #include "../inode.h"
1507: #include "../seg.h"
1508:
1509: #define CLOCK1  0177546
1510: #define CLOCK2  0172540
1511: /*
1512:  * Icode is the octal bootstrap
1513:  * program executed in user mode
1514:  * to bring up the system.
1515:  */
1516: int     icode[]
1517: {
1518:         0104413,        /* sys exec; init; initp */
1519:         0000014,
1520:         0000010,
1521:         0000777,        /* br . */
1522:         0000014,        /* initp: init; 0 */
1523:         0000000,
1524:         0062457,        /* init: &lt;/etc/init\0&gt; */
1525:         0061564,
1526:         0064457,
1527:         0064556,
1529: };
1528:         0000164,
1531:
1530: /* ---------------------------       */
1532: /*
1533:  * Initialization code.
1534:  * Called from m40.s or m45.s as
1535:  * soon as a stack and segmentation
1536:  * have been established.
1537:  * Functions:
1538:  *      clear and free user core
1539:  *      find which clock is configured
1540:  *      hand craft 0th process
1541:  *      call all initialization routines
1542:  *      fork - process 0 to schedule
1543:  *           - process 1 execute bootstrap
1544:  *
1545:  * panic: no clock -- neither clock responds
1546:  * loop at loc 6 in user mode -- /etc/init
1547:  *      cannot be executed.
1548:  */
1549:
1550: main()
1551: {
1552:         extern schar;
1553:         register i, *p;
1554:
1555:         /*
1556:          * zero and free all of core
1557:          */
1558:
1559:         updlock = 0;
1560:         i = *ka6 + USIZE;
1561:         UISD-&gt;r[0] = 077406;
1562:         for(;;) {
1563:                 UISA-&gt;r[0] = i;
1564:                 if(fuibyte(0) &lt; 0)
1565:                         break;
1566:                 clearseg(i);
1567:                 maxmem++;
1568:                 mfree(coremap, 1, i);
1569:                 i++;
1570:         }
1571:         if(cputype == 70)
1572:         for(i=0; i&lt;62; i=+2) {
1573:                 UBMAP-&gt;r[i] = i&lt;&lt;12;
1574:                 UBMAP-&gt;r[i+1] = 0;
1575:         }
1576:         printf("mem = %l\n", maxmem*5/16);
1577:
1578:
1579:
1580:
1581:
1582:         maxmem = min(maxmem, MAXMEM);
1583:         mfree(swapmap, nswap, swplo);
1584:
1585:         /*
1586:          * set up system process
1587:          */
1588:
1589:         proc[0].p_addr = *ka6;
1590:         proc[0].p_size = USIZE;
1591:         proc[0].p_stat = SRUN;
1592:         proc[0].p_flag =| SLOAD|SSYS;
1593:         u.u_procp = &amp;proc[0];
1594:
1595:         /*
1596:          * determine clock
1597:          */
1598:
1599:         UISA-&gt;r[7] = ka6[1]; /* io segment */
1600:         UISD-&gt;r[7] = 077406;
1601:         lks = CLOCK1;
1602:         if(fuiword(lks) == -1) {
1603:                 lks = CLOCK2;
1604:                 if(fuiword(lks) == -1)
1605:                         panic("no clock");
1606:         }
1607:
1608:         /*
1609:          * set up 'known' i-nodes
1610:          */
1611:
1612:         *lks = 0115;
1613:         cinit();
1614:         binit();
1615:         iinit();
1616:         rootdir = iget(rootdev, ROOTINO);
1617:         rootdir-&gt;i_flag =&amp; ~ILOCK;
1618:         u.u_cdir = iget(rootdev, ROOTINO);
1619:         u.u_cdir-&gt;i_flag =&amp; ~ILOCK;
1620:
1621:         /*
1622:          * make init process
1623:          * enter scheduling loop
1624:          * with system process
1625:          */
1626:
1627:         if(newproc()) {
1628:                 expand(USIZE+1);
1629:                 estabur(0, 1, 0, 0);
1630:                 copyout(icode, 0, sizeof icode);
1631:                 /*
1632:                  * Return goes to loc. 0 of user init
1633:                  * code just copied out.
1634:                  */
1635:                 return;
1636:         }
1637:         sched();
1638: }
1639: /* ---------------------------       */
1640:
1641: /*
1642:  * Set up software prototype segmentation
1643:  * registers to implement the 3 pseudo
1644:  * text,data,stack segment sizes passed
1645:  * as arguments.
1646:  * The argument sep specifies if the
1647:  * text and data+stack segments are to
1648:  * be separated.
1649:  */
1650: estabur(nt, nd, ns, sep)
1651: {
1652:         register a, *ap, *dp;
1653:
1654:         if(sep) {
1655:                 if(cputype == 40)
1656:                         goto err;
1657:                 if(nseg(nt) &gt; 8 || nseg(nd)+nseg(ns) &gt; 8)
1658:                         goto err;
1659:         } else
1660:                 if(nseg(nt)+nseg(nd)+nseg(ns) &gt; 8)
1661:                         goto err;
1662:         if(nt+nd+ns+USIZE &gt; maxmem)
1663:                 goto err;
1664:         a = 0;
1665:         ap = &amp;u.u_uisa[0];
1666:         dp = &amp;u.u_uisd[0];
1667:         while(nt &gt;= 128) {
1668:                 *dp++ = (127&lt;&lt;8) | RO;
1669:                 *ap++ = a;
1670:                 a =+ 128;
1671:                 nt =- 128;
1672:         }
1673:         if(nt) {
1674:                 *dp++ = ((nt-1)&lt;&lt;8) | RO;
1675:                 *ap++ = a;
1676:         }
1677:         if(sep)
1678:         while(ap &lt; &amp;u.u_uisa[8]) {
1679:                 *ap++ = 0;
1680:                 *dp++ = 0;
1681:         }
1682:         a = USIZE;
1683:         while(nd &gt;= 128) {
1684:                 *dp++ = (127&lt;&lt;8) | RW;
1685:                 *ap++ = a;
1686:                 a =+ 128;
1687:                 nd =- 128;
1688:         }
1689:         if(nd) {
1690:                 *dp++ = ((nd-1)&lt;&lt;8) | RW;
1691:                 *ap++ = a;
1692:                 a =+ nd;
1693:         }
1694:         while(ap &lt; &amp;u.u_uisa[8]) {
1695:                 *dp++ = 0;
1696:                 *ap++ = 0;
1697:         }
1698:         if(sep)
1699:         while(ap &lt; &amp;u.u_uisa[16]) {
1700:                 *dp++ = 0;
1701:                 *ap++ = 0;
1702:         }
1703:         a =+ ns;
1704:         while(ns &gt;= 128) {
1705:                 a =- 128;
1706:                 ns =- 128;
1707:                 *--dp = (127&lt;&lt;8) | RW;
1708:                 *--ap = a;
1709:         }
1710:         if(ns) {
1711:                 *--dp = ((128-ns)&lt;&lt;8) | RW | ED;
1712:                 *--ap = a-128;
1713:         }
1714:         if(!sep) {
1715:                 ap = &amp;u.u_uisa[0];
1716:                 dp = &amp;u.u_uisa[8];
1717:                 while(ap &lt; &amp;u.u_uisa[8])
1718:                         *dp++ = *ap++;
1719:                 ap = &amp;u.u_uisd[0];
1720:                 dp = &amp;u.u_uisd[8];
1721:                 while(ap &lt; &amp;u.u_uisd[8])
1722:                         *dp++ = *ap++;
1723:         }
1724:         sureg();
1725:         return(0);
1726:
1727: err:
1728:         u.u_error = ENOMEM;
1729:         return(-1);
1730: }
1731: /* ---------------------------       */
1732:
1733: /*
1734:  * Load the user hardware segmentation
1735:  * registers from the software prototype.
1736:  * The software registers must have
1737:  * been setup prior by estabur.
1738:  */
1739: sureg()
1740: {
1741:         register *up, *rp, a;
1742:
1743:         a = u.u_procp-&gt;p_addr;
1744:         up = &amp;u.u_uisa[16];
1745:         rp = &amp;UISA-&gt;r[16];
1746:         if(cputype == 40) {
1747:                 up =- 8;
1748:                 rp =- 8;
1749:         }
1750:         while(rp &gt; &amp;UISA-&gt;r[0])
1751:                 *--rp = *--up + a;
1752:         if((up=u.u_procp-&gt;p_textp) != NULL)
1753:                 a =- up-&gt;x_caddr;
1754:         up = &amp;u.u_uisd[16];
1755:         rp = &amp;UISD-&gt;r[16];
1756:         if(cputype == 40) {
1757:                 up =- 8;
1758:                 rp =- 8;
1759:         }
1760:         while(rp &gt; &amp;UISD-&gt;r[0]) {
1761:                 *--rp = *--up;
1762:                 if((*rp &amp; WO) == 0)
1763:                         rp[(UISA-UISD)/2] =- a;
1764:         }
1765: }
1766: /* ---------------------------       */
1767:
1768: /*
1769:  * Return the arg/128 rounded up.
1770:  */
1771: nseg(n)
1772: {
1773:
1774:         return((n+127)&gt;&gt;7);
1775: }
1776: /* ---------------------------       */




