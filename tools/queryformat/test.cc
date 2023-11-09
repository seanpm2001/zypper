#include <iostream>

#include "Parser.h"

#include <zypp/ResPool.h>
#include <zypp/base/Exception.h>
#include <zypp/misc/DefaultLoadSystem.h>

using namespace zypp;

//////////////////////////////////////////////////////////////////
namespace zypp::qf
{
  template <class ItemT>
  using AttrRenderer=std::function<void(const ItemT &,std::ostream &)>;

  namespace
  {
    struct DummyDataGetter
    {
      template <class ItemT>
      std::string_view operator()( const Tag & tag_r, ItemT && item_r ) const
      { return tag_r.name; }
    };

    template <class ItemT>
    struct DataGetter
    {
      std::string_view operator()( const Tag & tag_r, ItemT && item_r ) const
      { return tag_r.name; }
    };

#if 0
    // DataT getter( item_r )
    // "NAME" -> DataGetter<DataT>
    ARCH
    NAME
    RELEASE
    REQUIREFLAGS
    REQUIRENAME
    REQUIRENEVRS
    REQUIRES
    REQUIREVERSION
    VERSION

    template <class ItemT>
    Attrib getAttr( const ItemT & item_r, const String & attr_r )

    struct Attrib
    {
      template <class ItemT>
      Atrttib

    };


#endif
  } // namespace

  template <class ItemT, class DataGetterT=DummyDataGetter>
  struct Renderer
  {
    Renderer( Format && format_r )
    : _format { std::move(format_r) }
    {}

    Renderer( std::string_view qf_r )
    : _format { qf::parse( qf_r ) }
    {}

    void operator()( const ItemT & item_r ) const
    { operator()( item_r, DOUT ); }

    void operator()( const ItemT & item_r, std::ostream & str ) const
    { render( _format, item_r, str ); }

  private:
    // Format
    void render( const Format & format_r, const ItemT & item_r, std::ostream & str, unsigned idx_r=0 ) const
    {
      for ( const auto & tok : format_r.tokens ) {
        switch ( tok->_type )
        {
          case TokenType::String:
            render( static_cast<const String &>(*tok), str );
            break;
          case TokenType::Tag:
            render( static_cast<const Tag &>(*tok), item_r, str, idx_r );
            break;
          case TokenType::Array:
            render( static_cast<const Array &>(*tok), item_r, str );
            break;
          case TokenType::Conditional:
            render( static_cast<const Conditional &>(*tok), item_r, str );
            break;
        }
      }
    }

    // TokenType::String
    void render( const String & string_r, std::ostream & str ) const
    { str << string_r.value; }

    // TokenType::Tag
    void render( const Tag & tag_r, const ItemT & item_r, std::ostream & str, unsigned idx_r ) const
    {
      std::string_view val{ _dataGetter( tag_r, item_r ) };
      if ( tag_r.fieldw  ) {
        const char * fieldw { tag_r.fieldw->c_str() };  // parser asserts at least one digit
        bool ladjust = ( *fieldw == '-' );
        if ( ladjust ) ++fieldw;
        std::size_t fw{ ::strtoul( fieldw, NULL, 10 ) };
        if ( val.size() < fw ) {
          char padchar = ( ladjust || fieldw[0] != '0' ? ' ' : '0' );
          if ( ladjust )
            str << val << std::string( fw-val.size(), padchar );
          else
            str << std::string( fw-val.size(), padchar ) << val;
          return;
        }
      }
      str << val;
    }

    // TokenType::Array
    void render( const Array & array_r, const ItemT & item_r, std::ostream & str ) const
    {
      render( array_r.format, item_r, str, 0 );
      render( array_r.format, item_r, str, 1 );
      render( array_r.format, item_r, str, 2 );
    }

    // TokenType::Conditional
    void render( const Conditional & conditional_r, const ItemT & item_r, std::ostream & str ) const
    { ; }

  private:
    Format _format;
    DataGetterT _dataGetter;
  };

} // namespace zypp::qf


///////////////////////////////////////////////////////////////////
void process( std::string_view qf_r )
try {
  qf::Renderer<PoolItem> render { qf_r };

  static const bool once __attribute__ ((__unused__)) = [](){
    zypp::misc::defaultLoadSystem( misc::LS_NOREFRESH | misc::LS_NOREPOS  );
    return true;
  }();

  unsigned max = 10;
  for ( const auto & el : zypp::ResPool::instance() ) {
    if ( not max-- ) break;
    render(el);
  }
}
catch( const std::exception & ex ) {
  MOUT << "Oopsed: " << ex.what() << endl;
}

///////////////////////////////////////////////////////////////////
int main( int argc, const char ** argv )
{
  using namespace zypp;
  using namespace std::literals;
  ++argv,--argc;

  if ( argc ) {
    if ( *argv == "p"sv ) {
      ++argv,--argc;
      for( ; argc; ++argv,--argc ) {
        MOUT << qf::parse( *argv ) << endl;
      }
      return 22;
    }

    //process( "[\"%15{name}-%-15{version}-%015{release}.%-015{arch}\"\n]" );
    process( "=== %15{name}-%015{version}-%-15{release}.%-015{arch}\n[%{requirenevrs}\n]" );
    return 0;
    for( ; argc; ++argv,--argc ) {
      process( std::string_view(*argv) );
    }
  }
  else {
    return qf::test::parsetest() == 0 ? 0 : 13;
  }

  return 0;
}


