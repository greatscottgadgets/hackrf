#ifndef __picobool_h_INCLUDED__
#define __picobool_h_INCLUDED__

// this file provides a bool type which might or might not be available on your platform

#ifndef __cplusplus                     // C++: use standard bool
    #ifndef __STDC_VERSION__            // Pre-C99: define bool manually
        typedef unsigned char bool;
        #define true 1
        #define false 0
    #elif __STDC_VERSION__ < 199901L    // C90/C95: define bool manually
        typedef unsigned char bool;
        #define true 1
        #define false 0
    #else                               // C99+: use standard bool
        #include <stdbool.h>
    #endif
#endif

#endif // __picobool_h_INCLUDED__
