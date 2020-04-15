
/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

void hexdump(unsigned char * data, size_t len, const char *name)
{
    unsigned int i;
    size_t r,c;

    if (!data)
        return;

    for (r=0,i=0; r<(len/16+(len%16!=0)); r++,i+=16)
    {
        if (!r)
            printf("<clearing %s>\n", name ? name : ""); /* should not be null! */
        printf("\t%04X:   ",i); /* location of first byte in line */

        for (c=i; c<i+8; c++) /* left half of hex dump */
            if (c<len)
                printf("%02X ",((unsigned char const *)data)[c]);
            else
                printf("   "); /* pad if short line */

        printf("  ");

        for (c=i+8; c<i+16; c++) /* right half of hex dump */
            if (c<len)
                printf("%02X ",((unsigned char const *)data)[c]);
            else
                printf("   "); /* pad if short line */

        printf("   ");

        for (c=i; c<i+16; c++) /* ASCII dump */
            if (c<len)
                if (((unsigned char const *)data)[c]>=32 &&
                        ((unsigned char const *)data)[c]<127)
                    printf("%c",((char const *)data)[c]);
                else
                    printf("."); /* put this for non-printables */
            else
                printf(" "); /* pad if short line */

        printf("\n");
    }
}
