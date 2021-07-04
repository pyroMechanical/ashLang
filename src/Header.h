#pragma once
/*
Ashlang Grammar

		<program> ::= <declaration> "EOF_"
		
		<declaration> ::= <typeDecl> | <funcDecl> | <varDecl> | <statement>

		<typeDecl> ::= "def" <identifier> "{" <parameters> "}" ";"

		<funcDecl> ::= <identifier>? <identifier> <identifier> "(" <parameters> ")" <block> ";"

		<varDecl> ::= <identifier>? <identifier> <identifier> ":" <expression> ";"

		<statement> ::= <expressionStmt> | <forStmt> | <ifStmt> | <returnStmt> | <whileStmt> | <block>

		<expressionStmt> ::= <expression> ";"

		<forStmt> "for" "(" <expression>? ";" <expression>? ";" <expression>? ")" <statement> ";"
*/