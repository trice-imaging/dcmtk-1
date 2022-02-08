#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define OFTEST_OFSTD_ONLY
#include "dcmtk/ofstd/oftest.h"
#include "dcmtk/ofstd/ofoption.h"

struct test{test(int,bool){}bool doit(){return OFTrue;}};

STD_NAMESPACE istream& operator>>( STD_NAMESPACE istream& in, const test& ) { in.setstate(STD_NAMESPACE istream::failbit); return in; }

OFTEST(ofstd_optional)
{
    OFoptional<int> o0( 3 ), o1, o2( OFnullopt );

    OFCHECK( o0 && *o0 == 3 );

    OFCHECK( !o1 && !o2 );

    OFswap( o0, o1 );

    OFCHECK( !o0 && o1 && *o1 == 3 );

    o2 = 7;

    o1.swap( o2 );

    OFCHECK( o1 && o2 );
    OFCHECK( *o1 == 7 && *o2 == 3 );
    OFCHECK( o0 < o2 && o2 <= o1 );
    OFCHECK( o2 != o1 );
    o1 = o2;
    OFCHECK( o1 == o2 );
    OFCHECK( o0 == OFnullopt );
    OFCHECK( o2 == 3 );
    OFCHECK( o1 >= 1 );

    OFStringStream s( "42" );
    o1 = OFnullopt;
    OFCHECK( !o1 );
    OFCHECK( !( s >> o1 ).fail() );
    OFCHECK( o1 && *o1 == 42 );
    s.clear();
    s.str("");
    OFCHECK( s.str().empty() );
    s << o1;
    OFCHECK( s.str() == "42" );
    o1 = OFnullopt;
    s.clear();
    s.str("");
    s << o1;
    OFCHECK( s.str() == "nullopt" );
    s.clear();
    s.str("23");
    s >> o1;
    OFCHECK( o1 && *o1 == 23 );

#ifdef DCMTK_USE_CXX11_STL
    OFoptional<test> o3( 2, OFFalse );
#else // C++11
    OFoptional<test> o3( test( 2, OFFalse ) );
#endif // NOT C++11
    OFCHECK( o3 && o3->doit() );

    s.clear();
    s.str( "test" );
    // if everything works, this sets the failbit
    OFCHECK( (s >> o3).fail() );

    o3 = OFnullopt;
    OFCHECK( !o3 );

    s.clear();
    OFCHECK( !(s >> o3).fail() ); // no fail, since no-call
    OFCHECK( !o3 );

#ifdef DCMTK_USE_CXX11_STL
    o3.emplace( 0, OFFalse );
#else // C++11
    o3 = test( 0, OFFalse );
#endif // NOT C++11

    OFCHECK( o3 && o3->doit() );

    int i = 8;
    OFoptional<int&> o4;

    OFCHECK( !o4 );
    o4 = i;
    OFCHECK( o4 );
    *o4 = 19;
    OFCHECK( i == 19 );
}
