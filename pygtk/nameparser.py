#!/usr/bin/env python

import ply.lex as lex
import ply.yacc as yacc

class Parser:
	tokens = ()

	def __init__(self,instance):
		# Build the lexer and parser
		self.instance = instance
		lex.lex(module = self)
		yacc.yacc(module = self)

	def run(self,s):
		if not s: return
		return yacc.parse(s)

class NameParser(Parser):
	def __init__(self,instance):
		Parser.__init__(self,instance)

	tokens = (
		'ID',
	)

	# Tokens
	t_ID = r'[a-zA-Z_][a-zA-Z0-9_]*'

	def t_error(self, t):
		print("Illegal character '%s'" % t.value[0])
		t.lexer.skip(1)

	# Parsing rules
	def p_name(self, p):
		'name : ID'
		p[0] = p[1]

	def p_error(self, p):
		print("Syntax error at '%s'" % p.value)

# Test the parser:
if __name__ == '__main__':
    np = NameParser("instance")
    s = raw_input('Enter the name: ')
    print np.run(s)