/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       comp1                                                              */
/*                                                                          */
/*                                                                          */
/*       Group Members:      ID numbers                                     */
/*                                                                          */
/*       Aaron Moloney       14174014                                       */
/*       Liam Normoyle       14177994                                       */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "scanner.h"
#include "line.h"
#include "symbol.h"
#include "code.h"
#include "strtab.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */
PRIVATE FILE *CodeFile;

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
			                       /*  initialised before parser starts.    */
PRIVATE int scope=0; /*global scope variable for semantic parsing */
PRIVATE int varaddress=0;  /*global address variable for assigning globals next to each other*/

/* List of first and follow sets for error recovery*/ 
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


PRIVATE int  OpenFiles( int argc, char *argv[] );

PRIVATE void ParseProgram( void );
PRIVATE void ParseProcDeclaration( void );
PRIVATE void ParseDeclarations( void); 
PRIVATE void ParseParameterList( void );
PRIVATE void ParseFormalParameter( void );
PRIVATE void ParseBlock( void );
PRIVATE void ParseStatement( void );
PRIVATE void ParseSimpleStatement( void );
PRIVATE void ParseRestOfStatement( SYMBOL *target );
PRIVATE void ParseProcCallList( SYMBOL *target );
PRIVATE void ParseAssignment( void );
PRIVATE void ParseActualParameter( void );
PRIVATE void ParseWhileStatement( void );
PRIVATE void ParseIfStatement( void );
PRIVATE void ParseReadStatement( void );
PRIVATE void ParseWriteStatement( void );
PRIVATE void ParseExpression( void );
PRIVATE void ParseCompoundTerm( void );
PRIVATE void ParseTerm( void );
PRIVATE int  ParseBooleanExpression( void );

/* Supporting functions */

PRIVATE void Accept( int code );
PRIVATE void Synchronise( SET *F, SET *FB );
PRIVATE void SetupSets( void );
PRIVATE void MakeSymbolTableEntry( int symtype );
PRIVATE SYMBOL *LookupSymbol( void );



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Comp1 entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
    if ( OpenFiles( argc, argv ) )  {
      InitCharProcessor( InputFile, ListFile );
	    InitCodeGenerator ( CodeFile );
	    CurrentToken = GetToken();
	    ParseProgram();
	    WriteCodeFile();
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
/*       <Program>     :== "BEGIN" { <Statement> ";" } "END" "."            */
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
    if ( CurrentToken.code == VAR )  ParseDeclarations();     
    Synchronise( &ProgramFS_aug, &ProgramFBS);   
    while ( CurrentToken.code == PROCEDURE )
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
/*       <Declarations> ::== "VAR" <Variable> { "," <Variable> } ";"        */
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
    MakeSymbolTableEntry( STYPE_VARIABLE );
    Accept( IDENTIFIER );
    while ( CurrentToken.code == COMMA )  {
	    Accept( COMMA );
	    MakeSymbolTableEntry( STYPE_VARIABLE );
	    Accept( IDENTIFIER );
    }
    Accept( SEMICOLON );
}

PRIVATE void ParseProcDeclaration( void )
{
    Accept( PROCEDURE );
    MakeSymbolTableEntry( STYPE_PROCEDURE );
    Accept( IDENTIFIER );
    scope++;
    if ( CurrentToken.code == LEFTPARENTHESIS )  ParseParameterList();
    Accept( SEMICOLON );
    Synchronise( &ProcFS_aug, &ProcFBS); 
    if ( CurrentToken.code == VAR )  ParseDeclarations();
    Synchronise( &ProcFS_aug, &ProcFBS); 
    while ( CurrentToken.code == PROCEDURE )  
    {
	    ParseProcDeclaration();    	
	    Synchronise( &ProcFS_aug, &ProcFBS); 
    }
    ParseBlock();
    Accept( SEMICOLON );
    RemoveSymbols( scope );
    scope--;
}


PRIVATE void ParseParameterList( void )
{
    Accept( LEFTPARENTHESIS );
    ParseFormalParameter();
    while ( CurrentToken.code == COMMA ) /*check for 1 or more formal parameters */
      {
        Accept ( COMMA );
        ParseFormalParameter();
      }
    Accept( RIGHTPARENTHESIS );
}


PRIVATE void ParseFormalParameter( void )
{
    SYMBOL *var;  
    if ( CurrentToken.code == REF )  
    {
	    var = LookupSymbol();
	    Accept( REF);
    }
    var = LookupSymbol();
    if ( var != NULL ) { ;  } /* TODO Add code generation*/
    Accept ( IDENTIFIER );
}


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


PRIVATE void ParseSimpleStatement( void )
{
    SYMBOL *target;
    target = LookupSymbol(); /*Lookup Identifer in lookahead */
    Accept( IDENTIFIER );
    ParseRestOfStatement( target );
}


PRIVATE void ParseRestOfStatement( SYMBOL *target )
{

    switch (CurrentToken.code)
    {
    case LEFTPARENTHESIS:
            ParseProcCallList( target ); ;
    case SEMICOLON:
            if ( target != NULL && target->type == STYPE_PROCEDURE )
                    Emit( I_CALL, target->address);
            else
            {
                    /* TODO: Put error into function, make sure its robust*/
                    fprintf( stderr, "Fatal error:  procedure  \"%s\" is not a procedure\n",CurrentToken.s );
                    KillCodeGeneration();
                    fclose ( InputFile );
                    fclose ( ListFile );
                    fclose ( CodeFile );
                    exit ( EXIT_FAILURE );
            }
            /*TODO "SOME DETAIL OMITTED:" What? Does he mean the error code I put in? */
            break;
    case ASSIGNMENT: 
    default:
            ParseAssignment(); /*TODO: THIS WASNT PARSING ASSIGNMENT! Check and test in older versions */
            if ( target != NULL && target->type == STYPE_VARIABLE )
            {
                    Emit( I_STOREA,target->address);
            }
            else
            {
                    /* TODO: Put error into function, make sure its robust*/
                    fprintf( stderr, "Fatal error:  procedure  \"%s\" is not a procedure\n",CurrentToken.s );
                    KillCodeGeneration();
                    fclose ( InputFile );
                    fclose ( ListFile );
                    fclose ( CodeFile );
                    exit ( EXIT_FAILURE );
            }
            break;                       
                                           
    }
}



PRIVATE void ParseProcCallList( SYMBOL *target )
{
    Accept( LEFTPARENTHESIS );
    ParseActualParameter();
    while ( CurrentToken.code == COMMA ) /*check for 1 or more formal parameters */
      {
        Accept ( COMMA );
        ParseFormalParameter();
      }
    Accept( RIGHTPARENTHESIS );
}


PRIVATE void ParseAssignment( void )
{
        /*TODO Got rid of uneeded(? check) if */
	Accept ( ASSIGNMENT );    
    ParseExpression();

}


PRIVATE void ParseActualParameter( void )
{

    switch (CurrentToken.code)
    {
    case VAR:   Accept( VAR ); break;
    default:    ParseExpression(); break;
    }

}


PRIVATE void ParseWhileStatement( void )
{
    int Label1, Label2,L2BackPatchLoc;
    Accept ( WHILE );
    Label1=CurrentCodeAddress();
    L2BackPatchLoc = ParseBooleanExpression();
    Accept ( DO );
    ParseBlock();
    Emit ( I_BR,Label1);
    Label2 = CurrentCodeAddress();
    BackPatch ( L2BackPatchLoc,Label2);

}


PRIVATE void ParseIfStatement( void )
{
    int Label1,Label2,L1BackPatchLoc,L2BackPatchLoc;
    Accept ( IF );
    L1BackPatchLoc = ParseBooleanExpression(); /*Don't know where we branch to yet*/
    Accept ( THEN );    
    ParseBlock();
    Emit (I_BR,0); /*Backpatch later */
    L2BackPatchLoc = CurrentCodeAddress(); /*if (true)-->Execute + Branch past else block */
    if (CurrentToken.code == ELSE)
    {
        Emit (I_BR,0); /*case of if+then+else, if branches around the else block here */
	    Accept ( ELSE );
        Label1=CurrentCodeAddress();
        BackPatch ( L1BackPatchLoc, Label1 );
	    ParseBlock();
        Label2=CurrentCodeAddress();
        BackPatch ( L2BackPatchLoc, Label2 );
    }
    else /*if then statement, if(false)-> branch past THEN block*/
    {            
    Label1=CurrentCodeAddress(); 
    BackPatch ( L1BackPatchLoc,Label1); 
    }    
    
}

/* TODO EMIT FOR WRITE/READ STATEMENTS???*/
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
/*       <Expression>  :== <Identifier> | <IntConst>                        */
/*                                                                          */
/*       Note that <Identifier> and <IntConst> are handled by the scanner   */
/*       and are returned as tokens IDENTIFER and INTCONST respectively.    */
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
  int op;
  
  ParseCompoundTerm();
  while ( ( op = CurrentToken.code) == ADD || op  == SUBTRACT )
  {
      Accept( CurrentToken.code );
      ParseCompoundTerm();
      if ( op == ADD ) _Emit (I_ADD); else _Emit ( I_SUB );
  }



}


PRIVATE void ParseCompoundTerm( void )
{

    /*Term Parse*/
    int op;
    
    ParseTerm();
    

    /*MultOp Parse*/

    while ( ( op = CurrentToken.code ) == MULTIPLY  || op  == DIVIDE )
    {
	    if (CurrentToken.code == MULTIPLY || CurrentToken.code == DIVIDE)
	    {
		    Accept (CurrentToken.code);
		    ParseTerm();
	    }
        if ( op == MULTIPLY ) _Emit( I_MULT); else _Emit ( I_DIV );
    } 

}


PRIVATE void ParseTerm( void )
{
    SYMBOL *var;
    int negateflag=0;
    /*Term Parse*/  

    if (CurrentToken.code == SUBTRACT)
    {
            Accept ( SUBTRACT );
            negateflag=1;
    }
    /*SubTerm Parse*/


    switch (CurrentToken.code)
    {
    case IDENTIFIER:
            
	    var = LookupSymbol();
	    if ( var != NULL && var->type == STYPE_VARIABLE )
        {
                Emit( I_LOADA,var->address );
        }
        else
        {
           	 /* TODO: Put error into function, make sure its robust*/
                fprintf( stderr, "Fatal error: variable \"%s\" not delcared\n",CurrentToken.s );
		    fclose ( InputFile );
		    fclose ( ListFile );
		    fclose ( CodeFile );
		    exit ( EXIT_FAILURE );     
        }        
	    Accept ( IDENTIFIER );
	    break;
    case INTCONST:
            Emit ( I_LOADI,CurrentToken.value);
            Accept ( INTCONST );            
            break;
    default:
        Accept ( LEFTPARENTHESIS );
	    ParseExpression();
	    Accept ( RIGHTPARENTHESIS );
	    break;
    }     
    
    if (negateflag) _Emit ( I_NEG );
}
      
            
PRIVATE int ParseBooleanExpression(void)       
{
    int BackPatchAddr,RelOpInstruction;
    ParseExpression();
    switch (CurrentToken.code)
    {
    case EQUALITY:            
            RelOpInstruction = I_BZ;
            Accept( EQUALITY );
            break;
    case LESSEQUAL:                 
            RelOpInstruction = I_BG;
            Accept( LESSEQUAL );      
            break;
    case GREATEREQUAL:            
            RelOpInstruction = I_BL;
            Accept( GREATEREQUAL );
            break;
    case LESS:
            RelOpInstruction = I_BGZ;
            Accept( LESS );
            break;
    case GREATER:
    default:            
            RelOpInstruction = I_BLZ;
            Accept ( GREATER );
            break;      
    }    

    ParseExpression();
    _Emit( I_SUB );
    BackPatchAddr = CurrentCodeAddress();
    Emit( RelOpInstruction, 0);
    return BackPatchAddr;
            

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
/*           parsing).  Note the use of routine "SyntaxError"               */
/*           (from "scanner.h") which puts the error message on the         */
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
	    /* TODO: Need more defense of ENDOFINPUT inside this while loop? */
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

PRIVATE void MakeSymbolTableEntry( int symtype )
{
    SYMBOL *oldsptr;
    SYMBOL *newsptr;
    int hashindex;
    char *cptr;
    if (CurrentToken.code == IDENTIFIER)
    {
	    if ( NULL == ( oldsptr = Probe(CurrentToken.s, &hashindex )) || oldsptr->scope < scope)
	    {
		    if ( oldsptr == NULL )
		    {
			    cptr = CurrentToken.s;
		    }
		    else
		    {
			    cptr = oldsptr->s;
		    }
		    if ( NULL == ( newsptr = EnterSymbol( cptr, hashindex) ) )
		    {
			    fprintf( stderr, "Fatal Error from function EnterSymbol\n" );
			    fclose( InputFile );
			    fclose ( ListFile );
			    fclose ( CodeFile );
			    exit ( EXIT_FAILURE );
		    }
		    else
		    {
			    if ( oldsptr == NULL ) PreserveString();
			    newsptr->scope = scope;
			    newsptr->type = symtype;
			    if (symtype == STYPE_VARIABLE )
			    {
				    newsptr->address = varaddress;
				    varaddress++;
			    }
			    else
			    {
				    newsptr->address = -1;
			    }

		    }


	    }
	    else
	    {
		    /* TODO: Put error into function, make sure its robust*/
		    fprintf( stderr, "Fatal error:  Variable \"%s\" already delcared\n",oldsptr->s );
		    fclose ( InputFile );
		    fclose ( ListFile );
		    fclose ( CodeFile );
		    exit ( EXIT_FAILURE );
	    }

    }



}
PRIVATE SYMBOL *LookupSymbol ( void )
{
    SYMBOL *sptr;

    if (CurrentToken.code == IDENTIFIER )
    {
	    sptr = Probe(CurrentToken.s,NULL);
	    if ( sptr == NULL )
	    {
		    Error("Identifer not delcared",CurrentToken.pos);
		    KillCodeGeneration();
	    }

    }
    else
    {
	    sptr = NULL;
    }
    return sptr;
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

    if ( argc != 4 )
    {
	    fprintf( stderr, "%s <inputfile> <listfile> <codefile>\n", argv[0] );
	    return 0;
    }

    if ( NULL == ( InputFile = fopen( argv[1], "r" ) ) )
    {
	    fprintf( stderr, "cannot open \"%s\" for input\n", argv[1] );
	    return 0;
    }

    if ( NULL == ( ListFile = fopen( argv[2], "w" ) ) )
    {
	    fprintf( stderr, "cannot open \"%s\" for output\n", argv[2] );
	    fclose( InputFile );
	    return 0;
    }
    if ( NULL == ( CodeFile = fopen( argv[3], "w") ) )
    {
	    fprintf( stderr, "cannot open \"%s\" for output\n", argv[3] );
	    fclose( CodeFile );
	    return 0;
    }



    return 1;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadToEndOfFile:  Reads all remaining tokens from the input file.       */
/*              associated input and listing files.                         */
/*                                                                          */
/*    This is used to ensure that the listing file refects the entire       */
/*    input, even after a syntax error (because of crash & burn parsing,    */
/*    if a routine like this is not used, the listing file will not be      */
/*    complete.  Note that this routine also reports in the listing file    */
/*    exactly where the parsing stopped.  Note that this routine is         */
/*    superfluous in a parser that performs error-recovery.                 */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Reads all remaining tokens from the input.  There won't */
/*                  be any more available input after this routine returns. */
/*                                                                          */
/*--------------------------------------------------------------------------*/


