/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: options.c
     This file contains routines that are used to determine the options
     that were specified by the user
*/
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "options.h"
#include "global.h"

int power_10(int power)
{
	/*	Raise 10 to the power */
	register int i,p;

	p = 1;
	for (i = 1; i <= power; ++i)
		p = p * 10;
	return p;
}

float power_negative(int power)
{
     /*   Raise 10 to the negative power */

     register int i;
     float p;
     
     p = (float)1;
     for (i = 1; i<=power; i++)
          p = p * (float).1;
     return p;
}

float convert_array(int num[],int stack_size)
{
	/* Convert an array of characters to an integer */
	
	register int counter,c;
	float temp =(float)0.0;

	for (c=(stack_size-1), counter = 0; c>=0; c--, counter++)
     {
          if (num[c] == -1)
          /*   We are at the decimal point, convert to decimal
               less than 1
          */
          {
               counter = -1;
               temp = power_negative(stack_size - c - 1) * temp;
          }
          else 
               temp += power_10(counter) * num[c];
     }
			
	return(temp);
}

float get_options(int argc, char **argv, int *f, int *t, int *tr, int *group)
{
     char c;
     int count = 0;
     int buffer[MAX1];
     int next = 0;
     /*    tie variable */
     enum tie_options tie = FIRST;
     /*   triangulation variable */
     enum triangulation_options triangulate = WHOLE;
     /*   normal difference variable (in degrees) */
     float norm_difference = (float)360.0;
     /*   file-type variable */
     enum file_options file_type = ASCII;

     /*      User has the wrong number of options */
	if ((argc > 5) || (argc < 2))
	{
		printf("Usage: bands -[file_option][ties_option][triangulation_option][normal_difference] file_name\n");
		exit(0);
	}
	
     /* Interpret the options specified */
	while (--argc > 0 && (*++argv)[0] == '-')
     {
          /*   At the next option that was specified */
          next = 1;
          while (c = *++argv[0])
			switch (c)
		{
				case 'f': 
					/*      Use the first polygon we see. */
                         tie = FIRST;
					break;
				
				case 'r':
					/*      Randomly choose the next polygon */
                         tie = RANDOM;
					break;

				case 'a':
					/*      Alternate direction in choosing the next polygon */
                         tie = ALTERNATE;
					break;

				case 'l':
					/*      Use lookahead to choose the next polygon */
                         tie = LOOK;
					break;

				case 'q':
					/*  Try to reduce swaps */
                         tie = SEQUENTIAL;
					break;

				case 'p':
					/*      Use partial triangulation of polygons */
                         triangulate = PARTIAL;
					break;

				case 'w':
					/*      Use whole triangulation of polygons */
                         triangulate = WHOLE;
					break;

                    case 'b':
                         /*      Input file is in binary */
                         file_type = BINARY;
                         break;

                    case 'g':
                         /*   Strips will be grouped according to the groups in 
                              the data file. We will have to restrict strips to be
                              in the grouping of the data file.
                         */
                         *group = 1;
  		    
                         /*	Get each the value of the integer */
                         /*	We have an integer */
                    default:
                         if ((c >= '0') && (c <= '9'))
                         {
                              /*   More than one normal difference specified, use the last one */
                              if (next == 1)
                              {
                                   count = 0;
                                   next = 0;
                              }
                              buffer[count++] = ATOI(c);
                         }
                              /*   At the decimal point */
                         else if (c == '.')
                         {
                              /*   More than one normal difference specified, use the last one */
                              if (next == 1)
                              {
                                   count = 0;
                                   next = 0;
                              }
                              buffer[count++] = -1;
                         }
                         else 
                              break;
		}
     }
     /*   Convert the buffer of characters to a floating pt integer */
     if (count != 0) 
          norm_difference = convert_array(buffer,count);
     *f = file_type;
     *t = tie;
     *tr = triangulate;
     return norm_difference;
}
