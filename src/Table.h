/*-----------------------------------------------------------*- c++ -*-\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/

#ifndef ZYPPER_TABULATOR_H
#define ZYPPER_TABULATOR_H

#include <string>
#include <iosfwd>
#include <set>
#include <list>
#include <vector>

#include <zypp/base/String.h>
#include <zypp/base/Gettext.h>

#include "utils/colors.h"

using std::string;
using std::ostream;
using std::list;
using std::vector;

//! table drawing style
enum TableLineStyle {
  Ascii         = 0,           ///< | - +
  Light,
  Heavy,
  Double,
  Light3,
  Heavy3,
  LightHeavy,
  LightDouble,
  HeavyLight,
  DoubleLight,
  Colon,
  none,
  _End,			       ///< sentinel
};

class Table;

class TableRow {
private:
  void dumpDetails(ostream &stream, const Table & parent) const;

public:
  //! Constructor. Reserve place for c columns.
  TableRow (unsigned c = 0) {
    _columns.reserve (c);
  }

  void add (const string& s);

  void addDetail (const string& s);

  // return number of columns
  unsigned int cols( void ) const;

  //! tab separated output
  void dumbDumpTo (ostream &stream) const;
  //! output with \a parent table attributes
  void dumpTo (ostream & stream, const Table & parent) const;

  typedef vector<string> container;

  // BinaryPredicate
  struct Less {
    unsigned _by_column;
    Less (unsigned by_column): _by_column (by_column) {}

    bool operator ()(const TableRow& a, const TableRow& b) const {
      return a._columns[_by_column] < b._columns[_by_column];
    }
  };

  const container & columns() const
  { return _columns; }

  container & columns()
  { return _columns; }

private:
  container _columns;
  container _details;

  friend class Table;
};

/** \relates TableRow Add colummn. */
template<class _Tp>
TableRow & operator<<( TableRow & tr, const _Tp & val )
{
  tr.add( zypp::str::asString( val ) );
  return tr;
}



class TableHeader : public TableRow {
public:
  //! Constructor. Reserve place for c columns.
  TableHeader (unsigned c = 0): TableRow (c) {}
};

/** \relates TableHeader  Add colummn. */
template<class _Tp>
TableHeader & operator<<( TableHeader & th, const _Tp & val )
{ static_cast<TableRow&>( th ) << val; return th; }

/** \todo nice idea but poor interface */
class Table {
public:
  typedef list<TableRow> container;

  static TableLineStyle defaultStyle;

  void add (const TableRow& tr);
  void setHeader (const TableHeader& tr);
  void dumpTo (ostream& stream) const;
  bool empty () const { return _rows.empty(); }
  void sort (unsigned by_column);       // columns start with 0...

  void lineStyle (TableLineStyle st);
  void wrap(int force_break_after = -1);
  void allowAbbrev(unsigned column);
  void margin(unsigned margin);

  const TableHeader & header() const
  { return _header; }
  const container & rows() const
  { return _rows; }
  container & rows()
  { return _rows; }

  Table ();

private:
  void dumpRule (ostream &stream) const;
  void updateColWidths (const TableRow& tr);

  bool _has_header;
  TableHeader _header;
  container _rows;

  //! maximum column index seen in this table
  unsigned _max_col;
  //! maximum width of respective columns
  mutable vector<unsigned> _max_width;
  //! table width (columns)
  int _width;
  //! table line drawing style
  TableLineStyle _style;
  //! amount of space we have to print this table
  int _screen_width;
  //! whether to abbreviate the respective column if needed
  vector<bool> _abbrev_col;
  //! left/right margin in number of spaces
  unsigned _margin;
  //! if _do_wrap is set, first break the table at this column;
  //! If negative, wrap as needed.
  int _force_break_after;
  //! Whether to wrap the table if it exceeds _screen_width
  bool _do_wrap;

  friend class TableRow;
};

inline
Table& operator << (Table& table, const TableRow& tr) {
  table.add (tr);
  return table;
}

inline
Table& operator << (Table& table, const TableHeader& tr) {
  table.setHeader (tr);
  return table;
}

inline
ostream& operator << (ostream& stream, const Table& table) {
  table.dumpTo (stream);
  return stream;
}

///////////////////////////////////////////////////////////////////
/// \class PropertyTable
/// \brief Alligned key/value with multiline support
/// Key       : value 1
/// LongKey   : value 2
/// Multiline :
///     line 1
///     line 2
/// Next Key  : value 3
///
///////////////////////////////////////////////////////////////////
class PropertyTable
{
public:
  PropertyTable()
  { _table.lineStyle( ::Colon ); }

public:
  ///////////////////////////////////////////////////////////////////
  // Key / Value
  template <class KeyType, class ValueType>
  PropertyTable & add( const KeyType & key_r, const ValueType & val_r )
  { _table << ( TableRow() << key_r << val_r ); return *this; }

  template <class KeyType>
  PropertyTable & add( const KeyType & key_r, bool val_r )
  { _table << ( TableRow() << key_r << (val_r ? _("Yes") : _("No")) ); return *this; }

  ///////////////////////////////////////////////////////////////////
  // Key / Container<Value>
  template <class KeyType, class _Iterator >
  PropertyTable & add( const KeyType & key_r, _Iterator begin_r, _Iterator end_r )
  {
    TableRow r;
    r << key_r;
    if ( begin_r != end_r )
    {
      _Iterator first = begin_r++;
      if ( begin_r != end_r )
      {
	unsigned cnt = 1;
	r.addDetail( *first );	// list in details
	while ( begin_r != end_r )
	{
	  ++cnt;
	  r.addDetail( *(begin_r++) );
	}
	r << cnt;		// size as value
      }
      else
	r << *first;		// only one value
    }
    else
      r << "";			// dummy to get the ":"
    _table << r;
    return *this;
  }

  template <class KeyType, class ContainerType>
  PropertyTable & lst( const KeyType & key_r, const ContainerType & lst_r )
  { return add( key_r, lst_r.begin(), lst_r.end() ); }

  template <class KeyType, class ValueType>
  PropertyTable & add( const KeyType & key_r, const std::set<ValueType> & lst_r )
  { return lst( key_r, lst_r );  }
  template <class KeyType, class ValueType>
  PropertyTable & add( const KeyType & key_r, const std::list<ValueType> & lst_r )
  { return lst( key_r, lst_r );  }
  template <class KeyType, class ValueType>
  PropertyTable & add( const KeyType & key_r, const std::vector<ValueType> & lst_r )
  { return lst( key_r, lst_r ); }

  ///////////////////////////////////////////////////////////////////
  // misc
  PropertyTable & paint( ColorContext cc_r, bool cond_r = true )
  {
    if ( cond_r )
    {
      std::string & lastval( _table.rows().back().columns().back() );
      lastval = ColorString( lastval, cc_r ).str();
    }
    return *this;
  }

public:
  friend std::ostream & operator << ( std::ostream & str, const PropertyTable & obj )
  { return str << obj._table; }

private:
  Table _table;
};



// Local Variables:
// c-basic-offset: 2
// End:
#endif
