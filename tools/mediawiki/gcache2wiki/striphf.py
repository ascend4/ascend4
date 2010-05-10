#!/usr/bin/env python
# Filter to strip the header and footer stuff from the cached page

from BeautifulSoup import *
import sys

print "Reading file",sys.argv[1],"..."

f = open(sys.argv[1]).read()

print "Parsing whole page..."

s = BeautifulSoup(f);

title = s.title.string

if(title[-9:] != " - ASCEND"):
	print "Incorrect title '%s'" % s.title.string
	sys.exit(1)

title = title[:-9]
print "  page title = '%s'" % title

import re
r = re.compile("<!-- start content -->(.*)<!-- end content -->",re.S);

pagecontent = r.search(f).group(1)

print "Parsing page content..."
s1 = BeautifulSoup(pagecontent)

def strip_highlight(soup):
	for a1 in soup.findAll('p'):
		if a1.find('style',{'type':"text/css"}):
			n1 = a1.nextSibling
			a1.extract()
			n2 = n1.nextSibling
			if n1.find('p'):
				n1.extract()
			n3 = n2.nextSibling
			n2.extract()
			n4= n3.nextSibling
			n3.extract()
			pre = n4.nextSibling
			n4.extract()
			print "CODE LISTING"
			if pre.name != 'pre':
				print "ERROR parsing syntax-highlighting:",pre
				sys.exit(1)
			for x in pre.findAll('b',{'style':True}):
				print "BOLD:",x,"-->",str(x.string)
				x1 = NavigableString(str(x.string))
				print "X1:",x1
				x.replaceWith(x1)

			print "\n\nPRE:",pre,"\n\n"
	
			for x in pre.findAll('span',{'class':True}):
				x1 = NavigableString(str(x.string))
				x.replaceWith(x1)

			print "\n\nPRE2:",pre,"\n\n"

			t = Tag(soup,"src",[("lang",'a4c')])
			t.insert(0, NavigableString(str(pre.string)))
			pre.replaceWith(t)
			print str(pre)
			sys.exit(1)

def strip_anchors(soup):
	for a1 in soup.findAll('a',{'name':True}):
		a1.extract()

def wikify_headings(soup):
	def h1(tag):
		if not tag.name in ['h1','h2','h3','h4','h5','h6']:
			return False
		if tag.span['class'] == 'mw-headline':
			return True
		return False
	for h in soup.findAll(h1):
		level = int(str(h.name)[1:])
		h2 = NavigableString("="*level + h.span.string + "="*level)
		h.replaceWith(h2)

def wikify_paragraphs(soup):
	for p in soup.findAll('p'):
		if p.string is None:
			p.replaceWith(NavigableString("\n"))
		else:
			p.replaceWith(NavigableString("\n" + p.string + "\n"))

strip_highlight(s1)

print str(s1)
sys.exit(1)

strip_anchors(s1)
wikify_headings(s1)
wikify_paragraphs(s1)


