#include "../../pcbnew/SI/ibisParser.cpp"

int main( void )
{
    IbisParser* parser = new IbisParser();
    IbisFile*   file = new IbisFile();
    parser->parseFile( wxFileName( "ibis_v1_1.ibs" ), file );
    
    parser = new IbisParser();
    file = new IbisFile();
    parser->parseFile( wxFileName( "ibis_v2_1.ibs" ), file );

    std::cout << "Done" << std::endl;
    return 1;
}