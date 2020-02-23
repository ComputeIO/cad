/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 <author> janvi@veith.net
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/wx.h>

#include <pcb_calculator.h>
//#include <pcb_calculator_frame_base.h>
#include <dialog_helpers.h>

wxString eseries_help =
#include <eseries_help.h>

extern double DoubleFromString( const wxString& TextValue );

//** Known limitations against the javascript version (see www.veith.net/e12calc.html)

// todo 2 exclude values
// todo remove redundand brackets for 3R and 4R solutions
// todo cleanup unused UI checkbox
// 3 result output

//**
//** 1) Only one exclude value is allowed
//** If you need more than one exclude value, consider to select a smaller available E series
//** and complete your stock with E6 and E12 standard components
//** 
//** 2) Using Kicad DoubleFromString() limit the calculator to enter the resistance in Kiloohms 
//** from 0,005 KiloOhm to 2000 KiloOhm. No formulas accepted. 5 decades excel available toler-
//** ance down to 0,1%. User are free to abstract the units from Megaohm or (milli)Ohm to KiloOhm 
//** as well as the parallel serial swap rule in case of capacitors itself.

// E-Values derived from a geometric sequence formula by Charles Renard were already  
// accepted and widely used before the ISO recommendation no. 3 has been published. 
// For this historical reason, rounding rules of some values are sometimes irregular.
// Although all E-Values could be calculated at runtime, we initialize them in a lookup table
// what seems the most easy way to consider any inconvenient irregular rules. Same table can 
// also be used to lookup non calculatable but readable value strings commonly used in BOM.
// Smaller E-series are a subset of the next bigger E-series. E48, E96 and E192 do not follow  
// this rule. Such predicament causes frequently requests from contract manufacturers like:
// Are we allowed to use a pullup resistor value of 4k75 instead of 4k7 given by BOM ?
// To solve this, most (but not all) values of the lower E-series are inserted in the E-196 series 
// This way, designer can use small tolerance E-196 series and limit the stock to E12 values.
// For exhaustive calculations, unsorted E-series LookUpTables (LUT) are sufficient.
// Higher E-Series increase computing time and memory use by square to cube. Although this seems 
// a dont care for Kicads multikernel workstations, this calculator is limited to the E12 series.

enum              {E12, E6, E3, E1};            // usable E-series
enum              {S2R, S3R, S4R};              // number of resistors for a solution

struct r_data    { 
                     bool        e_use;        
                     std::string e_name;       
                     double      e_value;      
                 };

struct lut_dir   {
                     r_data*  lut_ptr;
                     uint     lut_size;
                 };

// Number of used values for each E-Series in use 

#define E12_SIZE ( sizeof ( e12_lut ) / sizeof ( r_data ) )
#define E6_SIZE  ( sizeof ( e6_lut )  / sizeof ( r_data ) )
#define E3_SIZE  ( sizeof ( e3_lut )  / sizeof ( r_data ) )
#define E1_SIZE  ( sizeof ( e1_lut )  / sizeof ( r_data ) )

// Number of parallel & serial brute force solutions for the largest E-Series E12 in use 
// Swappable terms, generate a single, equal and redundant result for each of the combinations

#define MAX_COMB ( 2 * ( E12_SIZE * E12_SIZE ) )

// Used E-series values from 10 Ohms to 1 and its associated BOM strings. All series larger E1 
// are defined by additional values what are containing the previous series

#define E1_VAL 	 {true,"1K",1000},\
                 {true,"10K",10000},\
                 {true,"100K",100000},\
                 {true,"10R",10},\
                 {true,"100R",100},\
                 {true,"1M",1000000}

#define E3_ADD 	 {true,"22R",22},\
                 {true,"47R",47},\
                 {true,"220R",220},\
                 {true,"470R",470},\
                 {true,"2K2",2200},\
                 {true,"4K7",4700},\
                 {true,"22K",22000},\
                 {true,"47K",47000},\
                 {true,"220K",220000},\
                 {true,"470K",470000}

#define E6_ADD 	 {true,"15R",15},\
                 {true,"33R",33},\
                 {true,"68R",68},\
                 {true,"150R",150},\
                 {true,"330R",330},\
                 {true,"680R",680},\
                 {true,"1K5",1500},\
                 {true,"3K3",3300},\
                 {true,"6K8",6800},\
                 {true,"15K",15000},\
                 {true,"33K",33000},\
                 {true,"68K",68000},\
                 {true,"150K",150000},\
                 {true,"330K",330000},\
                 {true,"680K",680000}

#define E12_ADD  {true,"12R",12},\
                 {true,"18R",18},\
                 {true,"27R",27},\
                 {true,"39R",39},\
                 {true,"56R",56},\
                 {true,"82R",82},\
                 {true,"120R",120},\
                 {true,"180R",180},\
                 {true,"270R",270},\
                 {true,"390R",390},\
                 {true,"560R",560},\
                 {true,"820R",820},\
                 {true,"1K2",1200},\
                 {true,"1K8",1800},\
                 {true,"2K7",2700},\
                 {true,"3K9",3900},\
                 {true,"5K6",5600},\
                 {true,"8K2",8200},\
                 {true,"12K",12000},\
                 {true,"18K",18000},\
                 {true,"27K",27000},\
                 {true,"39K",39000},\
                 {true,"56K",56000},\
                 {true,"82K",820000},\
                 {true,"120K",120000},\
                 {true,"180K",180000},\
                 {true,"270K",270000},\
                 {true,"390K",390000},\
                 {true,"560K",560000},\
                 {true,"820K",820000}

r_data e1_lut[] = {
                    E1_VAL
                  };

r_data e3_lut[] = {
                    E1_VAL,
                    E3_ADD
                  };

r_data e6_lut[] = {
                    E1_VAL,
                    E3_ADD,
                    E6_ADD
                  };

r_data e12_lut[] ={
                    E1_VAL,
                    E3_ADD,
                    E6_ADD,
                    E12_ADD
                  };

lut_dir lut_tab[] =  {
                      {( e12_lut), E12_SIZE},\
                      {( e6_lut ),  E6_SIZE},\
                      {( e3_lut ),  E3_SIZE},\
                      {( e1_lut ),  E1_SIZE}
                     };				//directory for look up tables 
                      
r_data comb_lut[MAX_COMB];	// array with all 2 components combination solutions
r_data rslt_lut[S4R+1];		// array with 2,3 and 4 component solutions

uint act_rb = 0;		// radio button default selection
double targetR;			// required resistor value what is searched for

//************************************************************************************************

void lut_dump(uint size)
{
  uint i;

  std::cout<<"Solutions 2R, 3R, 4R:"<<std::endl;
  for (i=0; i<size;  i++)
  {std::cout<<\
   rslt_lut[i].e_use<<" "<<\
   rslt_lut[i].e_name<<\
   " Deviation: "<<(rslt_lut[i].e_value)<<" Ohm"\
   <<std::endl;
  }
}

//************************************************************************************************
//**
//** If any value of the solution is not available, it can be entered as an exclude value.

void parseExclude(double res_exclude)
{
  uint i;                                  // Index to initialize all resistor data lookup tables

  for (i=0; i<E12_SIZE; i++)   e12_lut[i].e_use = true;	// assume all values available
  for (i=0; i<E6_SIZE;  i++)   e6_lut[i].e_use  = true;
  for (i=0; i<E3_SIZE;  i++)   e3_lut[i].e_use  = true;
  for (i=0; i<E1_SIZE;  i++)   e1_lut[i].e_use  = true;

  for (i=0; i<=S4R;      i++) rslt_lut[i].e_use  = false; // no results before calculation done
  for (i=0; i<MAX_COMB;  i++) comb_lut[i].e_use  = false; // no combinations available

 /*
  std::cout<<"Exclude ";
  if (res_exclude)
  {
      std::cout<<res_exclude<< "K Ohm"<<std::endl;   // entry was in KiloOhms
      res_exclude = res_exclude * 1000;              // convert to Ohms
      for (i=0; i<E1_SIZE;  i++)		     // check all E-series without care for its use
      {
         if ( e1_lut[i].e_value == res_exclude)      // if value to exclude found
         { 
           e1_lut[i].e_use = false;		     // disable its use
         }
      }
      for (i=0; i<E3_SIZE;  i++)		     // check all E-series without care for its use
      {
         if ( e3_lut[i].e_value == res_exclude)      // if value to exclude found
         {  
           e3_lut[i].e_use = false;		     // disable its use
         }
      }
      for (i=0; i<E6_SIZE;  i++)		     // check all E-series without care for its use
      {
         if ( e6_lut[i].e_value == res_exclude)      // if value to exclude found
         { 
           e6_lut[i].e_use = false;		     // disable its use
         }
      }
      for (i=0; i<E12_SIZE;  i++)		     // check all E-series without care for its use
      {
         if ( e12_lut[i].e_value == res_exclude)     // if value to exclude found
         { 
           e12_lut[i].e_use = false;	     	     // disable its use
         }
      }
    }
  else std::cout<<"All values available"<<std::endl; //entry was in KiloOhms
*/
}

//************************************************************************************************
//
// search for closest two component solution

void simple_solution (uint size)
{
  uint i;
  rslt_lut[S2R].e_value = 9999999;	 // assume no 2R solution or very big deviation
  
  #ifdef DEBUG  
  std::cout<<"S2R search, Combi = "<<size<<std::endl;
  #endif
  
  for (i=0; i<size; i++)
  {if ( abs( comb_lut[i].e_value - targetR )  < abs( rslt_lut[S2R].e_value ) )
      {
     rslt_lut[S2R].e_value = comb_lut[i].e_value - targetR;	// save deviation in Ohms
     rslt_lut[S2R].e_name = comb_lut[i].e_name;			// save combination text
     rslt_lut[S2R].e_use = true;				// this is a possible solution

     #ifdef DEBUG
     std::cout<<"found: "    <<rslt_lut[S2R].e_name<<\
                " Deviation "<<rslt_lut[S2R].e_value<<\
     std::endl;
     #endif
    }
  }
}

//************************************************************************************************
//
// Check if there is a better four component solution. The parameter 
// (uint size) gives the number of combinations to be checked inside comb_lut
// 5 decade E12 series gives over 100 Mio combinations

void combine4(uint size)
{
    uint i,j;
    double tmp_result;
    std::string s;

    rslt_lut[S4R].e_use = false;                   // disable 4R solution, until
    rslt_lut[S4R].e_value = rslt_lut[S3R].e_value; // 4R becomes better than 3R solution

    #ifdef DEBUG
    std::cout<<"S4R serial search "<<std::endl;
    #endif
    
    for (i=0; i<size;  i++) 			// 2R+2R=4R serial search
    {                                           // go through all 2R solutions 
      for (j=0; j<size;  j++)			// combine all with itself
      {
        tmp_result = comb_lut[j].e_value + comb_lut[i].e_value;     // calculate 2R+2R serial
        tmp_result -= targetR;                                      // calculate 4R deviation
        if ( abs ( tmp_result )  < abs ( rslt_lut[S4R].e_value ) )  // if new 4R is better
        {                                                           // then take it
          rslt_lut[S4R].e_value = tmp_result;                       // save amount of benefit
          s = "( "; 
          s.append ( comb_lut[i].e_name );                          // mention 1st 2 component
          s.append ( " ) + ( " );	                            // in series
          s.append ( comb_lut[j].e_name );                          // with 2nd 2 components
          s.append ( " )" );
          rslt_lut[S4R].e_name = s;	                            // with 2nd 2 components
          rslt_lut[S4R].e_use = true;                               // enable later use
          std::cout<<"found: "<<rslt_lut[S4R].e_name<<\
          " Deviation "<<rslt_lut[S4R].e_value<<std::endl;
        }
      }
    }
    
    #ifdef DEBUG
    std::cout<<"S4R parallel search "<<std::endl;
    #endif
    
    for (i=0; i<size;  i++) 			// 2R|2R=4R parallel search
    {                                           // go through all 2R solutions 
      for (j=0; j<size;  j++)			// combine all with itself
      {
        tmp_result = 1 /\
        ( 1 / comb_lut[j].e_value + 1 / comb_lut[i].e_value);       // calculate 2R|2R parallel
        tmp_result -= targetR;                                      // calculate 4R deviation
        if ( abs ( tmp_result )  < abs ( rslt_lut[S4R].e_value ) )  // if new 4R is better
        {                                                           // then take it
        rslt_lut[S4R].e_value = tmp_result;			   // save amount of benefit
        s = "( "; 
        s.append ( comb_lut[i].e_name );                           // mention 1st 2 component
        s.append ( " ) + ( " );		                           // in series
        s.append ( comb_lut[j].e_name );                           // with 2nd 2 components
        s.append ( " )" );
        rslt_lut[S4R].e_name = s;				   // with 2nd 2 components
        rslt_lut[S4R].e_use = true;                                // enable later use
        #ifdef DEBUG
        std::cout<<"found: "<<rslt_lut[S4R].e_name<<\
        " Deviation "<<rslt_lut[S4R].e_value<<std::endl;
        #endif
        }
      }
    }
}

//************************************************************************************************
//
// Check if there is a better three component solution. The parameter 
// (uint size) gives the number of combinations to be checked inside comb_lut

void combine3(uint size)
{
    uint act_size,i,j = 0;
    r_data* act_serie;
    double tmp_result;
    std::string s;

    act_serie = lut_tab[act_rb].lut_ptr;           //lookup selected E-series
    act_size =  lut_tab[act_rb].lut_size;          //and its number of entries

    rslt_lut[S3R].e_use = false;                   // disable 3R solution, until
    rslt_lut[S3R].e_value = rslt_lut[S2R].e_value; // 3R should be better than 2R solution
    
    #ifdef DEBUG
    std::cout<<\
    "S3R serial search "<<\
    std::endl;
    #endif
                                                
    for (i=0; i<act_size;  i++) 		// Solution search for 3R serial
    {                                           // go through all values from used E-serie 
      if ( act_serie[i].e_use)                  // skip all excluded values
      {
        for (j=0; j<size;  j++)			// combine with all 2R combinations
        {
          tmp_result = comb_lut[j].e_value + act_serie[i].e_value;    // calculate serial combi
          tmp_result -= targetR;                                      // calculate deviation

          if ( abs ( tmp_result )  < abs ( rslt_lut[S3R].e_value ) )  // compare if better
          {                                                           // its an improvment, take it
            s = ( act_serie[i].e_name );                              // mention 3rd component
            s.append( " + ( " );			              // in series
            s.append( comb_lut[j].e_name );                           // with 2R combination
            s.append( " )" );
            rslt_lut[S3R].e_name = s;                                 // save S3R result
            rslt_lut[S3R].e_value = tmp_result;			      // save amount of benefit
            rslt_lut[S3R].e_use = true;                               // enable later use
            #ifdef DEBUG
            std::cout<<"found: "<<rslt_lut[S3R].e_name<<\
            " Deviation "<<rslt_lut[S3R].e_value<<std::endl;
            #endif
          }
        }
      }
    }
    
    #ifdef DEBUG
    std::cout<<"S3R parallel search "<<std::endl;
    #endif
                                                
    for (i=0; i<act_size;  i++) 		// Solution search for 3R parallel
    {                                           // go through all values from used E-serie 
      if ( act_serie[i].e_use)                  // skip all excluded values
      {
        for (j=0; j<size;  j++)			// combine with all 2R combinations
        {
          tmp_result = 1/( 1/comb_lut[j].e_value + 1/act_serie[i].e_value );    // parallel combi
          tmp_result -= targetR;                                      // calculate deviation

          if ( abs ( tmp_result )  < abs ( rslt_lut[S3R].e_value ) )  // compare if better
          {                                                           // then take it
            s = act_serie[i].e_name;		                      // mention 3rd component
            s.append( " | ( " );                                      // in parallel
            s.append( comb_lut[j].e_name );                           // with 2R combination
            s.append( " )" );
            rslt_lut[S3R].e_name = s;
            rslt_lut[S3R].e_value = tmp_result;			      // save amount of benefit
            rslt_lut[S3R].e_use = true;                               // enable later use
            #ifdef DEBUG
            std::cout<<"found: "<<rslt_lut[S3R].e_name<<\
            " Deviation: "<<rslt_lut[S3R].e_value<<std::endl;
            #endif
          }
        }
      }
    }
    combine4(size);  // continiue searching for a possibly better 4R solution
}
  
//************************************************************************************************
//**
//** According to MAX_COMB makro, 2 component combinations including redundant swappable terms are
//** 72 combinations for E1
//** 512 combinations for E3
//** 1922 combinations for E6
//** 7442 combinations for E12

void doCalculate(void)
{
    uint act_size,i,j,i_results=0;
    r_data* act_serie;
    std::string s;

    act_serie = lut_tab[act_rb].lut_ptr;	 // lookup selected E-series
    act_size =  lut_tab[act_rb].lut_size;	 // and its number of entries

    for (i=0; i<act_size;  i++) 		// calculate 2 component serial results 
    {
      for (j=0; j<act_size;  j++)
      {
      comb_lut[i_results].e_use = true;
      comb_lut[i_results].e_value = act_serie [i].e_value + act_serie[j].e_value;
      s = act_serie[i].e_name;		                              // mention first component
      s.append(" + ");			                              // in series
      comb_lut[i_results++].e_name = s.append(act_serie[j].e_name);    // with second component
      }
    }

    for (i=0; i<act_size;  i++) 		// calculate 2 component parallel results 
    {
      for (j=0; j<act_size;  j++)
      {
      comb_lut[i_results].e_use = true;
      comb_lut[i_results].e_value = 1 /
      ((1 / act_serie[i].e_value) + (1 / (act_serie[j].e_value)));
      s = act_serie[i].e_name;		                            // mention first component
      s.append(" | ");			                            // in parallel
      comb_lut[i_results++].e_name = s.append(act_serie[j].e_name); // with second component
      }
    }
    simple_solution(i_results);
    combine3(i_results);        // continiue searching for a possibly better 3 components solution
    lut_dump(3);
}

//************************************************************************************************

void PCB_CALCULATOR_FRAME::OnCalculateESeries( wxCommandEvent& event )
{
    targetR = std::abs( DoubleFromString( m_ResRequired->GetValue() ) );

    #ifdef DEBUG
    std::cout<<", targetR = "<<targetR<<std::endl;
    #endif
    
    parseExclude(std::abs( DoubleFromString( m_ResExclude->GetValue() ) ));
    doCalculate();
}

//************************************************************************************************

void PCB_CALCULATOR_FRAME::OnESerieSelection( wxCommandEvent& event )
{
    act_rb = event.GetSelection();
    #ifdef DEBUG
    std::cout<<"act_rb = "<<act_rb<<\
    ", NoOfValues= "<<lut_tab[act_rb].lut_size;
    #endif
}

//************************************************************************************************

void PCB_CALCULATOR_FRAME::ES_Init()    // initialize ESeries tab at each pcb-calculator start
{
    #ifdef DEBUG
    std::cout<<"ES_Init"<<std::endl;
    #endif
    wxString html_txt;                  //show markdown formula explanation in lower help panel
    ConvertMarkdown2Html( wxGetTranslation( eseries_help ), html_txt );
    m_panelESeriesHelp->SetPage( html_txt );	
}
