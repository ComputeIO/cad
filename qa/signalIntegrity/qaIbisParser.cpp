#include "../../pcbnew/SI/ibisParser.cpp"

int main( void )
{
    IbisParser* parser = new IbisParser();
    IbisFile*   file = new IbisFile();
    parser->parseFile( wxFileName( "z11b_ver7.ibs" ), file );

    std::cout << "Done" << std::endl;
    return 1;
}