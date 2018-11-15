#include <string>
#include <vector>
#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>

using namespace std;

namespace ptree
{
enum class eType
{
    category,
    text,
    integer,
    real,
    dropdown,
};
class cBranch
{
public:
    int myParent;
    int myChild;
    cBranch( int parent, int child )
        : myParent( parent )
        , myChild( child )
    {

    }
};

class cProp
{
public:
    int myParent;
    string myName;
    string myValue;
    string myDefault;
    vector<string> myOpts;
    eType myType;
    int myRowOnDisplay;

    cProp( const string& name,
           const string& value,
           eType type,
           int parent )
        : myName( name )
        , myValue( value )
        , myDefault( value )
        , myType( type )
        , myParent( parent )
    {

    }
    cProp( const string& name,
           const string& value,
           const vector<string> opts,
           int parent )
        : cProp( name, value, eType::dropdown, parent )
    {
        myOpts = opts;
    }
    string Text()
    {
        stringstream ss;
        ss << myParent
           <<" | "<< myName
           <<" | "<< myValue
           <<" | "<< (int)myType;
        return ss.str();
    }
};
class cPropertyTree
{
public:
    vector< cProp > myProp;
    vector< cBranch > myTree;

    cPropertyTree( nana::form& fm );

    /** Add category ( branch ) to the tree
        @param[in] name of the child
        @param[in] parent name of the parent, "theRoot" if top level
    */
    void AddCategory( const string& name,
                      const string& parent );

    /// Add property of various type
    void AddProperty( const string& name,
                      const string& value,
                      eType type,
                      const string& parent );

    /// Add dropdown property
    void AddProperty( const string& name,
                      const string& value,
                      eType type,
                      const vector<string>& opts,
                      const string& parent );

    /// Get property by index
    cProp& Property( int k )
    {
        return myProp[k];
    }

    /// Text display
    void Text( nana::form& fm );

    /// Draw
    void Draw( nana::form& fm );

    /// Pop-up inputbox to change property values
    void Input( nana::form& fm, int kprop );

private:
    int FindParent( const string& parent );

    vector< int > FindChildren( int parent );

    int FindCategoryFromY( int y );

};

cPropertyTree::cPropertyTree( nana::form& fm )
{
    //Show an inputbox when the form is clicked.
    fm.events().mouse_down([&](const nana::arg_mouse& arg)
    {
        Input( fm, arg.pos.y );
    });
}


void cPropertyTree::AddCategory( const string& name,
                                 const string& parent )
{
    int pa = -1;
    if( parent != "theRoot" )
    {
        pa = FindParent( parent );
    }

    myProp.push_back( cProp( name,"",eType::category,pa ));
    myTree.push_back( cBranch( pa, myProp.size()-1 ));

}

void cPropertyTree::AddProperty( const string& name,
                                 const string& value,
                                 eType type,
                                 const string& parent )
{
    int pa = FindParent( parent );
    myProp.push_back( cProp( name,value,type,pa ));
    myTree.push_back( cBranch( pa, myProp.size()-1 ));
}

void cPropertyTree::AddProperty( const string& name,
                                 const string& value,
                                 eType type,
                                 const vector<string>& opts,
                                 const string& parent )
{
    int pa = FindParent( parent );
    myProp.push_back( cProp( name,value,opts,pa ));
    myTree.push_back( cBranch( pa, myProp.size()-1 ));
}

void cPropertyTree::Text( nana::form& fm )
{
    nana::msgbox msg;
    for( auto& p : myProp )
        msg << p.Text() << "\n";
    msg << "\n";
    for( auto& b : myTree )
    {
        msg << b.myParent << "<<" << b.myChild << "\n";
    }
    msg();
}

void cPropertyTree::Draw( nana::form& fm)
{
    using namespace nana;
    drawing{ fm } .draw([&](paint::graphics& graph)
    {
        int kprop = -1;
        int row = 0;
        int depth = 1;
        for( auto& p : myProp )
        {
            kprop++;
            if( p.myParent != -1 )
                continue;
            if( p.myType == eType::category )
            {
                graph.string({ 20, 20*row }, p.myName );
                row++;
                depth++;
                for( int c : FindChildren( kprop ) )
                {
                    cProp& prop = Property( c );
                    if( prop.myType == eType::category )
                    {
                        graph.string({ 20 * depth, 20*row }, prop.myName );
                        prop.myRowOnDisplay = row;
                        row++;
                        depth++;
                        for( int c2 : FindChildren( c ) )
                        {
                            cProp& prop2 = Property( c2 );
                            prop2.myRowOnDisplay = row;
                            graph.string({ 20 * depth, 20*row },
                                         prop2.myName + ": " + prop2.myValue );
                            row++;
                        }
                        depth--;
                    }
                }
                depth--;
            }
        }

    });
}

void cPropertyTree::Input( nana::form& fm, int y )
{
    using namespace nana;
    int kprop = FindCategoryFromY( y );
    if( kprop < 0 )
        return;
    cProp& temp = myProp[ kprop ];
    if( temp.myType != eType::category )
    {
        kprop = temp.myParent;
    }
    cProp& prop = myProp[ kprop ];
    std::vector<inputbox::abstract_content*> contents;

    for( int c : FindChildren( kprop ) )
    {
        cProp& prop = myProp[ c ];
        switch ( prop.myType )
        {
        case eType::text:
            contents.push_back( new inputbox::text( prop.myName, prop.myValue ) );
            break;
        case eType::integer:
            contents.push_back( new inputbox::integer( prop.myName, atoi(prop.myValue.c_str()), 0, 100, 1 ) );
            break;
        case eType::real:
            contents.push_back( new inputbox::real( prop.myName, atof(prop.myValue.c_str()), 0, 20, 1 ) );
            break;
        case eType::dropdown:
            contents.push_back( new inputbox::text( prop.myName, prop.myOpts ));
            break;
        }
    }

    inputbox inbox(fm, "Please input", prop.myName);

    /* show the inputbox

    This uses _m_open which is private in the standard nana lib
    However it is so convenient that I have moved it into public
    */
    if( inbox._m_open( contents, true ) )
    {
        int k = -1;
        for( int c : FindChildren( kprop ) )
        {
            k++;
            cProp& prop = myProp[ c ];
            switch ( prop.myType )
            {
            case eType::text:
            case eType::dropdown:
                prop.myValue = ((inputbox::text*)contents[k])->value();
                break;
            case eType::integer:
            {
                stringstream ss;
                ss << ((inputbox::integer*)contents[k])->value();
                prop.myValue = ss.str();
            }
            break;
            case eType::real:
            {
                stringstream ss;
                ss << ((inputbox::real*)contents[k])->value();
                prop.myValue = ss.str();
            }
            break;

            }
        }
        drawing{ fm } .update();
    }

    // safely delete contents of inputbox
    int kc = -1;
    for( int c : FindChildren( kprop ) )
    {
        cProp& prop = myProp[ c ];
        switch ( prop.myType )
        {
        case eType::text:
        case eType::dropdown:
            kc++;
            delete ( (inputbox::text*)contents[kc] );
            contents[kc] = 0;
            break;
        case eType::integer:
            kc++;
            delete ( (inputbox::integer*)contents[kc] );
            contents[kc] = 0;
            break;
        case eType::real:
            kc++;
            delete ( (inputbox::real*)contents[kc] );
            contents[kc] = 0;
            break;
        default:
            break;
        }
    }
}

int cPropertyTree::FindCategoryFromY( int y )
{
    int targetrow = y / 20;
    int kprop = -1;
    for( auto& p : myProp )
    {
        kprop++;
        if( p.myRowOnDisplay == targetrow )
            return kprop;
    }
    return -1;
}

int cPropertyTree::FindParent( const string& parent )
{
    int k = -1;
    for( auto& prop : myProp )
    {
        k++;
        if( prop.myType != eType::category )
            continue;
        if( prop.myName == parent )
            return k;
    }
    throw std::runtime_error("Cannot find parent "+ parent );
}

vector< int > cPropertyTree::FindChildren( int parent )
{
    vector< int > vprop;
    int kprop = -1;
    for ( auto& b : myTree )
    {
        kprop++;
        if( b.myParent == parent )
            vprop.push_back( kprop );
    }
    return vprop;
}
}

int main()
{

    using namespace nana;
    form fm;
    ptree::cPropertyTree thePropertyTree( fm );


    thePropertyTree.AddCategory( "Signal Processing", "theRoot" );
    thePropertyTree.AddCategory( "Cardiac Activity Peak", "Signal Processing" );
    thePropertyTree.AddProperty( "Peak Finder", "none", ptree::eType::text, "Cardiac Activity Peak" );
    thePropertyTree.AddProperty( "Channel", "10", ptree::eType::integer, "Cardiac Activity Peak" );
    thePropertyTree.AddCategory( "Cardiac Peak Filters", "Signal Processing" );
    thePropertyTree.AddProperty( "Polarity", "positive", ptree::eType::dropdown, { "positive", "negative" }, "Cardiac Peak Filters" );
    thePropertyTree.AddProperty( "Height", "0.8", ptree::eType::real, "Cardiac Peak Filters" );
    thePropertyTree.AddProperty( "Separation", "100", ptree::eType::integer, "Cardiac Peak Filters" );
    thePropertyTree.AddProperty( "Width", "50", ptree::eType::integer, "Cardiac Peak Filters" );
    thePropertyTree.AddProperty( "Minimum Slope", "-1", ptree::eType::real, "Cardiac Peak Filters" );
    thePropertyTree.AddProperty( "Maximum Slope", "-1", ptree::eType::real, "Cardiac Peak Filters" );
    thePropertyTree.AddProperty( "Minimum Data Range", "-1", ptree::eType::integer, "Cardiac Peak Filters" );
    thePropertyTree.AddProperty( "Moving Average", "-1", ptree::eType::real, "Cardiac Peak Filters" );
    //thePropertyTree.Text( fm );
    thePropertyTree.Draw( fm );

    fm.show();

    exec();
}
