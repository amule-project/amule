%{
#include <stdio.h>
#include "SearchExpr.h"
#include "Parser.hpp"
#include "ED2KLink.h"
#include <wx/string.h>

#include "libs/common/StringFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define YY_NEVER_INTERACTIVE 1

extern int yyerror(const char* errstr);
extern int yyerror(wxString errstr);

#define	YY_INPUT			ReadLexBuff
#define	YY_FATAL_ERROR		FatalLexError

static void ReadLexBuff(char* pcBuff, int& riResult, size_t uMaxSize);
static void FatalLexError(yyconst char msg[]);

static char* _pszLexBuff;
static char* _pszLexStr;

%}

%option noyywrap

keywordchar		[^ \"()]

%%

[ ]				{ /* Skip blanks. */ }
"OR"			{ return TOK_OR; }
"AND"			{ return TOK_AND; }
"NOT"			{ return TOK_NOT; }

"ed2k::"[a-fA-F0-9]{32}	{
					yylval.pstr = new wxString(UTF82unicode(yytext));
					return TOK_ED2K_LINK;
				}

{keywordchar}*  {
					yylval.pstr = new wxString(UTF82unicode(yytext));
					return TOK_STRING;
                }

"\""			{
					int l = 128;
					char* psz = (char*)malloc(l);
					int i = 0;
					int c;
					while ((c = yyinput()) != '\"')
					{
						if (c == EOF || c == '\n'){
							unput(c);
							yyerror(wxT("Search expression error: unterminated string"));
							break;
						}
						if (c == '\\'){		/*Escape sequence*/
							switch (c = yyinput())
							{
							case '\n':
								continue;
							case 't':		/*Tab*/
								c = '\t';
								break;
							case 'n':		/*Linefeed*/
								c = '\n';
								break;
							case 'f':		/*Formfeed*/
								c = '\f';
								break;
							case 'r':		/*Carriage return*/
								c = '\r';
								break;
							case '\\':		/*Backslash*/
								c = '\\';
								break;
							case '"':		/*Double quotation mark*/
								c = '\"';
								break;
							case '\'':		/*Single quotation mark*/
								c = '\'';
								break;
							case '?':		/*Question mark*/
								c = '\?';
								break;
							case 'v':		/*Vertical Tab*/
								c = '\v';
								break;
							case 'a':		/*Alert*/
								c = '\a';
								break;
							case 'b':		/*Backspace*/
								c = '\b';
								break;
							case 'x':		/*Hexadecimal number*/
								{
									int n, octv;
									for (n = 1, octv = 0; n <= 3; n++) {
										if ((c = yyinput()) >= '0' && c <= '9')
											c -= '0';
										else if (c >= 'a' && c <= 'f')
											c = (c - 'a') + 10;
										else if (c >= 'A' && c <= 'F')
											c = (c - 'A') + 10;
										else
											break;
										octv = octv * 16 + c;
									}
									unput(c);
									if (n == 1)
										c = 'x';
									else
										c = octv;
								}
								break;
							}
						} else {
							if ((unsigned char)c >= 0x80/* && IsDBCSLeadByte(yytext[0]) */){
								psz[i++] = (unsigned char)c;
								if (i >= l){
									psz = (char*)realloc(psz, l += 128);
									if (psz == NULL){
										yyerror("Less memory for string");
										break;
									}
								}
								c = yyinput();
							}
						}

						psz[i++] = (unsigned char)c;
						if (i >= l){
							psz = (char*)realloc(psz, l += 128);
							if (psz == NULL){
								yyerror("Less memory for string");
								break;
							}
						}
					}
					psz[i] = '\0';
					yylval.pstr = new wxString(UTF82unicode(psz));
					free(psz);
					return TOK_STRING;
				}

.				{ return yytext[0]; }

%%

static void ReadLexBuff(char* pcBuff, int& riResult, size_t uMaxSize)
{
	wxASSERT( _pszLexBuff != NULL );
	
	if (_pszLexBuff == NULL) {
		YY_FATAL_ERROR("Input in flex scanner failed");
	}

	wxASSERT( sizeof(YY_CHAR) == sizeof(char) );
	size_t uCharsInBuff = strlen(_pszLexBuff);
	size_t uCharsRead = min(uMaxSize, uCharsInBuff);
	riResult = uCharsRead;
	memcpy(pcBuff, _pszLexBuff, uCharsRead);
	_pszLexBuff += uCharsRead;
}

static void FatalLexError(yyconst char msg[])
{
#ifdef _CONSOLE
	printf("Fatal error in flex scanner: %s\n", msg);
#else
	printf("Fatal error in flex scanner: %s\n", msg);
#endif
}

void LexInit(const wxString& pszInput)
{

	_pszLexStr = strdup(unicode2UTF8(pszInput));
	_pszLexBuff = _pszLexStr;
}

void LexFree()
{
	yyleng = 0;
	yytext = NULL;
	yyin = NULL;
	yyout = NULL;
	yy_hold_char = '\0';
	yy_n_chars = 0;
	yy_c_buf_p = NULL;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_init = 1;
	yy_start = 0;
	yy_did_buffer_switch_on_eof = 0;
	yy_last_accepting_state = 0;
	yy_last_accepting_cpos = NULL;
	
#if YY_STACK_USED
	yy_start_stack_ptr = 0;
	yy_start_stack_depth = 0;
	yy_start_stack = NULL;
#endif

	free(_pszLexStr);
}
