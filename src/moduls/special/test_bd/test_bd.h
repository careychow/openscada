#ifndef TEST_BD_H
#define TEST_BD_H

#include <tspecials.h>

namespace BDTest
{
    class TTest: public TSpecial
    {
	public:
	    TTest( string name );
	    ~TTest();

	    void start();

	    void mod_CheckCommandLine( );
	    void mod_UpdateOpt();
	private:
	    void pr_opt_descr( FILE * stream );	
	    string mod_info( const string name );
	    void   mod_info( vector<string> &list );
	private:
	    static SExpFunc ExpFuncLc[];	
    };    
}

#endif //TEST_BD_H