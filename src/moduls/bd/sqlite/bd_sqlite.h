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

#ifndef BD_SQLITE
#define BD_SQLITE

#include <string>
#include <vector>
#include <tmodule.h>
#include <tbds.h>

using std::string;
using std::vector;

namespace BDSQLite
{
    class MBD;

    class MTable : public TTable
    {
	public:
	    MTable(MBD *bd,string name, bool create);
	    ~MTable(  );

	    //Fields
	    bool fieldSeek( int row, TConfig &cfg );
	    void fieldGet( TConfig &cfg );
	    void fieldSet( TConfig &cfg );
	    void fieldDel( TConfig &cfg );
	    
	private:
	    void fieldFix( TConfig &cfg );
    
	private:
	    MBD  *m_bd;
	    bool my_trans;
    };

    class MBD : public TBD
    {
	friend class MTable;
	public:
	    MBD( string name, bool create );
	    ~MBD(  );

	    TTable *openTable( const string &name, bool create );
	    void delTable( const string &name );
	    
    	protected:
	    void sqlReq( const string &req, vector< vector<string> > *tbl = NULL );
			
	protected:	
	    sqlite3 *m_db;
    	    bool openTrans;
    };

    class BDMod: public TTipBD
    {
	public:
	    BDMod( string name );
	    ~BDMod();
	
	    TBD *openBD( const string &name, bool create );
	    void delBD( const string &name );
	    
	    void modLoad( );
	    
	private:
	    string optDescr( );
    };
}

#endif // BD_SQLITE
