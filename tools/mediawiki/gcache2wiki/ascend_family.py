# -*- coding: utf-8  -*-              # REQUIRED
import config, family, urllib         # REQUIRED

class Family(family.Family):          # REQUIRED
    def __init__(self):               # REQUIRED
        family.Family.__init__(self)  # REQUIRED
        self.name = 'ascend'        # REQUIRED; replace with actual name

        self.langs = {                # REQUIRED
            'en': 'ascend.cheme.cmu.edu',  # Include one line for each wiki in family
        }

        # Translation used on all wikis for the different namespaces.
        # Most namespaces are inherited from family.Family.
        # Check the family.py file (in main directory) to see the standard
        # namespace translations for each known language.

        # You only need to enter translations that differ from the default.
        # There are two ways of entering namespace translations.
        # 1.  If you only need to change the translation of a particular
        #     namespace for one or two languages, use this format (example):
        self.namespaces[2]['en'] = u'User'
        self.namespaces[3]['en'] = u'User talk'

        # 2.  If you need to change the translation for many languages
        #     for the same namespace number, use this format (this is common
        #     for namespaces 4 and 5, because these are usually given a
        #     unique name for each wiki) (example):
        self.namespaces[4] = {
            '_default': [u'ASCEND', self.namespaces[4]['_default']], # REQUIRED
            # ETC.
        }

        # Wikimedia wikis all use "bodyContent" as the id of the <div>
        # element that contains the actual page content; change this for
        # wikis that use something else (e.g., mozilla family)
        self.content_id = "bodyContent"

        # On most wikis page names must start with a capital letter, but some
        # languages don't use this.  This should be a list of languages that
        # _don't_ require the first letter to be capitalized.  Example:
        # self.nocapitalize = ['foo', 'bar']

        # SETTINGS FOR WIKIS THAT USE DISAMBIGUATION PAGES:

        # Disambiguation template names in different languages; each value
        # must be a list, even if there is only one entry.  Example:
        # self.disambiguationTemplates['en'] = ['disambig', 'disambiguation']

        # The name of the category containing disambiguation
        # pages for the various languages. Only one category per language,
        # and without the namespace, so add things like:
        # self.disambcatname['en'] = "Disambiguation"

        # SETTINGS FOR WIKIS THAT USE INTERLANGUAGE LINKS:

        # attop is a list of languages that prefer to have the interwiki
        # links at the top of the page.  Example:
        # self.interwiki_attop = ['de', 'xz']

        # on_one_line is a list of languages that want the interwiki links
        # one-after-another on a single line.  Example:
        # self.interwiki_on_one_line = ['aa', 'cc']

        # String used as separator between interwiki links and the text
        self.interwiki_text_separator = '\r\n\r\n'

        # Which languages have a special order for putting interlanguage links,
        # and what order is it? If a language is not in interwiki_putfirst,
        # alphabetical order on language code is used. For languages that are in
        # interwiki_putfirst, interwiki_putfirst is checked first, and
        # languages are put in the order given there. All other languages are put
        # after those, in code-alphabetical order.
        self.interwiki_putfirst = {}

        # Languages in interwiki_putfirst_doubled should have a number plus a list
        # of languages. If there are at least the number of interwiki links, all
        # languages in the list should be placed at the front as well as in the
        # normal list.
        self.interwiki_putfirst_doubled = {}

        # Some families, e. g. commons and meta, are not multilingual and
        # forward interlanguage links to another family (wikipedia).
        # These families can set this variable to the name of the target
        # family.
        self.interwiki_forward = None

        # Which language codes no longer exist and by which language code
        # should they be replaced. If for example the language with code xx:
        # has been replaced by code yy:, add {'xx':'yy'} to obsolete.
        # If all links to language xx: should be removed, add {'xx': None}.
        self.obsolete = {}

        # SETTINGS FOR CATEGORY LINKS:

        # Languages that want the category links at the top of the page
        self.category_attop = []

        # languages that want the category links
        # one-after-another on a single line
        self.category_on_one_line = []

        # String used as separator between category links and the text
        self.category_text_separator = '\r\n\r\n'

        # When both at the bottom should categories come after interwikilinks?
        self.categories_last = []

        # SETTINGS FOR LDAP AUTHENTICATION
        # If your wiki uses:
        #  http://www.mediawiki.org/wiki/Extension:LDAP_Authentication.
        # then uncomment this line and define the user's domain required
        # at login.
        #self.ldapDomain = 'domain here'

    def protocol(self, code):
        """
        Can be overridden to return 'https'. Other protocols are not supported.
        """
        return 'http'

    def scriptpath(self, code):
        """The prefix used to locate scripts on this wiki.

        This is the value displayed when you enter {{SCRIPTPATH}} on a
        wiki page (often displayed at [[Help:Variables]] if the wiki has
        copied the master help page correctly).

        The default value is the one used on Wikimedia Foundation wikis,
        but needs to be overridden in the family file for any wiki that
        uses a different value.

        """
        return ''

    # IMPORTANT: if your wiki does not support the api.php interface,
    # you must uncomment the second line of this method:
    def apipath(self, code):
        # raise NotImplementedError, "%s wiki family does not support api.php" % self.name
        return '%s/api.php' % self.scriptpath(code)

    # Which version of MediaWiki is used? REQUIRED
    def version(self, code):
        # Replace with the actual version being run on your wiki
        return '1.13alpha'

    def code2encoding(self, code):
        """Return the encoding for a specific language wiki"""
        # Most wikis nowadays use UTF-8, but change this if yours uses
        # a different encoding
        return 'utf-8'

