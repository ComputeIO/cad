#include <wx/wx.h>

#include <pcb_calculator.h>
#include <dialog_helpers.h>

wxString eseries_help =
#include <eseries_help.h>

extern double DoubleFromString( const wxString& TextValue );

void PCB_CALCULATOR_FRAME::ES_Init() 	// initialize ESeries tab at each pcb-calculator start
{
    std::cout<<"ES_Init"<<std::endl;
    wxString html_txt;			//show markdown formula explanation in lower help panel
    ConvertMarkdown2Html( wxGetTranslation( eseries_help ), html_txt );
    m_panelESeriesHelp->SetPage( html_txt );	
}

void PCB_CALCULATOR_FRAME::OnCalculateESeries( wxCommandEvent& event )
{
    double targetR;     // resistor value required

// preliminary dummy loop uses DoubleFromString what cannot do proper input parsing

    std::cout<<"Now calculating"<<std::endl;
    std::cout<<m_ResRequired->GetValue()<<std::endl;
    targetR = std::abs( DoubleFromString( m_ResRequired->GetValue() ) );
    std::cout<<targetR<<std::endl;
}

// Called on a ESeries Radio Button
//void PCB_CALCULATOR_FRAME::OnESeriesSelection( wxCommandEvent& event )
//{
//    std::cout<<"OnESerieSelection"<<std::endl;
//    SetESeries( (unsigned) event.GetSelection() );
//    Refresh();
//}

//void PCB_CALCULATOR_FRAME::SetESeries( unsigned aIdx )
//{
//      std::cout<<aIdx<<std::endl;
//}

/* down here old but functional javascript requires cpp conversion

var form
var targetR			//required resistor value
var useAllValues
var showValue

var arrayTop			//array of results with pointer to
var rName$ = new Array
var rangeTop			//range array
var rValue = new Array
var excludeTop			//exclude array
var excludeValue = new Array

var l$
var cPtr
var c$
var w$
var o$
var x$
var lastTerm

function Create(e12name$,e12value)
{ arrayTop++
  rName$[arrayTop] = e12name$
  rValue[arrayTop] = e12value
}

function R(e12name$,e12value)
{ var useThisValue = ( e12value != lastTerm ) || useAllValues
  if ( excludeTop != 0 )
  { var i = excludeTop
    while ( i > 0 && useThisValue )
    {if ( e12value == excludeValue[i] ) useThisValue = false
     i--
  } }
  if ( useThisValue ) Create(e12name$,e12value)
}

function E1()
{ R("1K", 1e3)
  R("10K", 10e3)
  R("100K", 100e3)
  R("10R", 10)
  R("100R", 100)
  R("1M", 1e6)
}

function E3()
{ E1()
  R("10R", 10)
  R("22R", 22)
  R("47R", 47)
  R("100R", 100)
  R("220R", 220)
  R("470R", 470)
  R("2K2", 2.2e3)
  R("4K7", 4.7e3)
  R("22K", 22e3)
  R("47K", 47e3)
  R("220K", 220e3)
  R("470K", 470e3)
  R("1M", 1e6)
}

function E6()
{ E1()
  E3() 
  R("15R", 15)
  R("33R", 33)
  R("68R", 68)
  R("150R", 150)
  R("330R", 330)
  R("680R", 680)
  R("1K5", 1.5e3)
  R("3K3", 3.3e3)
  R("6K8", 6.8e3)
  R("15K", 15e3)
  R("33K", 33e3)
  R("68K", 68e3)
  R("150K", 150e3)
  R("330K", 330e3)
  R("680K", 680e3)
}

function E12()
{ E1()
  E3()
  E6()
  R("12R", 12)
  R("18R", 18)
  R("27R", 27)
  R("39R", 39)
  R("56R", 56)
  R("82R", 82)
  R("120R", 120)
  R("180R", 180)
  R("270R", 270)
  R("390R", 390)
  R("560R", 560)
  R("820R", 820)
  R("1K2", 1.2e3)
  R("1K8", 1.8e3)
  R("2K7", 2.7e3)
  R("3K9", 3.9e3)
  R("5K6", 5.6e3)
  R("8K2", 8.2e3)
  R("12K", 12e3)
  R("18K", 18e3)
  R("27K", 27e3)
  R("39K", 39e3)
  R("56K", 56e3)
  R("82K", 82e3)
  R("120K", 120e3)
  R("180K", 180e3)
  R("270K", 270e3)
  R("390K", 390e3)
  R("560K", 560e3)
  R("820K", 820e3)
  R("1M", 1e6)
}


function Value(e12value$)
{ var i
  var k$ = ""
  i = Instr(e12value$,"E")
  if ( i != 0 ) showValue = true
  else
  {i = Instr(e12value$,"R")
   if ( i == 0 )
    {i = Instr(e12value$,"K")
      if ( i > 0 ) k$ = "e3"
      else
      {i = Instr(e12value$,"M")
        if ( i > 0 )
        {k$ = "e6"
    } } }

    if ( i != 0 )
    { var j = Instr(e12value$,".")
      if ( j > 0 )
      { e12value$ = Left$(e12value$,i-1)+Right$(e12value$,-i)
        showValue = true
      }
      else e12value$ = PokeMid$(e12value$,i,".")
      e12value$ = e12value$+k$
    }
    if ( Left$(e12value$,1) == "." ) e12value$ = "0"+e12value$
  }
  return Math.abs(Val(e12value$))

}

function ErrorPercent(gap,target)
{ if ( gap == 0 ) return( "None" )
  else
  { if ( gap > 0 )return( "+"+(parseInt((gap*100/target)*100)/100)+"%" )
    else return( ""+(parseInt((gap*100/target)*100)/100)+"%" )
} }

*************************************************************

function FormatR$(value)
{ if ( value >= 1e6 ) return ReplacePoint$(value,1e6,"M")
  else
  { if ( value >= 1e3 ) return ReplacePoint$(value,1e3,"K")
    else return ReplacePoint$(value,1,"R")
  }
}

*************************************************************

function ReplacePoint$(value,divisor,symbol$)
{ var t$ = ""+(value/divisor)
  var i = Instr(t$,".")
  if ( i == 0 ) return t$+symbol$
  else
  {if ( i == 1 )
    { t$="0"+t$
      i=2
    }
    return PokeMid$(t$,i,symbol$)
} }

*************************************************************

function DetermineName$(i,op,j)
{ var nameI$ = rName$[i]
  if ( j == 0 ) return nameI$
  else
  {var nameJ$ = rName$[j]
    if ( op == 0 )
    {if ( Instr(nameI$,"|") > 0 )
      { if ( Instr(nameJ$,"|") > 0 ) return "("+nameI$+") + ("+nameJ$+")"
        else return nameJ$+" + ( "+nameI$+" )"
      }
      else
      { if ( Instr(nameJ$,"|") > 0 ) return nameI$+" + ( "+nameJ$+" )"
        else return nameI$+" + "+nameJ$
      }
    }
    else
    {if ( Instr(nameI$,"+") > 0 )
      {if ( Instr(nameJ$,"+") > 0 ) return "("+nameI$+") | ("+nameJ$+")"
        else return nameJ$+" | ( "+nameI$+" )"
      }
      else
      {if ( Instr(nameJ$,"+") > 0 ) return nameI$+" | ( "+nameJ$+" )"
       else return nameI$+" | "+nameJ$
} } } }

*************************************************************

function ClearResults(form)
{ window.status=""
  form.txtMinimum.value = ""
  form.txtMinimumError.value = ""
  form.txtAlternative.value = ""
  form.txtAlternativeError.value = ""
  if (form.txtERange[0].checked || form.txtERange[1].checked )
  {form.txtFourRs.disabled = true
   form.txtFourRs.checked = false
  }
  {form.txtFourRs.disabled = false
} }

*************************************************************

function DoCalculate(aForm)
{ form = aForm
  ClearResults(form)
  if ( Trim$(form.txtValue.value) == "" ) { return };
  targetR = ParseExpr(form.txtValue.value); //fetch required value

  if ( targetR == 0 )								
  { form.txtMinimum.value = "Wire Link"
    form.txtMinimumError.value = "None"
    form.txtAlternative.value = "0R Resistor"
    form.txtAlternativeError.value = "None"
    return
  }
  arrayTop = 0		// Create single values
  ParseExclude()	// fetch exclude values
  if ( targetR < 5 )
  { R("1R", 1)
    if ( targetR < 1 ) Create("Wire Link", 0)
  }

  if ( form.txtERange[0].checked ) { E12() }
  if ( form.txtERange[1].checked ) { E6()  }
  if ( form.txtERange[2].checked ) { E3()  }
  if ( form.txtERange[3].checked ) { E1()  }


  if ( targetR > 3e6 || arrayTop == 0 )
  { R("10M", 10e6)
    if ( targetR > 30e6 || arrayTop <= 1 ) R("100M", 100e6)
  }

  if ( arrayTop == 0 )
  { form.txtMinimum.value = "All resistors are excluded"
    return
  }

  var rangeTop = arrayTop	//now calcualting
  var i		// index cnt 
  var j
  var k
  var n$	// result string

  for ( i=1; i<=rangeTop; i++ )
    Create( rName$[i]+" + "+rName$[i] , rValue[i]*2 ) //2R serial combination

  for ( i=1; i<=rangeTop; i++ ) 
    Create( rName$[i]+" | "+rName$[i] , rValue[i]/2 ) //2R parallel combination

  for ( i=1; i<=rangeTop; i++ )			// Create unequal serial combinations
  { k = rValue[i]
    n$ = rName$[i]+" + "
    for ( j=i+1; j<=rangeTop; j++ ) Create( n$+rName$[j] , k+rValue[j] )
  }

  for ( i=1; i<=rangeTop; i++ ) //Create unequal parallel combinations
  {k = 1/rValue[i]
   n$ = rName$[i]+" | "
   for ( j=i+1; j<=rangeTop; j++ ) Create( n$+rName$[j] , 1/(k+(1/rValue[j])) )
  }

                                // now select the best result
  var bestR      = rValue[1]	// Start with any arbitrary selection
  var bestGap    = Math.abs(bestR-targetR)
  var thisI = 1
  var thisJ = 0
  var thisOp = 0
  var thisR
  var thisGap
  i=2		// Try and find an exact match with what we have
  while ( ( i <= arrayTop ) && ( bestGap != 0 ) )
  {if ( Math.abs(rValue[i]-targetR) < bestGap )
    { bestR = rValue[i]
      bestGap = Math.abs(bestR-targetR)
      thisI = i
    }
    i++
  }

  var fN$ = DetermineName$(thisI,thisOp,thisJ) // Return  simple result
  var fR$ = FormatR$(targetR)
  if ( showValue && fN$ != fR$ )
  { if ( bestR == targetR ) form.txtMinimum.value = fN$+"  [ = "+fR$+" ]"
    else form.txtMinimum.value = fN$+"  [ ~= "+fR$+" ]"
  }
  else form.txtMinimum.value = fN$
  form.txtMinimumError.value = ErrorPercent(bestR-targetR,targetR)
  var minimumGap = 0.002*targetR
  if ( form.txtExact.checked ) minimumGap = 1e-6
  if ( bestGap == 0 ) form.txtAlternative.value = "Not needed"
  else
  {if ( bestGap > minimumGap ) form.txtAlternative.value = "Nothing better"
    else form.txtAlternative.value = "Not worth using"
  }

  if ( bestGap > minimumGap )
  { window.status="Looking for alternatives"
    var anythingBetter = false	// Note if we get anything better
    var thisTop = rangeTop	// Check known with added serial resistor
    if ( ( ! form.txtFourRs.disabled ) && form.txtFourRs.checked )
    { if ( form.txtERange[2].checked ) { thisTop = arrayTop  } // E3
      if ( form.txtERange[3].checked ) { thisTop = arrayTop  } // E1
    }
    i = 1
    while ( ( i <= arrayTop ) && ( bestGap > minimumGap ) )
    { k=rValue[i]
      j = 1
      while ( ( j <= thisTop ) && ( bestGap > minimumGap ) )
      { if ( Math.abs(k+rValue[j]-targetR) < bestGap )
        { anythingBetter = true
          bestR = k+rValue[j]
          bestGap = Math.abs( bestR-targetR )
          thisI = i
          thisJ = j
          thisOp = 0
        }
        j++
      }
      i++
    }

    if ( bestGap > minimumGap )	// locking for more alternatives
    {i = 1 	// Check known with added parallel resistor
     while ( ( i <= arrayTop ) && ( bestGap > minimumGap ) )
      { k=1/rValue[i]
        j = 1
        while ( ( j <= thisTop ) && ( bestGap > minimumGap ) )
        { thisR = 1/(k+(1/rValue[j]))
          thisGap = Math.abs( thisR-targetR )
          if ( thisGap < bestGap )
          { anythingBetter = true
            bestR = thisR
            bestGap = thisGap
            thisI = i
            thisJ = j
            thisOp = 1
          }
          j++
        }
        i++
    } }

    if ( anythingBetter )    // If we got anything better; show it
    { form.txtAlternative.value = DetermineName$(thisI,thisOp,thisJ)
      form.txtAlternativeError.value = ErrorPercent(bestR-targetR,targetR)
    }
  }
}

*************************************************************

function ParseExpr(expr$)
{ l$ = expr$
  o$ = ""
  x$ = ""
  lastTerm=-1
  useAllValues = false
  showValue = false
  FirstWord()
  var acc = Math.abs(Expr())
  if ( o$ != x$ ) form.txtValue.value = x$
  return acc
}

*************************************************************

function Expr()
{ var acc = Term()
  while ( w$ != "" && w$ != ")" )
  { showValue = true
    if ( w$ == "|" )
    { x$=x$+"|"
      Word()
      var t = Term()
      if ( acc == 0 || t == 0 ) acc = 0
      else acc = 1/( (1/acc) + (1/t) )
    }
    else
    {if ( w$ == "-" )
      { x$=x$+"-"
        Word()
        acc = acc - Term()
      }
      else
      { x$=x$+"+"
        if ( w$ == "+" ) Word()
        acc = acc + Term()
  } } }
  return acc
}

*************************************************************

function Term()
{ var acc
  var mpy
  if ( w$ == "(" )
  { x$=x$+"("
    Word()
    acc = Expr()
    if ( w$ == ")" ) Word()
    x$=x$+")"
  }
  else
  {if ( w$ == "-" )
    {
      x$=x$+"-"
      Word()
      acc = -Term()
    }
    else
    { if ( w$ == "" || w$ == ")" ) { w$ = "0R" }
      x$=x$+w$
      acc = Value(Ucase$(w$));
      Word()
      if ( acc != lastTerm && lastTerm >= 0 ) useAllValues = true
      lastTerm = acc
  } }
  while ( w$ == "*" || w$ == "/" )
  { showValue = true
    x$=x$+w$
    if ( w$ == "*" )
    { Word()
      x$=x$+w$
      mpy=Val(w$)
      acc = acc * mpy
    }
    else
    { Word()
      x$=x$+w$
      mpy=Val(w$)
      if ( mpy != 0 ) acc = acc / mpy
    }
    Word()
  }
  return acc
}

*************************************************************

function FirstWord()
{ cPtr = 1
  Char()
  Word()
}

function Word()
{ while ( c$ == " " ) Char()
  w$ = c$
  Char()
  if
  (w$ != ""  &&
    w$ != " " &&
    w$ != "+" &&
    w$ != "-" &&
    w$ != "|" &&
    w$ != "*" &&
    w$ != "/" &&
    w$ != "(" &&
    w$ != ")"
  )
  {while
    ( c$ != ""  &&
      c$ != " " &&
      c$ != "+" &&
      c$ != "-" &&
      c$ != "|" &&
      c$ != "*" &&
      c$ != "/" &&
      c$ != "(" &&
      c$ != ")"
    )
    { w$ = w$+c$
      Char()
    }
  }
  o$=o$+w$
}

function Char()
{if ( cPtr > Len(l$) ) c$ = ""
  else
  {
    c$ = Mid$(l$,cPtr,1)
    cPtr++
} }

function ParseExclude()
{ excludeTop = 0
  if ( form.txtExclude.checked )
  { var acc
    l$ = Ucase$(form.txtExcludeValues.value)
    o$=""
    x$=""
    FirstWord()
    while ( w$ != "" )
    { o$=o$+" "
      acc = Value(w$)
      if ( ! isNaN(acc) )
      { excludeTop++
        excludeValue[excludeTop] = acc
        x$=x$+w$+" "
      }
      Word()
    }
    if ( o$ != x$ ) form.txtExcludeValues.value = x$
} }

****************************************************************************
// string manipulation helper library

// remove leading and trailing spaces		      
// Trim$("  Hello There  ") ==> "Hello There"

function Trim$(t$)
{return ( Ltrim$(Rtrim$(t$)) )
}

// remove all leading spaces from a string
// Ltrim$("  Hello There  ") ==> "Hello There

function Ltrim$(t$)
{ var Alpha$ = t$
  while ( Left$(Alpha$,1) == " " ) Alpha$ = Right$(Alpha$,-1)
  return ( Alpha$ )
}

//remove all trailing spaces from a string
//Rtrim$("  Hello There  ") ==> "  Hello There"

function Rtrim$(t$)
{ var Alpha$ = t$
  while ( Right$(Alpha$,1) == " " ) Alpha$ = Left$(Alpha$,-1)
  return ( Alpha$ )
}

//returns the leftmost n or removes rightmost -n characters of a string 
//Left$("Hello There", 4) ==> "Hell"
//Left$("Hello There",-4) ==> "Hello T"

function Left$(t$,n)
{if ( n >=0 )
    return ( t$.substring(0,n) )
 else
    return ( Left$(t$,Len(t$)+n) )
}

// return the rightmost n or removes the leftmost -n charcters

function Right$(t$,n)
{ if ( n >=0 )
    return ( t$.substring(Len(t$)-n,Len(t$)) )
  else
    return ( Right$(t$,Len(t$)+n) )
}

//return w characters of string starting from position n
//Mid$("Hello There",3,5) ==> "llo T"

function Mid$(t$,n,w)
{  return ( t$.substring(n-1,n-1+w) )
}

// Overwrites string t at position n by string w
// PokeMid$("Hello There",3,"XX") ==> "HeXXo There"

function PokeMid$(t$,n,w$)
{return ( Left$(t$,n-1)+w$+Right$(t$, (-(n+Len(w$)))+1 ) )
}

// convert a string to uppercase
// Ucase$("Hello There") ==> "HELLO THERE"

function Ucase$(t$)
{return ( t$.toUpperCase() )
}

// return the number of characters in a string
// Len("Hello There") ==> 11                             
// Len("")            ==>  0                             
// Len(null)          ==>  0                             

function Len(t$)
{if ( t$ == null ) 	return ( 0 )
 else 			return ( t$.length )
}

// will return the position of target string where substring is found
// Instr("Hello There","There") ==> 7                 
// Instr("Hello There","llo")   ==> 3
// Instr("Hello There","X")     ==> 0

function Instr(t$,s$)
{ return ( t$.indexOf(s$)+1 )
}

*/
