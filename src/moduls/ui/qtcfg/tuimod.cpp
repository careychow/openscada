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
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>       

#include <qtextcodec.h>
#include <qapplication.h>

#include <tsys.h>
#include <tkernel.h>
#include <tmessage.h>
#include "qtcfg.h"
#include "tuimod.h"

//============ Modul info! =====================================================
#define MOD_ID      "QTCfg"
#define MOD_NAME    "QT configurator"
#define MOD_TYPE    "UI"
#define VER_TYPE    VER_UI
#define SUB_TYPE    "QT"
#define VERSION     "0.9.0"
#define AUTORS      "Roman Savochenko"
#define DESCRIPTION "QT based OpenSCADA Configurator."
#define LICENSE     "GPL"
//==============================================================================

extern "C"
{
    TModule::SAt module( int n_mod )
    {
    	TModule::SAt AtMod;

	if(n_mod==0)
	{
	    AtMod.id	= MOD_ID;
	    AtMod.type  = MOD_TYPE;
    	    AtMod.t_ver = VER_TYPE;
	}
	else
	    AtMod.id	= "";

	return( AtMod );
    }

    TModule *attach( const TModule::SAt &AtMod, const string &source )
    {
	QTCFG::TUIMod *self_addr = NULL;

	if( AtMod.id == MOD_ID && AtMod.type == MOD_TYPE && AtMod.t_ver == VER_TYPE )
	    self_addr = new QTCFG::TUIMod( source );       

	return ( self_addr );
    }    
}

using namespace QTCFG;

//==============================================================================
//================= QTCFG::TUIMod =============================================
//==============================================================================

TUIMod::TUIMod( string name ) : cfapp(NULL)
{
    mId		= MOD_ID;
    mName       = MOD_NAME;
    mType  	= MOD_TYPE;
    Vers      	= VERSION;
    Autors    	= AUTORS;
    DescrMod  	= DESCRIPTION;
    License   	= LICENSE;
    Source    	= name;
}

TUIMod::~TUIMod()
{
    if( run_st ) stop();
}

string TUIMod::modInfo( const string &name )
{
    if( name == "SubType" ) return(SUB_TYPE);
    else return( TModule::modInfo( name) );
}

void TUIMod::modInfo( vector<string> &list )
{
    TModule::modInfo(list);
    list.push_back("SubType");
}

string TUIMod::optDescr( )
{
    char buf[STR_BUF_LEN];

    snprintf(buf,sizeof(buf),I18N(
	"======================= The module <%s:%s> options =======================\n"
	"---------- Parameters of the module section <%s> in config file ----------\n\n"),
	MOD_TYPE,MOD_ID,MOD_ID);

    return(buf);
}

void TUIMod::modCheckCommandLine( )
{
    int next_opt;
    char *short_opt="h";
    struct option long_opt[] =
    {
	{"help"    ,0,NULL,'h'},
	{NULL      ,0,NULL,0  }
    };

    optind=opterr=0;
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': fprintf(stdout,optDescr().c_str()); break;
	    case -1 : break;
	}
    } while(next_opt != -1);
}

void TUIMod::modConnect(  )
{
    TModule::modConnect(  );
    
    //Set QT environments    
    QTextCodec::setCodecForCStrings( QTextCodec::codecForLocale () ); //codepage for QT across QString recode!
}

void TUIMod::start()
{
    if( run_st ) return;
    pthread_attr_t pthr_attr;
    
    pthread_attr_init(&pthr_attr);
    pthread_attr_setschedpolicy(&pthr_attr,SCHED_OTHER);
    pthread_create(&pthr_tsk,&pthr_attr,Task,this);
    pthread_attr_destroy(&pthr_attr);
    if( SYS->event_wait( run_st, true, string(MOD_ID)+": The configurator is starting....",5) )
       	throw TError("%s: The configurator no started!",MOD_ID);   
}

void TUIMod::stop()
{
    if( run_st)
    {
	if( cfapp != NULL ) cfapp->close();
	if( SYS->event_wait( run_st, false, string(MOD_ID)+": The configurator is stoping....",5) )
	    throw TError("%s: The configurator no stoped!",MOD_ID);   
	pthread_join(pthr_tsk,NULL);
    }	
}

void *TUIMod::Task( void *CfgM )
{
    TUIMod *Cfg = (TUIMod *)CfgM;

#if OSC_DEBUG
    Cfg->mPut("DEBUG",MESS_DEBUG,"Thread <%d>!",getpid() );
#endif    
    
    Cfg->run_st = true;

    QApplication app( (int)SYS->argc,(char **)SYS->argv );
    Cfg->cfapp = new ConfApp(Cfg,NULL);
    Cfg->cfapp->show();
    app.connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );    
    app.exec();
    
    Cfg->cfapp = NULL;
    Cfg->run_st = false;
    
    return(NULL);
}

