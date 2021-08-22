#include "../../pcbnew/SI/ibisParser.cpp"

int main( void )
{
    IbisParser* parser = new IbisParser();
    IbisFile*   file = new IbisFile();
    parser->parseFile( wxFileName( "sn74lvc541a.ibs" ), file );

    std::cout << "Done" << std::endl;
    return 1;
}