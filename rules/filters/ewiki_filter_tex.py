#!/usr/bin/env python
# encoding: utf-8

"""
Pandoc filter to convert mediawiki elements to html elements.
"""

from pandocfilters import toJSONFilter, RawInline, RawBlock, Link, walk, Para, Str
import ewiki
import logging, re, string

next_pattern = re.compile('\{\{weiter\|(link=)*([^|]*)(\|text=(.*))*\}\}')
category_pattern = re.compile('(Kategorie|Category):(.*)')
link_pattern = re.compile('([^#]*)(#(.*))*')

next_chapter_link = 'Weiter: '
next_chapter_title = "N&auml;chstes Kapitel: "

count = 0
meta_infos = None
'''Set to file name, e.g. html0.log, for debugging'''
DEBUG=None

if DEBUG != None:
    logging.basicConfig(filename=DEBUG,filemode='w',level=logging.DEBUG)

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
  logging.debug(link+"("+id_prefix+"):")
  logging.debug('rrrr'+str(ewiki.redirects))
  if m.group(1) == "":
    logging.debug(id_prefix+"####"+('---' if m.group(3)==None else m.group(3)))
    result = '#' + ewiki.link_id(m.group(3) or '', id_prefix)
  else:
    target = m.group(1).replace('_', ' ')
    subject = None
    target = ewiki.find_redirect(target)
    target = target.replace('_', ' ')
    if '#' in target:
        subject = target[string.find(target, '#')+1:]
        target = target[:string.find(target, '#')]
    else:
        subject = m.group(3)

    logging.debug(target+"###"+('---' if subject==None else subject))

    result = "#" + ewiki.link_id(subject or "", target)

  logging.debug(result)
  return result

def mwiki_specials(key, value, format, meta):
  global count
  count += 1
  if DEBUG:
    logging.debug(".."+key+"."+str(count))
  if key == 'RawInline' and value[0] == 'html' and value[1] == '<noinclude>':
    return RawInline('html', '<div class="mw-noinclude">')
  if key == 'RawInline' and value[0] == 'html' and value[1] == '</noinclude>':
    return RawInline('html', '</div>')

  if (key == 'RawBlock' or key == 'RawInline') and value[0] == 'mediawiki':
    m = next_pattern.match(value[1])
    if m:
      return []

  if key == 'Link' and value[2][1] == 'wikilink':
    if DEBUG:
      logging.debug((key+"."+value[2][0]))
    m = category_pattern.match(value[2][0])
    if m:
      return []
    else:
      return Link(value[0], value[1], [convert_internal_link(value[2][0]), ""])
      
  if key == "Header":
      value[1][0] = ewiki.link_id(value[1][0], id_prefix)

  return None

def mwiki_specials2(key, value, format, meta):
  global count, id_prefix
  id_prefix = unicode(meta['id-prefix']['c'])
  ewiki.parse_redirects(unicode(meta['redirects']['c']))
  if count > 0:
    count-=1
  if DEBUG:
    logging.debug(key+"."+str(count))
  if count == 0:
    basic = mwiki_specials(key, value, format, meta)
    if basic == None:
      if DEBUG:
        logging.debug ("walk")
      subtree = ewiki.elem(key, walk(value, mwiki_specials, format, meta))
      if meta_infos:
        subtree = [subtree]
        subtree.extend(meta_infos)
      return subtree
    else:
      return basic


if __name__ == "__main__":
  toJSONFilter(mwiki_specials2)

