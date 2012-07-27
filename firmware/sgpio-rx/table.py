mossmann@grumio ~/github/hackrf/firmware/simpletx $ python
Python 2.7.3 (default, Jun 22 2012, 11:10:47) 
[GCC 4.5.3] on linux2
Type "help", "copyright", "credits" or "license" for more information.
>>> import math
>>> def y(i,max):
...     return int(127.5*(math.sin(tau*i/max)+1))
... 
>>> tau=math.pi*2
>>> def x(i,max):
...     return int(127.5*(math.cos(tau*i/max)+1))
... 
>>> def table(max):
...     for i in range(0, max, 2):
...             print "%02x%02x%02x%02x," % (y(i+1,max), x(i+1,max), y(i,max), x(i,max))
... 
>>> table(32)
98fc7fff,
c6e9b0f5,
e9c6d9d9,
fc98f5b0,
fc66ff7f,
e938f54e,
c615d925,
9802b009,
66027f00,
38154e09,
15382525,
0266094e,
0298007f,
15c609b0,
38e925d9,
66fc4ef5,


>>> def table(max):
...     for i in range(0, max, 2):
...             print "0x%02x%02x%02x%02x," % (y(i+1,max), x(i+1,max), y(i,max), x(i,max))
... 
>>> table(32)
0x98fc7fff,
0xc6e9b0f5,
0xe9c6d9d9,
0xfc98f5b0,
0xfc66ff7f,
0xe938f54e,
0xc615d925,
0x9802b009,
0x66027f00,
0x38154e09,
0x15382525,
0x0266094e,
0x0298007f,
0x15c609b0,
0x38e925d9,
0x66fc4ef5,

