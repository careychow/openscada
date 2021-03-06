
//OpenSCADA system file: resalloc.h
/***************************************************************************
 *   Copyright (C) 2003-2010 by Roman Savochenko                           *
 *   rom_as@oscada.org, rom_as@fromru.com                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef RESALLOC_H
#define RESALLOC_H

#include <features.h>
#include <pthread.h>

#include <string>

using std::string;

namespace OSCADA
{

//********************************************
//* Resource object                          *
//********************************************
class Res
{
    public:
	Res( );
	~Res( );

	void resRequestW( unsigned short tm = 0 );	// Write request, tm in milliseconds
	bool resTryW( );
	void resRequestR( unsigned short tm = 0 );	// Read request, tm in milliseconds
	bool resTryR( );
	void resRelease( );				// Release

    private:
	pthread_rwlock_t	rwc;
#if !__GLIBC_PREREQ(2,4)
	pthread_t		wThr;
#endif
};

//********************************************
//* Automatic resource allocator/deallocator *
//********************************************
class ResAlloc
{
    public:
	//Methods
	ResAlloc( Res &rid );
	ResAlloc( Res &rid, bool write, unsigned short tm = 0 );
	~ResAlloc( );

	void request( bool write = false, unsigned short tm = 0 );
	void release( );

    private:
	//Attributes
	Res	&mId;
	bool	mAlloc;
};

//********************************************
//* String+resource for			     *
//********************************************
class ResString
{
    public:
	//Methods
	explicit ResString( const string &vl = "" );
	~ResString( );

	ResString &operator=( const string &val );
	operator string( )		{ return getVal(); }

	size_t size( );
	bool   empty( );

	void setVal( const string &vl );
	string getVal( );
	const string &getValRef( )	{ return str; }

    private:
	//Attributes
	pthread_mutex_t	mRes;
	string	str;
};

}

#endif // RESALLOC_H
