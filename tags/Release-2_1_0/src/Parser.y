%{
#include "SearchExpr.h"
#include "Scanner.h.in"
#include "Scanner.h"
#include "OtherFunctions.h"

#include "libs/common/StringFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern wxArrayString _astrParserErrors;

void ParsedSearchExpression(const CSearchExpr* pexpr);
int yyerror(const char* errstr);
int yyerror(wxString errstr);

%}

%union {
	wxString*		pstr;
	CSearchExpr*	pexpr;
}

%token TOK_STRING
%token TOK_AND TOK_OR TOK_NOT
%token TOK_ED2K_LINK

%type <pexpr> searchexpr and_strings
%type <pstr> TOK_STRING TOK_ED2K_LINK

%left TOK_OR
%left TOK_AND
%left TOK_NOT

%%
/*-------------------------------------------------------------------*/

action			: searchexpr
					{
						ParsedSearchExpression($1);
						delete $1;
						return 0;
					}
				| TOK_ED2K_LINK
					{
						CSearchExpr* pexpr = new CSearchExpr(*$1);
						ParsedSearchExpression(pexpr);
						delete pexpr;
						delete $1;
						return 0;
					}
				/* --------- Error Handling --------- */
				| searchexpr error
					{
						yyerror(wxT("Undefined search expression error"));
						delete $1;
						return 1;
					}
				;


searchexpr		: and_strings
				| searchexpr TOK_AND searchexpr
					{
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_AND);
						pexpr->Add($1);
						pexpr->Add($3);
						$$ = pexpr;
						delete $1;
						delete $3;
					}
				| searchexpr TOK_OR searchexpr
					{
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_OR);
						pexpr->Add($1);
						pexpr->Add($3);
						$$ = pexpr;
						delete $1;
						delete $3;
					}
				| searchexpr TOK_NOT searchexpr
					{
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_NOT);
						pexpr->Add($1);
						pexpr->Add($3);
						$$ = pexpr;
						delete $1;
						delete $3;
					}
				| '(' searchexpr ')'
					{
						$$ = $2;
					}
				/* --------- Error Handling --------- */
				| searchexpr TOK_OR error
					{
						yyerror(wxT("Missing right operand for OR on search expression"));
						delete $1;
						return 1;
					}
				| searchexpr TOK_NOT error
					{
						yyerror(wxT("Missing operand for NOT on search expression"));
						delete $1;
						return 1;
					}
				| '(' error
					{
						yyerror(wxT("Missing left parenthesis on search expression"));
						return 1;
					}
				| '(' searchexpr error
					{
						yyerror(wxT("Missing closing parenthesis on search expression"));
						delete $2;
						return 1;
					}
				| TOK_AND error
					{
						yyerror(wxT("Missing left operand for AND on search expression"));
						return 1;
					}
				| TOK_OR error
					{
						yyerror(wxT("Missing left operand for OR on search expression"));
						return 1;
					}
				| TOK_NOT error
					{
						yyerror(wxT("Missing left operand for NOT on search expression (?)"));
						return 1;
					}
				;

and_strings		: TOK_STRING
					{
						$$ = new CSearchExpr(*$1);
						delete $1;
					}
				| and_strings TOK_STRING
					{
						/*$1->Concatenate($2);
						delete $2;*/
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_AND);
						pexpr->Add($1);
						pexpr->Add(*$2);
						$$ = pexpr;
						delete $1;
						delete $2;
					}
				;

%%

int yyerror(const char* errstr)
{
	// Errors created by yacc generated code
	//yyerror ("syntax error: cannot back up");
	//yyerror ("syntax error; also virtual memory exhausted");
	//yyerror ("syntax error");
	//yyerror ("parser stack overflow");

	_astrParserErrors.Add(char2unicode(errstr));
	return EXIT_FAILURE;
}

int yyerror(wxString errstr)
{
	_astrParserErrors.Add(errstr);
	return EXIT_FAILURE;
}
