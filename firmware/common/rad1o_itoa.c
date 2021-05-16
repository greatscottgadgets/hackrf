#include "rad1o_itoa.h"

/* flags:
   F_ZEROS  Print with leading zeros
   F_LONG   Print with leading spaces
   F_SPLUS  Prefix '+' on positive number
   F_SSPACE Prefix ' ' on positive numbers
   F_HEX    Print as (fixed-width) hex number
 */

#define LEN 11 /* Maximum length we support */
const char* IntToStr(int num, unsigned int mxlen, char flag){
    static char s[LEN+1];
    unsigned int len;
    s[LEN]=0;
    char neg=0;

    if(flag&F_HEX){
        unsigned int hex=num;
        for (len=(LEN-1);len>=(LEN-mxlen);len--){
            s[len]=(hex%16)+'0';
            if(s[len]>'9')
                s[len]+='A'-'9'-1;
            hex/=16;
        };
        len++;
    }else{
        if(num<0){
            num=-num;
            neg=1;
        };

        for (len=(LEN-1);len>=(LEN-mxlen);len--){
            s[len]=(num%10)+'0';
            num/=10;
        };
        len++;

        if(!(flag&F_LONG)){
            while(s[len]=='0' && len < (LEN-1))
                len++;
        }else if(!(flag&F_ZEROS)){
            int x=len;
            while(s[x]=='0' && x < (LEN-1)){
                s[x]=' ';
                x++;
            };
        }

        if(neg==1){
            len--;
            s[len]='-';
        }else if(flag&F_SPLUS){
            len--;
            s[len]='+';
        }else if(flag&F_SSPACE){
            len--;
            s[len]=' ';
        };
    };
    return &s[len];
}
#undef LEN
