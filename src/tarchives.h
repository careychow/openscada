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

#ifndef TArchiveS_H
#define TArchiveS_H

#define  VER_ARH 1    //ArchiveS type modules version

#include <string>
#include <vector>

#include "terror.h"
#include "thd.h"
#include "tbds.h"
#include "tmodule.h"
#include "tconfig.h"
#include "tmessage.h"
#include "tgrpmodule.h"

using std::string;
using std::vector;

//================================================================
//=========== TArchiveMess ========================================
//================================================================
class TTipArchive;

class TArchiveMess : public TContr, public TConfig
{
    public:
	TArchiveMess(const string &name, TTipArchive *owner );
	virtual ~TArchiveMess();	
	
        string &name()   { return(m_name); }
        string &lName()  { return(m_lname); }			
	
	bool toStart() { return(m_start); }
	bool startStat(){ return(run_st); }

        void load( );
        void save( );			
        virtual void start(){ };
        virtual void stop(){ };		       	
	
        string &addr()   { return(m_addr); }
        int    &level()  { return(m_level); }
        void   categ( vector<string> &list );				

	virtual void put( vector<TMessage::SRec> &mess ){ };
        virtual void get( time_t b_tm, time_t e_tm, vector<TMessage::SRec> &mess, const string &category = "", char level = 0 ) { };
	
	TTipArchive &owner() { return(*m_owner); }
	
    protected:
	//================== Controll functions ========================
	void ctrStat_( XMLNode *inf );
	void ctrDinGet_( const string &a_path, XMLNode *opt );
	void ctrDinSet_( const string &a_path, XMLNode *opt );
	
    protected:
	string         &m_name;
	string         &m_lname;
	bool           &m_start;
	string         &m_addr;
	string         &m_cat_o;
	int            &m_level;

	bool           run_st;
	
    private:
	TTipArchive     *m_owner;
	
	//Request mess params
	time_t	m_beg, m_end;
	string	m_cat;
	int	m_lvl;

	static const char *o_name;
};

//================================================================
//=========== TArchiveVal =========================================
//================================================================
class TArchiveVal : public TContr, public TConfig
{
    public:
	TArchiveVal( const string &name, TTipArchive *owner );
	virtual ~TArchiveVal();

	string &name() { return(m_name); }
	
	TTipArchive &owner() { return(*m_owner); }
	
    protected:
	//================== Controll functions ========================
	void ctrStat_( XMLNode *inf );
	
    protected:
	string  &m_name;
	string  &m_bd;
	
    private:
	TTipArchive *m_owner;
	
	static const char *o_name;
};

//================================================================
//=========== TTipArchive =========================================
//================================================================

class TTipArchive: public TModule
{
    /** Public methods: */
    public:
    	TTipArchive( );
	virtual ~TTipArchive();
		
	// Avoid message list
	void messList( vector<string> &list ) 
	{ m_hd_mess.objList( list ); }	
	// Avoid stat
        bool messAvoid( const string &name )
        { return m_hd_mess.objAvoid(name); }
	// Add message archive	
	void messAdd( const string &name );
	// Del message archive
	void messDel( const string &name ) 
	{ delete (TArchiveMess *)m_hd_mess.objDel( name ); }
	// Mess archive
	AutoHD<TArchiveMess> messAt( const string &name )
	{ AutoHD<TArchiveMess> obj( name, m_hd_mess ); return obj; }
	
	// Avoid message list
	void valList( vector<string> &list )
	{ m_hd_val.objList( list ); }
	// Avoid stat
        bool valAvoid( const string &name )
        { return m_hd_val.objAvoid(name); }	
	// Add message archive
	void valAdd( const string &name );
	// Del message archive
        void valDel( const string &name )
	{ delete (TArchiveVal *)m_hd_val.objDel( name ); }	
	// Mess archive
	AutoHD<TArchiveVal> valAt( const string &name )
	{ AutoHD<TArchiveVal> obj( name, m_hd_val ); return obj; }		
	
    protected:
	//================== Controll functions ========================
	void ctrStat_( XMLNode *inf );
	void ctrDinGet_( const string &a_path, XMLNode *opt );
	void ctrDinSet_( const string &a_path, XMLNode *opt );
	AutoHD<TContr> ctrAt1( const string &br );
	
    /** Private atributes:: */
    private:
	virtual TArchiveMess *AMess(const string &name )
	{ throw TError("(%s) Message arhiv no support!",o_name); }
	virtual TArchiveVal  *AVal(const string &name )
	{ throw TError("(%s) Value arhiv no support!",o_name); }
	
    /** Private atributes:: */
    private:
        THD    m_hd_mess; 
        THD    m_hd_val; 
	
	static const char *o_name;
};

//================================================================
//================ TArchiveS ======================================
//================================================================
class TArchiveS : public TGRPModule
{
    /** Public methods: */
    public:
	TArchiveS( TKernel *app );
	~TArchiveS(  );

	int gmdVer( ) { return(VER_ARH); }
    	// Init All transport's modules
	void gmdInit( );
	void gmdStart( );
	void gmdStop( );	       	
	// Load/Reload all BD and update internal controllers structure!
	void loadBD( );
	// Update all BD from current to external BD.
	void saveBD( );

	void gmdCheckCommandLine( );
	void gmdUpdateOpt();	
	
	TBDS::SName &messB() { return(m_bd_mess); }
	TBDS::SName &valB()  { return(m_bd_val); }
	
	TElem &messE(){ return(el_mess); }
	TElem &valE() { return(el_val); }
	
    /** Privates: */
    private:
	string optDescr(  );

	static void *MessArhTask(void *param);
	
	void gmdDel( const string &name );    
	//================== Controll functions ========================
	void ctrStat_( XMLNode *inf );
	void ctrDinGet_( const string &a_path, XMLNode *opt );
	void ctrDinSet_( const string &a_path, XMLNode *opt );
	
    /** Private atributes: */
    private:	
	TBDS::SName	m_bd_mess, m_bd_val;	
	TElem  		el_mess, el_val;

	int       m_mess_per;       //Mmessage arhiving period
	pthread_t m_mess_pthr;
	bool      m_mess_r_stat;
	bool      m_mess_r_endrun;
	
	static const char *o_name;
};

#endif // TArchiveS_H