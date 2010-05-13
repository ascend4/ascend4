#!/usr/bin/env python
# Filter to strip the header and footer stuff from the cached page

from BeautifulSoup import *
import sys

def replace_templates(soup):
	for t in soup.findAll("div",{"id":"task","class":"notice metadata"}):
		t.replaceWith(NavigableString("{{task}}"))

def strip_contents(soup):
	c = soup.find('table',{'id':'toc','class':'toc','summary':'Contents'})
	if c:
		c.extract()

def strip_wiki_comments(soup):
	msg1 = "NewPP limit report"
	l1 = len(msg1)
	msg2 = "Saved in parser cache"
	l2 = len(msg2)
	#print "STRIPPING WIKI COMMENTS"
	def co(tag):
		if isinstance(tag, Comment):
			if tag.string.strip()[0:l1] == msg1 or tag.string.strip()[0:l2]==msg2:
				#print "COMMENT:",tag.string.strip()
				return True
		return False
	for c in soup.findAll(text=co):
		c.extract()
	

def strip_script(soup):
	for s in soup.findAll('script'):
		s.extract()

def strip_highlight(soup):
	for a1 in soup.findAll('p'):
		if a1.find('style',{'type':"text/css"}):
			n1 = a1.nextSibling
			if str(n1.string).strip() != "/* Highlighting theme definition: */":
				#print "NO MATCH"
				sys.exit(1)
			n2 = n1.nextSibling
			#print "REMOVING",str(a1)
			a1.extract()
			#print "REMOVING N1",str(n1)
			n1.extract()
			n3 = n2.nextSibling
			#print "REMOVING N2",str(n2)
			n2.extract()
			n4= n3.nextSibling
			#print "REMOVING N3",str(n3)
			n3.extract()
			pre = n4.nextSibling
			#print "REMOVING N4",str(n4)
			n4.extract()

			if pre.name != 'pre':
				#print "ERROR parsing syntax-highlighting:",pre
				sys.exit(1)
			for x in pre.findAll('b',{'style':True}):
				x1 = NavigableString(str(x.string))
				x.replaceWith(x1)
	
			for x in pre.findAll('span',{'class':True}):
				x1 = NavigableString(str(x.renderContents()))
				x.replaceWith(x1)

			t = Tag(soup,"src",[("lang",'a4c')])
			t.insert(0, NavigableString(str(pre.renderContents()).strip()))
			pre.replaceWith(t)

def strip_anchors(soup):
	for a1 in soup.findAll('a',{'name':True}):
		#print "ANCHOR:",a1
		a1.extract()

def wikify_headings(soup):
	for h in soup.findAll(['h1','h2','h3','h4','h5','h6']):
		if not h.find('span',{'class':'mw-headline'}):
			#print "HEADING: SKIPPING:",h
			continue
		#print "HEADING:",h
		level = int(str(h.name)[1:])
		h2 = NavigableString("\n" + "="*level + h.span.renderContents() + "="*level)
		h.replaceWith(h2)

def wikify_paragraphs(soup):
	for p in soup.findAll('p'):
		#print "PARA",str(p)
		if p.renderContents() is None:
			p.replaceWith(NavigableString("\n"))
		else:
			p.replaceWith(NavigableString("\n" + p.renderContents()))

def strip_printfooter(soup):
	soup.find('div',{'class':'printfooter'}).extract()

def strip_wikicomments(soup):
	pass

def wikify_categories(soup):
	cats = soup.find("div",{"id":"catlinks"})
	if not cats:
		return
	r2 = re.compile("/[A-Z][a-z_0-9-]*")
	cc = []
	for a in cats.findAll("a"):
		if str(a['href']) == "/Special:Categories":
			#print "CATEGORIES LINK ignored"
			a.extract()
		elif r2.match(a['href']):
			t = NavigableString("[[" + a['href'][1:] + "]]\n")
			#print "  categ:",t.strip()
			cc.append(t)
	#print "CATS:",cc
	#cats.replace(cc)
	for c in cc:
		cats.parent.append(c)
	cats.extract()

def wikify_images(soup):
	for a in soup.findAll("a",{'class':'image'}):
		if a.img:
			if a.img['alt'][0:6] == "Image:":
				#print "IMG1",a.img['alt'][6:]
				a1 = NavigableString("[[Image:" + a.img['alt'][6:] + "]]")
				#print "-->",a1
				a.replaceWith(a1)
			elif a['href'][0:6] == "/File:":
				#print "IMG",a['href'][6:]
				a1 = NavigableString("[[Image:" + a['href'][6:] + "]]")
				a.replaceWith(a1)
				#print "-->",a1
			else:
				sys.stderr.write("CAN'T PROCESS IMAGE LINK %s\n" % str(a))

def wikify_math(soup):
	for img in soup.findAll("img",{'class':'tex'}):
		s = "<math>" + img['alt'] + "</math>"
		#print "MATH:",s
		img1 = NavigableString(s)
		img.replaceWith(img1)
		#img.replaceWith(NavigableText(s))

def wikify_indents(soup):
	for dl in soup.findAll("dl"):
		s = ""
		for dd in dl.findAll("dd"):
			s += ":" + dd.renderContents() + "\n"
		dl1 = NavigableString(s)
		dl.replaceWith(dl1)		

def wikify_links(soup):
	rr1 = re.compile(" ")
	def linkified(s):
		s = rr1.sub("_",s)
		s = s[0:1].upper() + s[1:]
		return s

	r = re.compile("^http://")
	r2 = re.compile("/[A-Z][a-z_0-9-]*")
	r3 = re.compile(r"^http://ascendcode.cheme.cmu.edu/viewvc.cgi/code/(.*)$");
	r3trunk = re.compile(r"trunk/(.*)\?view=markup$")
	r3branch = re.compile(r"branches/([^/]+)/(.*)\?view=markup$")
	r3dir = re.compile(r"trunk/(.*)")
	for a in soup.findAll('a',{'href':True}):
		#print "LINK:",a.parent
		m3 = r3.match(a['href'])
		if m3:
			t1 = m3.group(1)
			m3 = r3trunk.match(t1)
			if m3:
				t = NavigableString("{{src|%s}}" % m3.group(1))
				a.replaceWith(t)
			else:
				m3 = r3branch.match(t1)
				if m3:
					t = NavigableString("{{srcbranch|%s|%s}}" % (m3.group(1),m3.group(2)))
					a.replaceWith(t)
				else:
					m3 = r3dir.match(t1)
					if m3:
						t = NavigableString("{{srcdir|%s}}" % m3.group(1))
						a.replaceWith(t)
					else:
						t = NavigableString("[" + a['href'] + " " + a.renderContents() + "]")
						a.replaceWith(t)
			#print "LINK:",t
		elif r.match(a['href']):
			if a['href'] == a.renderContents():
				t = NavigableString("[" + a['href'] + "]")
			else:
				t = NavigableString("[" + a['href'] + " " + a.renderContents() + "]")
			a.replaceWith(t)
			#print "LINK:",t

		elif r2.match(a['href']):
			if linkified(a.renderContents()) == a['href'][1:]:
				t = NavigableString("[[" + a.renderContents() + "]]")
			else:
				t = NavigableString("[[" + a['href'][1:] + "|" + a.renderContents() + "]]")
			a.replaceWith(t)
			#print "LINK:",t

def wikify_bold(soup):
	for b in soup.findAll("b"):
		#print "BOLD:",b
		b2 = NavigableString("'''" + b.renderContents() + "'''")
		b.replaceWith(b2)

def wikify_italics(soup):
	for i in soup.findAll("i"):
		i.replaceWith("''" + i.renderContents() + "''")


def wikify_list(l, prefix="*"):
	#print "WIKIFY L:",l.prettify()
	s = ""
	for tag in l.findAll(['ul','ol','li']):
		if tag.name == "ul":
			s += wikify_list(tag,prefix+"*")
		elif tag.name == "ol":
			s += wikify_list(tag,prefix+"#")
		elif tag.name == "li":
			# sometimes nested lists are incorrectly placed within a <li>
			#print "STUFF IN LI:"
			if tag.findAll(["ol","ul"]):
				for stuff in tag:
					if isinstance(stuff,Tag) and stuff.name == "ol":
						s += wikify_list(stuff,prefix + "#")
					elif isinstance(stuff,Tag) and stuff.name == "ul":
						s += wikify_list(stuff,prefix + "*")
					elif isinstance(stuff,NavigableString) and stuff.string.strip():
						s += "\n" + prefix + " " + stuff.string.strip()
			else:
				s += "\n" + prefix + " " + tag.renderContents().strip()


	#print "\n\nRESULT OF WIKIFY L:",s,"\n\n"

	return s

def wikify_lists(soup):
	# FIXME handle nested lists!
	for ul in soup.findAll("ul"):
		ul.replaceWith(NavigableString(wikify_list(ul,"*")))
	for ol in soup.findAll("ol"):
		ol.replaceWith(NavigableString(wikify_list(ol,"#")))

def wikify_tables(soup):
	for ta in soup.findAll("table"):
		s = '\n{| class="wikitable"\n'
		for tr in ta.findAll("tr"):
			s += "|-\n"
			for t in tr.findAll(["td",'th']):
				if t.name == "td":
					s += "| " + t.renderContents()
				else:
					s += "! " + t.renderContents()
		s += "|}"
		ta.replaceWith(NavigableString(s))


def html2wiki(html,wikiname):
	"""
	This is the main function that converts an HTML string into corresponding wiki syntax.
	It expects a full HTML page including header, footer, etc, not just the 'content' section
	of the page.

	@param html the raw 'page source' input (eg from Google Cache)
	@param wikiname the name of the wiki from which the content is derived
	"""
	s = BeautifulSoup(html)

	title = s.title.string

	if(title[-9:] != " - " + wikiname):
		print "Incorrect title '%s'" % s.title.string
		sys.exit(1)

	title = title[:-9]
	#print "  page title = '%s'" % title

	import re
	r = re.compile("<!-- start content -->(.*)<!-- end content -->",re.S);

	pagecontent = r.search(html).group(1)

	#print "Parsing page content..."
	s1 = BeautifulSoup(pagecontent)

	replace_templates(s1)
	strip_contents(s1)
	strip_wiki_comments(s1)
	strip_script(s1)
	strip_printfooter(s1)
	strip_highlight(s1)
	strip_anchors(s1)
	wikify_headings(s1)
	wikify_paragraphs(s1)
	wikify_categories(s1)
	s1 = BeautifulSoup(str(s1))
	wikify_bold(s1)
	s1 = BeautifulSoup(str(s1))
	wikify_italics(s1)
	s1 = BeautifulSoup(str(s1))
	wikify_images(s1)
	wikify_math(s1)
	wikify_indents(s1)

	wikify_links(s1)

	wikify_lists(s1)
	wikify_tables(s1)

	# TODO: do something to catch 'texhtml'?

	return str(s1)

if __name__=="__main__":
	sys.stderr.write("Reading file %s...\n"%sys.argv[1])
	f = open(sys.argv[1]).read()
	wikiname = "ASCEND"
	print html2wiki(f,wikiname)


