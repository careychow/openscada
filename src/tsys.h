/***************************************************************************
 *   Copyright (C) 2004 by Roman Savochenko                                *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef TSYS_H
#define TSYS_H

#define TO_FREE         NULL // Object free
#define STR_BUF_LEN     2000 // Len of string buffers (no string class)
#define STD_WAIT_DELAY  100 // Standart wait dalay (ms)

#define C_INT_DEC 0
#define C_INT_OCT 1
#define C_INT_HEX 2

#include <semaphore.h>
#include <stdio.h>

#include <string>
#include <vector>

#include "tcntrnode.h"

#define __func__ __PRETTY_FUNCTION__

using std::string;
using std::vector;

class TKernel;

//======================================================================================
//====================== TSYS ==========================================================
//======================================================================================


class TSYS : public TCntrNode 
{
    // Public methods:
    public:
	TSYS( int argi, char ** argb, char **env );
	~TSYS(  );

	int start(  );		
	
        XMLNode *cfgNode();	// Config file's functions
	
        // Programms options
	string station() { return(m_station); }	
	string user() { return(m_user); }               //Run user name 
	string cfgFile() { return(m_confFile); }
	
	// Get option from generic config file and update data from XML config.
	void updateOpt();
	// Update comand line option
	void checkCommandLine( );
	// Print comand line options!
	string optDescr( );
	// Set task title
	//void SetTaskTitle(const char *fmt, ...);
	
        //================== Kernel functions ========================
        void kList( vector<string> &list )	{ chldList(m_kern,list); }
        bool kAvoid( const string &name )	{ return chldAvoid(m_kern,name); }
	void kAdd( const string &name );
	void kDel( const string &name );
	AutoHD<TKernel> kAt( const string &name )
	{ return chldAt(m_kern,name); }           

	static void sighandler( int signal );
	
        // Short time dimensions
        unsigned long long sysClk( ){ return m_sysclc; }
        unsigned long long shrtCnt( )
        {
    	    unsigned long cntl, cnth;
    	    asm volatile("rdtsc; movl %%eax,%0; movl %%edx,%1;":"=r"(cntl),"=r"(cnth)::"%eax","%edx");
	    return ((unsigned long long)cnth<<32)+cntl;	
        }										    
	
    // Public static methods:
    public:
        //========= System function ====================
    	// Convert path to absolut name
	static string fNameFix( const string &fname );
	// Convert value to string
        static string int2str( int val, char view = C_INT_DEC );
        static string real2str( double val );	
	// Wait event with timeout support
	static bool eventWait( bool &m_mess_r_stat, bool exempl, const string &loc, time_t time = 0 );
	
	//Separated string parse
        static string strSepParse( const string &path, int level, char sep );		
	
    public:
	// A comand line seting counter.
	const int argc;
	// A comand line seting buffer.
	const char **argv;
	// A system environment.
	const char **envp;							     

    private:
	void cfgFileScan( bool first = false );
        //================== Controll functions ========================
	void     ctrStat_( XMLNode *inf );
	void     ctrDinGet_( const string &a_path, XMLNode *opt );
	void     ctrDinSet_( const string &a_path, XMLNode *opt );
	AutoHD<TCntrNode> ctrAt1( const string &br );
	/** Private atributes: */
    
    private:    
	string m_user;	// A owner user name!
	string m_confFile;
	string m_station;
	unsigned m_cr_f_perm;
	unsigned m_cr_d_perm;
	//OpenScada and station XML config node
	XMLNode root_n;
	XMLNode *stat_n;
	
	int    	stop_signal;

	int	m_kern;

	//Request mess params
	time_t	m_beg, m_end;
	string	m_cat;
	int	m_lvl;
	unsigned long long m_sysclc;

	static const char *o_name;    
};

struct SSem
{
    bool  use;          // using flag
    bool  del;          // deleting flag    
    sem_t sem;          // semafor id 
    int   rd_c;         // readers counter
};

class ResAlloc 
{
    public: 
	ResAlloc( unsigned id );
	ResAlloc( unsigned id, bool write );
	~ResAlloc( );

	void request( bool write = false, long tm = 0 );
	void release();
	
	// Static metods
	static unsigned resCreate( unsigned val = 1 );
	static void resDelete( unsigned res );
    
	static void resRequestW( unsigned res, long tm = 0 ); // Write request
        static void resReleaseW( unsigned res );              // Write release
	static void resRequestR( unsigned res, long tm = 0 ); // Read request
	static void resReleaseR( unsigned res );              // Read release
	
    private:
	int   m_id;     //
	char  m_wr;     //0x01 - alloc; 0x02 - write
	
	static vector<SSem>  sems;
	
	static const char *o_name;    
};


extern TSYS *SYS;

#endif // TSYS_H
