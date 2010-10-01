/*

priiloader/preloader 0.30 - A tool which allows to change the default boot up sequence on the Wii console

Copyright (C) 2008-2009  crediar

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


*/

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <string.h>
#include <malloc.h>
#include <vector>
#include <unistd.h>

#include "gecko.h"
#include "hacks.h"
#include "settings.h"
#include "error.h"
#include "font.h"

std::vector<hack> hacks;
u32 *states=NULL;
extern u8 error;

unsigned int foff=0;
char *GetLine( char *astr, unsigned int len )
{
	if( foff >= len )
	{
		return NULL;
	}

	unsigned int llen = 0;

	if( strstr(	astr+foff, "\n" ) == NULL )
	{
		if( strstr( astr+foff, "\0" ) == NULL )
		{
			gprintf("failed to find the newline&zero byte @ 0x%x\n",(vu32)(astr+foff));
			return 0;
		}
		llen = strstr( astr+foff, "\0" ) - (astr+foff);
		if( llen == 0 )
		{
			gprintf("failed to find the newline\\zero byte @ 0x%x\n",(vu32)(astr+foff));
			return NULL;
		}
	}
	else
	{
		llen = strstr( astr+foff , "\n" ) - (astr+foff);
		if( llen == 0 )
		{
			gprintf("failed to find the newline @ 0x%x\n",(vu32)(astr+foff));
			return NULL;
		}
	}

	//printf("%d\n", llen );

	char *lbuf = (char*)malloc( llen );
	if( lbuf == NULL )
	{
		gprintf("malloc failed for lbuf\n",llen);
		error = ERROR_MALLOC;
		return 0;
	}
	memset(lbuf,0,llen);
	//silly fix for linux newlines...
	if(strstr( astr, "\r\n" ) != NULL )
	{
		//must be a windows newline D:<
		memcpy( lbuf, astr+foff, llen -1);
		lbuf[llen-1] = 0;
	}
	else
	{
		//linux it is
		memcpy( lbuf, astr+foff, llen);
		lbuf[llen] = 0;
	}
	foff+=llen+1;
	return lbuf;
}
u32 LoadHacks( bool Force_Load_Nand )
{
	if( hacks.size() ) //Hacks already loaded
	{
		hacks.clear();
		if(states)
		{
			free_pointer(states);
		}
        foff=0;
	}

	bool mode = true;
	s32 fd=0;
	char *buf=NULL;
	fstats *status = NULL;
	unsigned int size=0;
	FILE* in = NULL;
	if(!Force_Load_Nand)
	{
		in = fopen ("fat:/hacks.ini","rb");
	}
	if( !in )
	{
		fd = ISFS_Open("/title/00000001/00000002/data/hacks.ini", 1 );
		if( fd < 0 )
		{
			gprintf("LoadHacks : Hacks.ini not on FAT or Nand. ISFS_Open error %d\n",fd);
			return 0;
		} 
		mode = false;
	}
	if( mode )	//read file from FAT
	{
		//read whole file in
		fseek( in, 0, SEEK_END );
		size = ftell(in);
		fseek( in, 0, 0);

		if( size == 0 )
		{
			PrintFormat( 1, ((640/2)-((strlen("Error \"hacks.ini\" is 0 byte!"))*13/2))>>1, 208, "Error \"hacks.ini\" is 0 byte!");
			sleep(5);
			return 0;
		}

		buf = (char*)memalign( 32, (size+31)&(~31));
		if( buf == NULL )
		{
			error = ERROR_MALLOC;
			return 0;
		}
		memset( buf, 0, (size+31)&(~31) );
		if(fread( buf, sizeof( char ), size, in ) != size )
		{
			free_pointer(buf);
			PrintFormat( 1, ((640/2)-((strlen("Error reading \"hacks.ini\""))*13/2))>>1, 208, "Error reading \"hacks.ini\"");
			sleep(5);
			return 0;
		}
		fclose(in);

	} else {	//read file from NAND

		status = (fstats *)memalign( 32, (sizeof( fstats )+31)&(~31) );
		ISFS_GetFileStats( fd, status);
		size = status->file_length;
		free_pointer(status);
		buf = (char*)memalign( 32, (size+31)&(~31) );
		if( buf == NULL )
		{
			error = ERROR_MALLOC;
			return 0;
		}
		memset( buf, 0, (size+31)&(~31) );
		ISFS_Read( fd, buf, size );
		ISFS_Close( fd );
	}

	buf[size] = 0;

//read stuff into a nice struct
	char *str=buf;
	char *lbuf=NULL;
	unsigned int line = 1;

	lbuf = GetLine( str, size );
	if( lbuf == NULL )
	{
		//printf("Error 1: syntax error : expected EOF at line %d\n", line );
		PrintFormat( 1, ((640/2)-((strlen("Syntax error : expected EOF at line   "))*13/2))>>1, 208, "Syntax error : expected EOF at line %d", line);
		free_pointer(buf);
		hacks.clear();
		sleep(5);
		return 0;
	}

	while(1)
	{
		if( lbuf == NULL )
			break;

		//printf("\"%s\"\n", lbuf );

		if( strstr( lbuf, "[" ) == NULL )
		{
			//printf("Error 2: syntax error : missing '[' before '\\n' at line %d\n", line );
			PrintFormat( 1, ((640/2)-((strlen("Syntax error : missing '[' before 'n' at line   "))*13/2))>>1, 208, "Syntax error : missing '[' before 'n' at line %d", line);
			hacks.clear();
			sleep(5);
			return 0;
		}

		char *s = strtok( lbuf+(strstr( lbuf, "[" )-lbuf) + 1, "]");
		if( s == NULL )
		{
			//printf("Error 3: syntax error : missing ']' before '\\n' at line %d\n", line );
			PrintFormat( 1, ((640/2)-((strlen("Syntax error : missing ']' before 'n' at line   "))*13/2))>>1, 208, "Syntax error : missing ']' before 'n' at line %d", line);
			hacks.clear();
			sleep(5);
			return 0;
		}

		//printf("\"%s\"\n", s );

		hacks.resize( hacks.size() + 1 );
		hacks[hacks.size()-1].desc = new char[strlen(s)+1];
		memcpy( hacks[hacks.size()-1].desc, s, strlen(s)+1 );
		hacks[hacks.size()-1].desc[strlen(s)] = 0;
		free_pointer(lbuf);
		line++;
		lbuf = GetLine( str, size );
		if( lbuf == NULL )
		{
			//printf("Error 4: syntax error : expected EOF at line %d\n", line );
			PrintFormat( 1, ((640/2)-((strlen("Syntax error : expected EOF at line   "))*13/2))>>1, 208, "Syntax error : expected EOF at line %d", line);
			hacks.clear();
			sleep(5);
			return 0;
		}

		if( strcmp( lbuf, "version" ) != 0 )
		{
			if( strstr( lbuf, "=" ) ==  NULL )
			{
				//printf("Error 5: syntax error : missing '=' before '\\n' at line %d\n", line );
				PrintFormat( 1, ((640/2)-((strlen("Syntax error : missing '=' before 'n' at line   "))*13/2))>>1, 208, "Syntax error : missing '=' before 'n' at line %d", line);
				hacks.clear();
				sleep(5);
				return 0;
			}
			s = strtok( lbuf+(strstr( lbuf, "=" )-lbuf) + 1, "\n");
			if( s == NULL )
			{
				//printf("Error 5: syntax error : missing value before '\\n' at line %d\n", line );
				PrintFormat( 1, ((640/2)-((strlen("Syntax error : missing value before 'n' at line   "))*13/2))>>1, 208, "Syntax error : missing value before 'n' at line %d", line);
				hacks.clear();
				sleep(5);
				return 0;
			}
		} else {
			//printf("Error 6: syntax error : expected 'version' before '\\n' at line %d\n", line );
			PrintFormat( 1, ((640/2)-((strlen("Syntax error : expected 'version' before 'n' at line   "))*13/2))>>1, 208, "Syntax error : expected 'version' before 'n' at line %d", line);
			hacks.clear();
			sleep(5);
			return 0;
		}

		//printf("\"%s\"\n", s );

		hacks[hacks.size()-1].version = atoi( s );

		if( hacks[hacks.size()-1].version == 0 )
		{
			//printf("Error 12: syntax error : version is zero at line %d\n", line );
			PrintFormat( 1, ((640/2)-((strlen("Error : version is zero at line   "))*13/2))>>1, 208, "Error : version is zero at line %d", line);
			hacks.clear();
			sleep(5);
			return 0;
		}
		free_pointer(lbuf);

		while(1)
		{
			line++,
			lbuf = GetLine( str, size );
			if( lbuf == NULL )
				break;

			if( memcmp( lbuf, "offset", 6 ) == 0 )
			{
				//printf("offsets:");
				if( strstr( lbuf, "=" ) ==  NULL )
				{
					//printf("Error 5: syntax error : missing '=' before '\\n' at line %d\n", line );
					PrintFormat( 1, ((640/2)-((strlen("Syntax error : missing '=' before 'n' at line   "))*13/2))>>1, 208, "Syntax error : missing '=' before 'n' at line %d", line);
					hacks.clear();
					sleep(5);
					return 0;
				}
				s = strtok( lbuf+(strstr( lbuf, "=" )-lbuf) + 1, ",\n");
				if( s == NULL )
				{
					//printf("Error 7: syntax error : expected value before '\\n' at line %d\n", line );
					PrintFormat( 1, ((640/2)-((strlen("Syntax error : expected value before 'n' at line   "))*13/2))>>1, 208, "Syntax error : expected value before 'n' at line %d", line);
					hacks.clear();
					sleep(5);
					return 0;
				}
				
				do{
					hacks[hacks.size()-1].offset.resize( hacks[hacks.size()-1].offset.size() + 1 );
					sscanf( s, "%x", &hacks[hacks.size()-1].offset[hacks[hacks.size()-1].offset.size()-1] );
					//printf("%08X", hacks[hacks.size()-1].offset[hacks[hacks.size()-1].offset.size()-1] );

				}while( (s = strtok( NULL,",\n")) != NULL);
				//printf("\n");

			} else if ( memcmp( lbuf, "value", 5 ) == 0 ) {

				//printf("value:");
				if( strstr( lbuf, "=" ) ==  NULL )
				{
					//printf("Error 5: syntax error : missing '=' before '\\n' at line %d\n", line );
					PrintFormat( 1, ((640/2)-((strlen("Syntax error : missing '=' before 'n' at line   "))*13/2))>>1, 208, "Syntax error : missing '=' before 'n' at line %d", line);
					hacks.clear();
					sleep(5);
					return 0;
				}
				s = strtok( lbuf+(strstr( lbuf, "=" )-lbuf) + 1, ",\n");
				if( s == NULL )
				{
					//printf("Error 8: syntax error : expected value before '\\n' at line %d\n", line );
					PrintFormat( 1, ((640/2)-((strlen("Syntax error : expected value before 'n' at line   "))*13/2))>>1, 208, "Syntax error : expected value before 'n' at line %d", line);
					hacks.clear();
					sleep(5);
					return 0;
				}
				
				do{
					hacks[hacks.size()-1].value.resize( hacks[hacks.size()-1].value.size() + 1 );
					sscanf( s, "%x", &hacks[hacks.size()-1].value[hacks[hacks.size()-1].value.size()-1] );
					//printf("%08X", hacks[hacks.size()-1].value[hacks[hacks.size()-1].value.size()-1] );

				}while( (s = strtok( NULL,",\n")) != NULL);
				//printf("\n");

			} else if( hacks[hacks.size()-1].value.size() > 0 && hacks[hacks.size()-1].offset.size() > 0 ) {
				if( hacks[hacks.size()-1].value.size() > hacks[hacks.size()-1].offset.size() )
				{
					//printf("Error 9: syntax error : found more values than offsets at line %d\n", line );
					PrintFormat( 1, ((640/2)-((strlen("Error : found more values than offsets at line   "))*13/2))>>1, 208, "Error : found more values than offsets at line %d", line);
					hacks.clear();
					sleep(5);
					return 0;
				} else if ( hacks[hacks.size()-1].value.size() < hacks[hacks.size()-1].offset.size() ) {

					//printf("Error 10: syntax error : found more offsets than values at line %d\n", line );
					PrintFormat( 1, ((640/2)-((strlen("Error : found more offsets than values at line   "))*13/2))>>1, 208, "Error : found more offsets than values at line %d", line);
					hacks.clear();
					sleep(5);
					return 0;
				}
				break;
			} else {
				//printf("Error 11: syntax error : expected 'offset' or 'value' before '\\n' at line %d\n", line );
				PrintFormat( 1, ((640/2)-((strlen("Syntax error : expected 'offset' or 'value' before 'n' at line   "))*13/2))>>1, 208, "Syntax error : expected 'offset' or 'value' before 'n' at line %d", line);
				hacks.clear();
				sleep(5);
				return 0;
			}
			free_pointer(lbuf);
		}
	}
	free_pointer(buf);

	if(hacks.size()==0)
		return 0;

	//load hack states (on/off)
	if ( states == NULL )
	{
		states = (u32*)memalign( 32, ((sizeof( u32 ) * hacks.size())+31)&(~31) );
		if( states == NULL )
		{
			error = ERROR_MALLOC;
			return 0;
		}
	}
	memset( states, 0, ((sizeof( u32 ) * hacks.size())+31)&(~31) );

	fd = ISFS_Open("/title/00000001/00000002/data/hacks_s.ini", 1|2 );

	if( fd < 0 )
	{
		//file not found create a new one
		if(ISFS_CreateFile("/title/00000001/00000002/data/hacks_s.ini", 0, 3, 3, 3)<0)
			return 0;

		fd = ISFS_Open("/title/00000001/00000002/data/hacks_s.ini", 1|2 );
		if( fd < 0 )
			return 0;

		ISFS_Write( fd, states, sizeof( u32 ) * hacks.size() );

		ISFS_Seek( fd, 0, 0 );
	}

	status = (fstats *)memalign( 32, (sizeof(fstats)+31)&(~31) );
	if( status == NULL )
	{
		error = ERROR_MALLOC;
		return 0;
	}
	memset( status, 0, (sizeof(fstats)+31)&(~31) );

	if(ISFS_GetFileStats( fd, status)<0)
	{
		free_pointer(status);
		return 0;
	}

	u8 *fbuf = (u8 *)memalign( 32, (status->file_length+31)&(~31) );
	if( fbuf == NULL )
	{
		free_pointer(status);
		error = ERROR_MALLOC;
		return 0;
	}
	memset( fbuf, 0, (status->file_length+31)&(~31) );

	if(ISFS_Read( fd, fbuf, status->file_length )<0)
	{
		free_pointer(status);
		free_pointer(fbuf);
		return 0;
	}

	if( sizeof( u32 ) * hacks.size() < status->file_length )
		memcpy( states, fbuf, sizeof( u32 ) * hacks.size() );
	else
		memcpy( states, fbuf, status->file_length );

	free_pointer(fbuf);
	free_pointer(status);
	ISFS_Close( fd );

	//Set all hacks from other regions to 0
	unsigned int sysver = GetSysMenuVersion();
	for( u32 i=0; i < hacks.size(); ++i)
	{
		if( hacks[i].version != sysver )
		{
			states[i] = 0;
		}
	}
	return 1;
}
