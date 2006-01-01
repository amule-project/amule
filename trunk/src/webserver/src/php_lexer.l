%{
//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (C) 2005-2006Froenchenko Leonid ( lfroen@amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <string.h>

#include "php_syntree.h"
#include "php_parser.h"

void php_set_input_buffer(char *buf, int len)
{
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_scan_bytes(buf, len);
}

int yywrap();

int yywrap()
{
	return 1;
}

%}

%x BLOCK_COMMENT
%x LINE_COMMENT

%option stack
%option yylineno

DEC_NUM			[0-9]+
FLOANUM	([0-9]*[\.][0-9]+)|([0-9]+[\.][0-9]*)
HEX_NUM		"0x"[0-9a-fA-F]+

IDENT   [a-zA-Z_][a-zA-Z0-9_]*

EOL ("\r"|"\n"|"\r\n")

%%

"<?php"			return START_SCRIPT;
"?>"			return END_SCRIPT;

"exit"|"die"	return EXIT;

"function"		return FUNCTION;

"const"			return CONST;

"return"		return RETURN;

"if"			return IF;
"else"			return ELSE;
"elseif"		return ELSEIF;
"endif"			return ENDIF;

"while"			return WHILE;
"endwhile"		return ENDWHILE;
"do"			return DO;

"for"			return FOR;
"endfor"		return ENDFOR;
"foreach"		return FOREACH;
"as"			return AS;
"endforeach"	return ENDFOREACH;

"switch"		return SWITCH;
"endswitch"		return ENDSWITCH;
"case"			return CASE;
"default"		return DEFAULT;
"break"			return BREAK;

"continue"		return CONTINUE;

"echo"			return ECHO;
"print"			return PRINT;

"new"			return NEW;

"->"			return OBJECT_OPERATOR;

"list"			return LIST;
"array"			return ARRAY;

"++"			return INC;
"--"			return DEC;
"=>"			return HASH_ASSIGN;

"or"			return LOG_OR;
"xor"			return LOG_XOR;
"and"			return LOG_AND;
"||"			return BOOLEAN_OR;
"&&"			return BOOLEAN_AND;

"+="			return PLUS_EQ;
"-="			return MINUS_EQ;
"*="			return MUL_EQ;
"/="			return DIV_EQ;
".="			return CONCAT_EQ;
"%="			return MOD_EQ;
"&="			return AND_EQ;
"|="			return OR_EQ;
"^="			return XOR_EQ;
"<<="			return SL_EQ;
">>="			return SR_EQ;

"==="			return IS_IDENTICAL;
"!==="			return IS_NOIDENTICAL;
"=="			return IS_EQ;
"!="			return IS_NOEQUAL;
"<="			return IS_SMALLER_OR_EQ;
">="			return IS_GREATER_OR_EQ;

"global"		return GLOBAL;
"static"		return STATIC;

"("[ \t]*("int"|"integer")[ \t]*")" {
        return INT_CAST;
}

"("[ \t]*("real"|"double"|"float")[ \t]*")" {
        return DOUBLE_CAST;
}

"("[ \t]*"string"[ \t]*")" {
        return STRING_CAST;
}

"("[ \t]*("bool"|"boolean")[ \t]*")" {
        return BOOL_CAST;
}

"#"|"//" {
		BEGIN(LINE_COMMENT);
		yymore();
	}

<LINE_COMMENT>[^\n\r]+ {
		yymore();
	}

<LINE_COMMENT>{EOL} {
		BEGIN(INITIAL);
	}

"/*" {
		BEGIN(BLOCK_COMMENT);
		yymore();
	}

<BLOCK_COMMENT>"*/" {
		BEGIN(INITIAL);
	}

"$"{IDENT} {
        yylval.exp_node = get_var_node(yytext+1);
        return VARIABLE;
	}

{IDENT} {
        strcpy(yylval.str_val, yytext);
        return IDENT;
	}

{HEX_NUM} {
	int val;
	sscanf(yytext, "0x%x", &val);
	yylval.exp_node = make_const_exp_dnum(val);
	return DNUMBER;
	}

{DEC_NUM} {
	yylval.exp_node = make_const_exp_dnum(atoi(yytext));
	return DNUMBER;
	}

{FLOANUM} {
	yylval.exp_node = make_const_exp_fnum(atof(yytext));
	return FNUMBER;
	}

\"(\\.|[^\\"])*\" {
		yytext[strlen(yytext)-1] = 0;
		yylval.exp_node = make_const_exp_str(yytext+1, 1);
		return STRING;
}

\'(\\.|[^\\'])*\' {
		yytext[strlen(yytext)-1] = 0;
		yylval.exp_node = make_const_exp_str(yytext+1, 0);
		return STRING;
}

[ \t\v\n\r\f]		{  }

. {
		return yytext[0];
}
