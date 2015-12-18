#!/usr/bin/env python


"""
Pandoc filter to convert mediawiki elements to html elements.
"""

from pandocfilters import toJSONFilter, RawInline, RawBlock, Link, walk, Para, Str
import re, string
from ewiki import link_id

next_pattern = re.compile('\{\{weiter\|(link=)*([^|]*)(\|text=(.*))*\}\}')
category_pattern = re.compile('(Kategorie|Category):(.*)')
link_pattern = re.compile('([^#]*)(#(.*))*')

next_chapter_link = 'Weiter: '
next_chapter_title = "N&auml;chstes Kapitel: "

count = 0
meta_infos = None
DEBUG = False

def meta_info(info):
    global meta_infos
    if meta_infos:
      meta_infos.append(RawBlock('mediawiki', info))
    else:
      meta_infos = [RawBlock('mediawiki', info)]

id_prefix = ''

"""link_pattern = re.compile('([^#]*)(#(.*))*')"""
def convert_internal_link(link):
  m = link_pattern.match(link)
  if m.group(1) == "":
    return "#" + link_id(m.group(3) or "", id_prefix)
  else:
    if m.group(2) == None:
      return "#" + link_id("", m.group(1)+".")
    else:
      return "#" + link_id(m.group(3) or "", m.group(1)+".")

def mwiki_specials(key, value, format, meta):
  global count
  count += 1
  if DEBUG:
    print ".."+key+"."+str(count)
  if key == 'RawInline' and value[0] == 'html' and value[1] == '<noinclude>':
    return RawInline('html', '<div class="mw-noinclude">')
  if key == 'RawInline' and value[0] == 'html' and value[1] == '</noinclude>':
    return RawInline('html', '</div>')

  if (key == 'RawBlock' or key == 'RawInline') and value[0] == 'mediawiki':
    m = next_pattern.match(value[1])
    if m:
      return []
  if key == 'Link' and value[1][1] == 'wikilink':
    if DEBUG:
      print (key+"."+value[1][0])
    m = category_pattern.match(value[1][0])
    if m:
      return []
    else:
      return Link(value[0], [convert_internal_link(value[1][0]), ""])

  return None

def nothing(k, v, f, m):
  return None

def elem(k, v):
  return {'t' : k, 'c': v}

def mwiki_specials2(key, value, format, meta):
  global count, id_prefix
  id_prefix = str(meta['id-prefix']['c'])
  if count > 0:
    count-=1
  if DEBUG:
    print (key+"."+str(count))
  if count == 0:
    basic = mwiki_specials(key, value, format, meta)
    if basic == None:
      if DEBUG:
        print ("walk")
      subtree = elem(key, walk(value, mwiki_specials, format, meta))
      if meta_infos:
        subtree = [subtree]
        subtree.extend(meta_infos)
      return subtree
    else:
      return basic


if __name__ == "__main__":
  toJSONFilter(mwiki_specials2)

