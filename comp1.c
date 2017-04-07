/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       comp1 Implements parser2+semantic processing, code generation      */
/*       doesn't implement procedure calls                                  */
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
PRIVATE FILE *CodeFile;            /*  For output assembly code             */

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
/*  initialised before parser starts.    */
PRIVATE int scope=0; /*global scope variable for semantic parsing */
PRIVATE int varaddress=0;  /*global address variable for assigning globals next to each other*/
PRIVATE int vcount=0; /*number of global variables to push onto stack */

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
PRIVATE void ParseProcCallList ( void );
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
/*        Initialises code generator                                        */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
        if ( OpenFiles( argc, argv ) )  { /*open list code and input file */
                InitCharProcessor( InputFile, ListFile );  /*initialise scanner */
                InitCodeGenerator ( CodeFile ); /*initialise code generator  */
                CurrentToken = GetToken(); /*start with first token */
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
/*       Program :== “PROGRAM” <Identifier> “;”				                */
/*	[ <Declarations> ] { <ProcDeclaration> } <Block> “.”		            */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced                                */
/*                  Calls Syncronise function (see sideeffects              */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProgram( void )
{
        SetupSets();
        Accept( PROGRAM );
        Accept( IDENTIFIER );
        Accept( SEMICOLON );    
        /*Sync around the declarations (sync is for error detection & recovery) */
        Synchronise( &ProgramFS_aug, &ProgramFBS); 
        if ( CurrentToken.code == VAR )  ParseDeclarations();    /* check for declarations */   
        Synchronise( &ProgramFS_aug, &ProgramFBS);   
        while ( CurrentToken.code == PROCEDURE ) /*check for 1 or more proc declarations */
        { 
                ParseProcDeclaration(); 
                Synchronise( &ProgramFS_aug, &ProgramFBS);  /*Sync inside the loop  */
        }
        ParseBlock();
        Accept( ENDOFPROGRAM );
        _Emit( I_HALT ); /*halt assembly instructions */
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
/*                  Makes symbol table entry(ies) for a variable            */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseDeclarations( void )
{
        Accept( VAR );
        MakeSymbolTableEntry( STYPE_VARIABLE ); /*Enter variable into str tab */
        Accept( IDENTIFIER );
        vcount++;
        while ( CurrentToken.code == COMMA )  {  /*Check for 1 or more additional declarations */
                Accept( COMMA );
                MakeSymbolTableEntry( STYPE_VARIABLE );
                Accept( IDENTIFIER );
                vcount++; /*increment global variable count */
        }
        Accept( SEMICOLON );
        Emit(I_INC, vcount); /*make stack space for global variables */
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
/*    Side Effects: Lookahead token advanced                                */
/*                  Calls Syncronise function (see side effects)            */
/*                  Inc/dec global scope variable                           */
/*                  Makes symbol table entry(ies) for a proc name           */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcDeclaration( void )
{
        Accept( PROCEDURE );
        MakeSymbolTableEntry( STYPE_PROCEDURE );
        Accept( IDENTIFIER );
        scope++; /*Increment scope because we are inside a proc*/
        if ( CurrentToken.code == LEFTPARENTHESIS )  ParseParameterList(); /*check for a parameter list */
        Accept( SEMICOLON );
        Synchronise( &ProcFS_aug, &ProcFBS); 
        if ( CurrentToken.code == VAR )  ParseDeclarations();  /*check for declarations */
        Synchronise( &ProcFS_aug, &ProcFBS); 
        while ( CurrentToken.code == PROCEDURE )   /*check for nested proc declarations */
        {
                ParseProcDeclaration();    	
                Synchronise( &ProcFS_aug, &ProcFBS); 
        }
        ParseBlock();
        Accept( SEMICOLON );
        RemoveSymbols( scope ); /*Clear local variables */
        scope--; /*Put scope back to where it was */
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
        while ( CurrentToken.code == COMMA ) /*check for 1 or more formal parameters */
        {
                Accept ( COMMA );
                ParseFormalParameter();
        }
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
/*                  Calls lookup symbol, see side effects for that          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseFormalParameter( void )
{
        SYMBOL *var;  
        if ( CurrentToken.code == REF )  
        {
                var = LookupSymbol();
                Accept( REF);
        }
        var = LookupSymbol();
        if ( var != NULL ) { ;  } /* TODO Add code generation (for comp2.c) */
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
/*    Side Effects: Lookahead token advanced                                */
/*                  Calls Syncronise function (see side effects)            */
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
                Synchronise( &StatementFS_aug, &StatementFBS);  /*Sync inside statements loop  */
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
/*                  Calls lookupsymbol (see side effects)                   */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseSimpleStatement( void )
{
        SYMBOL *target;
        target = LookupSymbol(); /*Lookup Identifer in lookahead */
        Accept( IDENTIFIER );
        ParseRestOfStatement( target );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRestOfStatement implements:                                        */
/*                                                                          */
/*       <RestOfStatement> :== <ProcCallList> | <Assignment> | <nothing>    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       SYMBOL *target the identifier it uses from above        */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                  Error handling which will exit the program              */
/*                  Can Emit I_STORE or I_CALL                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseRestOfStatement( SYMBOL *target )
{

        switch (CurrentToken.code)
        {
        case LEFTPARENTHESIS:
                ParseProcCallList(); /*no target until comp2.c */
        case SEMICOLON: /* 'nothing' so no call to accept */
                if ( target != NULL && target->type == STYPE_PROCEDURE )
                        Emit( I_CALL, target->address);
                else
                {      
                        if ( target == NULL)
                        {                                
                                ; /*Seems lookup symbol tackles this case */
                        }
                        else
                        {
                                fprintf( stderr, "Semantic error: procedure %s is not declared\n",target->s);
                        }                  
                        KillCodeGeneration();
                }
                break;
        case ASSIGNMENT: 
        default:
                ParseAssignment(); 
                if ( target != NULL && target->type == STYPE_VARIABLE )
                {
                        Emit( I_STOREA,target->address);
                }
                else
                {
                        if ( target == NULL)
                        {                                
                                ; /*Seems lookup symbol tackles this case */
                        }
                        else
                        {
                                fprintf( stderr, "Semantic error: variable %s is not declared\n",target->s);
                        }
                        KillCodeGeneration();
                }
                break;                       
                                           
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
        while ( CurrentToken.code == COMMA ) /*check for 1 or more actual parameters */
        {
                Accept ( COMMA );
                ParseActualParameter();
        }
        Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAssignment implements:                                             */
/*                                                                          */
/*                 <Assignment> :== ":=" <Expression> 		                */
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
        Accept ( ASSIGNMENT );    
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
/*                  Emits some assembly code (+Calls backpatch)              */
/*--------------------------------------------------------------------------*/


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

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIfStatement implements:                                            */
/*                                                                          */
/*    <IfStatement> :==   "IF" <BooleanExpression> "THEN"                   */
/*    <Block> ["ELSE" <BLOCK>]                                              */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                  Emits some assembly code (+Calls backpatch)             */
/*--------------------------------------------------------------------------*/

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
/*                  Emits I_READ assembly                                   */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseReadStatement( void )
{
  
        Accept ( READ );    
        _Emit ( I_READ );
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
/*                  Emits I_WRITE assembly                                  */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseWriteStatement( void )
{
        Accept ( WRITE );     
        _Emit ( I_WRITE );
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
/*                  can Emit I_ADD or I_SUB                                 */
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
/*                  can Emit I_MULT or I_DIV                                */
/*--------------------------------------------------------------------------*/

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
/*                  Can emit I_LOAD I_LOADI I_NEG                           */
/*                  Calls lookup symbol (see side effects)                  */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseTerm( void )
{
        SYMBOL *var; /*for looking up potential identifer */
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
                        Emit( I_LOADA,var->address ); /*Var so load address */
                }
                else
                {
                        fprintf( stderr, "Semantic error: variable \"%s\" not delcared\n",CurrentToken.s );
                        KillCodeGeneration();
                }        
                Accept ( IDENTIFIER );
                break;
        case INTCONST:
                Emit ( I_LOADI,CurrentToken.value); /*Const so load immediate */
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
/*    Outputs:      int                                                     */
/*                                                                          */
/*    Returns:      BackPatchAddr for if+while statements                   */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                  Can emit I_SUB+A branching statement                    */
/*--------------------------------------------------------------------------*/
      
            
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
/*  End of compiler support routines follow.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Accept:  Takes an expected token name as argument, and if the current   */
/*           lookahead matches this, advances the lookahead and returns.    */
/*                                                                          */
/*           If the expected token fails to match the current lookahead,    */
/*           this routine reports a syntax error and attempts to recover    */
/*           but puts the error message on the standard output              */
/*           and on the listing file                                        */
/*                                                                          */
/*                                                                          */
/*    Inputs:       Integer code of expected token                          */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: If successful, advances the current lookahead token     */
/*                  "CurrentToken". Otherwise will continue to advance      */
/*                  The lookahead to attempt to resync                      */
/*                  Will exit program if it sees endofinput                 */
/*--------------------------------------------------------------------------*/

PRIVATE void Accept( int ExpectedToken )
{
        static int recovering = 0;


        if ( CurrentToken.code == ENDOFINPUT)  /* Short circuit/exit  if we reach the end of the input program */
        {    
                if (ExpectedToken == ENDOFINPUT) 
                {
                        Error("Parsing ends here in this program \n", CurrentToken.pos);            
                        fclose( InputFile );
                        fclose( ListFile );
                        fclose( CodeFile );
                        exit ( EXIT_SUCCESS ); 
                }
                else
                {       
                        Error("Resync at end of file \n", CurrentToken.pos);            
                        fclose( InputFile );
                        fclose( ListFile );                        
                        fclose( CodeFile );
                        
                        exit ( EXIT_FAILURE ); 
                }
        }
        if (recovering) 
        {
                while ( CurrentToken.code != ExpectedToken && CurrentToken.code != ENDOFINPUT)
                { 
                        CurrentToken = GetToken(); /*If we are recovering advance until we get something we expect */
                } 
                recovering = 0;
        }

        if ( CurrentToken.code != ExpectedToken )  
        {
                SyntaxError( ExpectedToken, CurrentToken );
                KillCodeGeneration(); /*don't write any broken code */
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
                        CurrentToken = GetToken(); /*If currenttoken not expected advance look ahead */
                }
        }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SetupSets  :  Initialise the first and follow sets for                  */
/*	 	          "sync" points in the parser for error recovery            */
/*                Currently sets for Statement,Program and ProcDeclartion   */
/*    Inputs:       None				                                    */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects:  (inits the 6 global variables with tokens)             */
/*--------------------------------------------------------------------------*/

PRIVATE void SetupSets( void )
{    
        InitSet( &StatementFS_aug, 6, IDENTIFIER, WHILE,IF, READ, WRITE, END );
        InitSet( &StatementFBS, 4, SEMICOLON, ELSE,ENDOFPROGRAM, ENDOFINPUT ); 
        InitSet ( &ProgramFS_aug,3, BEGIN, PROCEDURE, VAR ) ;
        InitSet ( &ProgramFBS,1,ENDOFINPUT );
        InitSet ( &ProcFS_aug,3,BEGIN,PROCEDURE,VAR);
        InitSet ( &ProcFBS,2, BEGIN,ENDOFINPUT);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MakeSymbolTableEntry  : Makes an entry in the symbol table for          */
/*	 	          a variable or procedure name/parameter name               */
/*          if valid it will add the name to the string table               */
/*    Inputs:       symtype : the type e.g variable/procedure               */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: -Exits if there's a syntax error or other error         */
/*   -Increments global variablevaraddress if it adds a var to symbol table */
/*--------------------------------------------------------------------------*/

PRIVATE void MakeSymbolTableEntry( int symtype )
{
        SYMBOL *oldsptr;
        SYMBOL *newsptr;
        int hashindex;
        char *cptr;
        if (CurrentToken.code == IDENTIFIER)
        {   /*Put in hashindex, hashing calculated in other functions in strtab */
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
                                fclose ( InputFile );
                                fclose ( ListFile );
                                fclose ( CodeFile );
                                exit ( EXIT_FAILURE );
                        }
                        else
                        {
                                if ( oldsptr == NULL ) PreserveString(); /*get ready to make the entry */
                                newsptr->scope = scope;
                                newsptr->type = symtype;
                                if (symtype == STYPE_VARIABLE )
                                {
                                        newsptr->address = varaddress;
                                        varaddress++; /*Global for assigning global variables on the stack */
                                }
                                else
                                {
                                        newsptr->address = -1; /*Proc calling not handled in comp */
                                }

                        }


                }
                else
                {		   
                        fprintf( stderr, "Semantic error:  Variable \"%s\" already delcared\n",oldsptr->s );
                        KillCodeGeneration();
                }

        }



}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* LookupSymbol: Checks if identifer was declared earlier in symbol table   */
/*    Inputs:       None				                                    */
/*    Outputs:      Symbol: pointer to the symbol it finds or NULL          */
/*                                                                          */
/*    Returns:      sptr which is the same as above                         */
/*                                                                          */
/*    Side Effects:  Exits if undeclared identifier                         */
/*--------------------------------------------------------------------------*/
PRIVATE SYMBOL *LookupSymbol ( void )
{
        SYMBOL *sptr;

        if (CurrentToken.code == IDENTIFIER )
        {
                sptr = Probe(CurrentToken.s,NULL);
                if ( sptr == NULL )
                {
                        Error("Identifer not declared", CurrentToken.pos);
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
/*              associated input listing files, and code files.             */
/*                                                                          */
/*    Note that this routine modifies the globals "InputFile" and           */
/*    "ListingFile", and "CodeFile".                                        */
/*    It returns 1 ("true" in C-speak) if the input, code and               */
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
/*    Side Effects: If successful, modifies globals "InputFile", "CodeFile" */
/*                  and  "ListingFile".                                     */
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


