/*
** Copyright (C) 2002 Erik de Castro Lopo <erikd@zip.com.au>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/



#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>

#include	<float_cast.h>

#include	<sndfile.h>

#include	"utils.h"

#define	BUFFER_SIZE		(10000)

#ifndef		M_PI
#define		M_PI		3.14159265358979323846264338
#endif

static  void	dwvw_test (int format, int bit_width) ;

int
main (void)
{
	dwvw_test (SF_FORMAT_RAW | SF_FORMAT_DWVW_12, 12) ;
	dwvw_test (SF_FORMAT_RAW | SF_FORMAT_DWVW_16, 16) ;
	dwvw_test (SF_FORMAT_RAW | SF_FORMAT_DWVW_24, 24) ;

	return 0 ;
} /* main */

static  void
dwvw_test (int format, int bit_width)
{	static	int		write_buf [BUFFER_SIZE] ;
	static	int		read_buf  [BUFFER_SIZE] ;

	SNDFILE	*file ;
	SF_INFO sfinfo ;
	char	*filename ;
	double 	value ;
	int		k, bit_mask ;
	
	srand (123456) ;

	/* Only want to grab the top bit_width bits. */
	bit_mask = (-1 << (32 - bit_width)) ;

	filename = "test.raw" ;
	
	printf ("    dwvw_test : %d bits  ... ", bit_width) ;
	fflush (stdout) ;

	sfinfo.format      = format ;
	sfinfo.samplerate  = 44100 ;
	sfinfo.frames     = -1 ; /* Unknown! */
	sfinfo.channels    = 1 ;
	
	if (! (file = sf_open (filename, SFM_WRITE, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
		
	/* Generate random.frames. */
	k = 0 ;
	for (k = 0 ; k < BUFFER_SIZE / 2 ; k++)
	{	value = 0x7FFFFFFF * sin (123.0 / sfinfo.samplerate * 2 * k * M_PI) ;
		write_buf [k] = bit_mask & lrint (value) ;
		} ;

	for ( ; k < BUFFER_SIZE ; k++)
		write_buf [k] = bit_mask & ((rand () << 11) ^ (rand () >> 11)) ;

	sf_write_int (file, write_buf, BUFFER_SIZE) ;
	sf_close (file) ;
	
	if (! (file = sf_open (filename, SFM_READ, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
		
	if ((k = sf_read_int (file, read_buf, BUFFER_SIZE)) != BUFFER_SIZE)
	{	printf ("Error (line %d) : Only read %d/%d.frames.\n", __LINE__, k, BUFFER_SIZE) ;
		exit (1) ;
		}
	
	for (k = 0 ; k < BUFFER_SIZE ; k++)
	{	if (read_buf [k] != write_buf [k])
		{	printf ("Error (line %d) : %d != %d at position %d/%d\n", __LINE__, 
				write_buf [k] >> (32 - bit_width), read_buf [k] >> (32 - bit_width), 
				k, BUFFER_SIZE) ;
			oct_save_int    (write_buf, read_buf, BUFFER_SIZE) ;
			exit (1) ;
			} ;
		} ;

	sf_close (file) ;

	printf ("ok\n") ;
	unlink (filename) ;

	return ;
} /* dwvw_test */
