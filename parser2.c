/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser1                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*           Aaron Moloney       14174014                                   */
/*           Liam Normoyle       14177994                                   */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "scanner.h"
#include "line.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before parser starts.    */

PRIVATE SET ProgramFS_aug;
PRIVATE SET ProgramFBS;
PRIVATE SET StatementFS_aug;
PRIVATE SET StatementFBS;
PRIVATE SET ProcFS_aug;
PRIVATE SET ProcFBS;
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*Recursive descent functions */
PRIVATE void ParseProgram( void );
PRIVATE void ParseProcDeclaration( void );
PRIVATE void ParseDeclarations( void); 
PRIVATE void ParseParameterList( void );
PRIVATE void ParseFormalParameter( void );
PRIVATE void ParseBlock( void );
PRIVATE void ParseStatement( void );
PRIVATE void ParseSimpleStatement( void );
PRIVATE void ParseRestOfStatement( void );
PRIVATE void ParseProcCallList( void );
PRIVATE void ParseAssignment( void );
PRIVATE void ParseActualParameter( void );
PRIVATE void ParseWhileStatement( void );
PRIVATE void ParseIfStatement( void );
PRIVATE void ParseReadStatement( void );
PRIVATE void ParseWriteStatement( void );
PRIVATE void ParseExpression( void );
PRIVATE void ParseCompoundTerm( void );
PRIVATE void ParseTerm( void );
PRIVATE void ParseBooleanExpression( void );

/*Supporting functions */
PRIVATE void Accept( int code );
PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Synchronise( SET *F, SET *FB );
PRIVATE void SetupSets( void );






/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: parser2 entry point.  Sets up parser globals (opens input and     */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
    if ( OpenFiles( argc, argv ) )  { /*open list and input file */
        InitCharProcessor( InputFile, ListFile ); /*initialise scanner */
        CurrentToken = GetToken(); /*start with first token */
        ParseProgram();
        fclose( InputFile );
        fclose( ListFile );
        return  EXIT_SUCCESS;
    }
    else 
        return EXIT_FAILURE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Parser routines: Recursive-descent implementaion of the grammar's       */
/*                   productions.                                           */
/*                                                                          */
/*                                                                          */
/*  ParseProgram implements:                                                */
/*                                                                          */
/*       Program :== “PROGRAM” <Identifier> “;”				    */
/*	[ <Declarations> ] { <ProcDeclaration> } <Block> “.”		    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProgram( void )
{
	SetupSets();
    Accept( PROGRAM );
    Accept( IDENTIFIER );
    Accept( SEMICOLON );
    Synchronise( &ProgramFS_aug, &ProgramFBS); 
    ParseDeclarations();    
    if ( CurrentToken.code == VAR )  ParseDeclarations();    /* check for declarations */  
    Synchronise( &ProgramFS_aug, &ProgramFBS);   
    while ( CurrentToken.code == PROCEDURE ) /*check for 1 or more proc declarations */
      { 
    ParseProcDeclaration(); 
    Synchronise( &ProgramFS_aug, &ProgramFBS); 
      }
    ParseBlock();
    Accept( ENDOFPROGRAM );
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseDeclarations implements:                                           */
/*                                                                          */
/*       <Declarations> :== "VAR" <Variable> { "," <Variable> } ";"         */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseDeclarations( void )
{
    Accept( VAR );
    Accept( IDENTIFIER );
    while ( CurrentToken.code == COMMA )  { /*Check for 1 or more additional declarations */
        Accept( COMMA );
        Accept( IDENTIFIER );
    }
    Accept( SEMICOLON );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcDeclaration implements:                                        */
/*                                                                          */
/*       <Declarations> :== "PROCEDURE" <Identifer> [ <ParameterList> ]     */
/*       ";" [ <Declarations> ] { <ProcDeclaration> } <Block> ";"           */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcDeclaration( void )
{
    Accept( PROCEDURE );
    Accept( IDENTIFIER );
    if ( CurrentToken.code == LEFTPARENTHESIS )  ParseParameterList(); /*check for a parameter list */
    Accept( SEMICOLON );
    Synchronise( &ProcFS_aug, &ProcFBS); 
    if ( CurrentToken.code == VAR )  ParseDeclarations(); /*check for declarations */
    Synchronise( &ProcFS_aug, &ProcFBS); 
    while ( CurrentToken.code == PROCEDURE )   /*check for nested proc declarations */
   	{
    		ParseProcDeclaration();    	
	  	Synchronise( &ProcFS_aug, &ProcFBS); 
	}
    ParseBlock();
    Accept( SEMICOLON );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseParameterList implements:                                          */
/*                                                                          */
/*   <ParameterList> :=="(" <FormalParameter> { "." <FormalParameter> } ")" */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseParameterList( void )
{
    Accept( LEFTPARENTHESIS );
    ParseFormalParameter();
    while ( CurrentToken.code == COMMA ) ParseFormalParameter(); /*check for 1 or more formal parameters */
    Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseFormalParameter implements:                                        */
/*                                                                          */
/*       <FormalParameter> :== [ "REF" ] <Variable>                         */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseFormalParameter( void )
{
 
  if ( CurrentToken.code == REF )  Accept( REF);
  Accept ( IDENTIFIER );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBlock implements:                                                  */
/*                                                                          */
/*       <Block> :== "BEGIN" { <Statement> ";" } "END"                      */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseBlock( void )
{
    
    Accept( BEGIN );
	Synchronise( &StatementFS_aug, &StatementFBS); 
    
    while ( CurrentToken.code != END)
    {   
      ParseStatement();
      Accept( SEMICOLON );  
      Synchronise( &StatementFS_aug, &StatementFBS); 
    }
    
    Accept( END );

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseStatement implements:                                              */
/*                                                                          */
/*       <Statement>   :== <Identifier> ":=" <Expression>                   */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseStatement( void )
{
    
    switch (CurrentToken.code)
        {
        case WRITE:     ParseWriteStatement(); break;
        case WHILE:     ParseWhileStatement(); break;
        case IF:        ParseIfStatement(); break;
        case READ:      ParseReadStatement(); break;
        default:        ParseSimpleStatement(); break;
        }


}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSimpleStatement implements:                                        */
/*                                                                          */
/*       <SimpleStatement> :== <VarOrProcName> <RestOfStatement>            */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseSimpleStatement( void )
{
    Accept( IDENTIFIER );
    ParseRestOfStatement();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRestOfStatement implements:                                        */
/*                                                                          */
/*       <RestOfStatement> :== <ProcCallList> | <Assignment> | <nothing>    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseRestOfStatement( void )
{

    switch (CurrentToken.code)
        {
        case LEFTPARENTHESIS:   ParseProcCallList(); break;
        case ASSIGNMENT:    ParseAssignment(); break;
        case SEMICOLON:         break;
        default:        Accept( ENDOFINPUT ); break;
        }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcCallList implements:                                           */
/*                                                                          */
/*   <ProcCallList> :== "(" <ActualParameter> { "." {ActualParameter> } ")  */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseProcCallList( void )
{
        Accept( LEFTPARENTHESIS );
    ParseActualParameter();
        while ( CurrentToken.code == COMMA ) ParseActualParameter();
        Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAssignment implements:                                             */
/*                                                                          */
/*                 <Assignment> :== ":=" <Expression> 		            */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseAssignment( void )
{
    
    if (CurrentToken.code == ASSIGNMENT)
    {
      Accept ( ASSIGNMENT );            
    }

    ParseExpression();

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseActualParameter implements:                                        */
/*                                                                          */
/*     <ActualParameter> :==   <Variable> | <Expression                     */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseActualParameter( void )
{

    switch (CurrentToken.code)
        {
        case VAR:   Accept( VAR ); break;
        default:    ParseExpression(); break;
        }
    
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWhileStatement implements:                                         */
/*                                                                          */
/*     <WhileStatement> :==   "WHILE" <BooleanExpression> "DO" <Block>      */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseWhileStatement( void )
{

    Accept ( WHILE );   
    ParseBooleanExpression();
    Accept ( DO );
    ParseBlock();
    
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIfStatement implements:                                            */
/*                                                                          */
/*    <IfStatement> :==   "IF" <BooleanExpression> "THEN"                   */
/*    <Block> ["ELSE" <BLOCK>]                                             */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseIfStatement( void )
{

    Accept ( IF );  
    ParseBooleanExpression();
    Accept ( THEN );
    ParseBlock();

    if (CurrentToken.code == ELSE){
        Accept ( ELSE );    
        ParseBlock();
        }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseReadStatement implements:                                          */
/*                                                                          */
/*    <ReadStatement> :==   "READ" "(" <Variable> { "," <Variable> } ")"    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseReadStatement( void )
{

    Accept ( READ );    
    Accept ( LEFTPARENTHESIS ); 
    Accept ( IDENTIFIER );  
            
    while (CurrentToken.code == COMMA)
      {
        Accept ( COMMA );   
        Accept ( IDENTIFIER );
      }

    Accept ( RIGHTPARENTHESIS );            

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWriteStatement implements:                                         */
/*                                                                          */
/* <WriteStatement> :==   "WRITE" "(" <Expression> { "," <Expression> } ")" */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseWriteStatement( void )
{

    Accept ( WRITE );
    Accept ( LEFTPARENTHESIS ); 
    ParseExpression();  
            
    while (CurrentToken.code == COMMA){
        Accept ( COMMA );   
        ParseExpression();
        }

    Accept ( RIGHTPARENTHESIS );            

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseExpression implements:                                             */
/*                                                                          */
/*       <Expression>  :== <CompoundTerm> { <AddOp> <CompoundTerm> }        */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseExpression( void )
{
    ParseCompoundTerm();
    while ( (CurrentToken.code == ADD) || (CurrentToken.code == SUBTRACT)  )
    {
        Accept( CurrentToken.code );
        ParseCompoundTerm();
    }



}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseCompoundTerm implements:                                           */
/*                                                                          */
/*       <CompoundTerm>  :== <Term> { <MultOp> <Term> }                     */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseCompoundTerm( void )
{

    /*Term Parse*/  

    ParseTerm();

    /*MultOp Parse*/

    while ( (CurrentToken.code == MULTIPLY)  || (CurrentToken.code == DIVIDE) )
      {
        if(CurrentToken.code == MULTIPLY || CurrentToken.code == DIVIDE)
          {
        Accept (CurrentToken.code);
        ParseTerm();
          }
      } 

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseTerm implements:                                                   */
/*                                                                          */
/*       <Term>  :== [ "-" ] <SubTerm>                                      */
/*       <SubTerm> :== <Variable> | <IntConst> | "(" <Expression> ")"       */
/*       Note: -Merged term and subterm function as they are very similar   */
/*             -IntConst handled by scanner                                 */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseTerm( void )
{

    /*Term Parse*/  

    if (CurrentToken.code == SUBTRACT) Accept ( SUBTRACT ); 
        /*SubTerm Parse*/

    
        switch (CurrentToken.code)
            {
            case IDENTIFIER: { Accept ( IDENTIFIER ); break; }
            case INTCONST:   { Accept ( INTCONST ); break;   } 
            default:     {
                     Accept ( LEFTPARENTHESIS );
                         ParseExpression();
                     Accept ( RIGHTPARENTHESIS );
                     break;
                     }
            }
    
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*       <BooleanExpression>  :== <Expression> <RelOp> <Expression>         */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseBooleanExpression( void )
{
    
    ParseExpression();

    switch (CurrentToken.code)
    {
    case EQUALITY:      Accept( EQUALITY ); break;
    case LESSEQUAL:     Accept( LESSEQUAL ); break;
    case GREATEREQUAL:  Accept( GREATEREQUAL ); break;
    case LESS:      Accept( LESS ); break;
    default:    Accept ( GREATER ); break;      
    }

    ParseExpression();

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  End of parser.  Support routines follow.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Accept:  Takes an expected token name as argument, and if the current   */
/*           lookahead matches this, advances the lookahead and returns.    */
/*                                                                          */
/*           If the expected token fails to match the current lookahead,    */
/*           this routine reports a syntax error and exits ("crash & burn"  */
/*           parsing).  "SyntaxError" (from "scanner.h")                    */
/*           puts the error message on the                                  */
/*           standard output and on the listing file, and the helper        */
/*           "ReadToEndOfFile" which just ensures that the listing file is  */
/*           completely generated.                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       Integer code of expected token                          */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: If successful, advances the current lookahead token     */
/*                  "CurrentToken".                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void Accept( int ExpectedToken )
{
    static int recovering = 0;

     
     if ( CurrentToken.code == ENDOFINPUT)
     {    
         if (ExpectedToken == ENDOFINPUT) 
         {
             Error("Parsing ends here in this program \n", CurrentToken.pos);            
             fclose( InputFile );
             fclose( ListFile );
             exit ( EXIT_SUCCESS ); 
         }
         else
         {       
             Error("Resync at end of file \n", CurrentToken.pos);            
             fclose( InputFile );
             fclose( ListFile );
             exit ( EXIT_FAILURE ); 
         }
    }
    if (recovering) 
    {
        while ( CurrentToken.code != ExpectedToken && CurrentToken.code != ENDOFINPUT)
        { 
            CurrentToken = GetToken();
        } 
        recovering = 0;
    }

    if ( CurrentToken.code != ExpectedToken )  
    {
        SyntaxError( ExpectedToken, CurrentToken );
        recovering = 1;
    }
    else  
    {
        CurrentToken = GetToken();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Synchronise: Takes first and follow sets for the grammar                */
/*           If something is expected prints a syntax error and calls       */
/*           GetToken() until CurrentToken matches union of F and  FB       */
/*           i.e waits till we get the expected token of this or another    */
/*           setence or a beacon and then we continue the parser            */
/*                                                                          */
/*    Inputs:       Set F the first set of the sentence                     */
/*                  Set FB the follow set of the sentence plus beacons      */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects:  Can print out a SyntaxError if CurrentToken doesn't    */
/*                   match the first set (i.e Calls SyntaxError2 function   */
/*                   Advances CurrentToken until we match                   */
/*                   First set follow set or beacon                         */
/*--------------------------------------------------------------------------*/

PRIVATE void Synchronise( SET *F, SET *FB )
{
    SET S;
    S = Union( 2, F, FB );
    if ( !InSet( F, CurrentToken.code ) ) 
    {
		SyntaxError2( *F, CurrentToken );
		while ( !InSet( &S, CurrentToken.code ) )
		{
		CurrentToken = GetToken();
		}
	}
} 

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SetupSets  :  Initialise the first and follow sets for                  */
/*	 	  "sync" points in the parser for error recovery            */
/*                Currently sets for Statement,Program and ProcDeclartion   */
/*    Inputs:       None				                    */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects:  None                                                   */
/*--------------------------------------------------------------------------*/

PRIVATE void SetupSets( void )
{    
	InitSet( &StatementFS_aug, 6, IDENTIFIER, WHILE,IF, READ, WRITE, END );
	InitSet( &StatementFBS, 4, SEMICOLON, ELSE,ENDOFPROGRAM, ENDOFINPUT ); 
	InitSet ( &ProgramFS_aug,3, BEGIN, PROCEDURE, VAR ) ;
    InitSet ( &ProgramFBS,1,ENDOFINPUT );
    InitSet ( &ProcFS_aug,3,BEGIN,PROCEDURE,VAR);
    InitSet ( &ProcFBS,2, BEGIN,ENDOFINPUT);
    /*OPTIONAL TODO: Secondary sync 
    Declarations inner loop
    ProcDeclarations parameterlist inner loop
    Parameter list formal parameter inner loop
    Formal Parameter
    Proc call list actual parameter inner loop
    */    
} 


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  OpenFiles:  Reads strings from the command-line and opens the           */
/*              associated input and listing files.                         */
/*                                                                          */
/*    Note that this routine mmodifies the globals "InputFile" and          */
/*    "ListingFile".  It returns 1 ("true" in C-speak) if the input and     */
/*    listing files are successfully opened, 0 if not, allowing the caller  */
/*    to make a graceful exit if the opening process failed.                */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1) Integer argument count (standard C "argc").          */
/*                  2) Array of pointers to C-strings containing arguments  */
/*                  (standard C "argv").                                    */
/*                                                                          */
/*    Outputs:      No direct outputs, but note side effects.               */
/*                                                                          */
/*    Returns:      Boolean success flag (i.e., an "int":  1 or 0)          */
/*                                                                          */
/*    Side Effects: If successful, modifies globals "InputFile" and         */
/*                  "ListingFile".                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] )
{

    if ( argc != 3 )  {
        fprintf( stderr, "%s <inputfile> <listfile>\n", argv[0] );
        return 0;
    }

    if ( NULL == ( InputFile = fopen( argv[1], "r" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for input\n", argv[1] );
        return 0;
    }

    if ( NULL == ( ListFile = fopen( argv[2], "w" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for output\n", argv[2] );
        fclose( InputFile );
        return 0;
    }

    return 1;
}
