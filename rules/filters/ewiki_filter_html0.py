#!/usr/bin/env python


"""
Pandoc filter to convert mediawiki elements to html elements.
"""

from pandocfilters import toJSONFilter, RawInline, RawBlock, Link, walk, Para, Str
import ewiki
import logging, sys, re, string


next_pattern = re.compile('\{\{weiter\|(link=)*([^|]*)(\|text=(.*))*\}\}')
category_pattern = re.compile('(Kategorie|Category):(.*)')
link_pattern = re.compile('([^#]*)(#(.*))*')

next_chapter_link = 'Weiter: '
next_chapter_title = "N&auml;chstes Kapitel: "

count = 0
meta_infos = None
DEBUG='html0.log'

if DEBUG != None:
    logging.basicConfig(filename=DEBUG,filemode='w',level=logging.DEBUG)

def meta_info(info):
    global meta_infos
    if meta_infos:
      meta_infos.append(RawBlock('mediawiki', info))
    else:
      meta_infos = [RawBlock('mediawiki', info)]

id_prefix = ''

def convert_internal_link(link):
  m = link_pattern.match(link.encode('utf-8'))
  if m.group(1) == "":
    return '#' + ewiki.link_id(m.group(3) or '', id_prefix)
  else:
    target = m.group(1).replace('_', ' ')
    subject = None
    target = ewiki.find_redirect(target)
    if '#' in target:
        subject = target[string.find(target, '#')+1:]
        target = target[:string.find(target, '#')]
    else:
        if m.group(2) != None:
            subject = m.group(2)

    logging.debug(link.encode('utf-8')+":"+target+"###"+('---' if subject==None else subject))
    logging.debug('rrrr'+str(ewiki.redirects))

    if subject == None:
      return ewiki.url_encode(ewiki.url_encode(target)) + ".html"
    else:
      return ewiki.url_encode(ewiki.url_encode(target)) + ".html" + '#' + ewiki.link_id(subject or "", target+".")

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
      link = m.group(2)
      if m.group(3) == None:
        text = next_chapter_title + link
      else:
        text = next_chapter_title + m.group(4)
      if key == 'RawBlock':
        return Para( [ Link( [ Str(next_chapter_link + link) ], [ convert_internal_link(link), text] )])
      else:
        return Link( [ Str(next_chapter_link + link) ], [ convert_internal_link(link), text] )
  if key == 'Link' and value[1][1] == 'wikilink':
    if DEBUG:
      logging.debug((key+"."+value[1][0]))
    m = category_pattern.match(value[1][0])
    if m:
      meta_info('{{Kategorie|' + m.group(2) + '}}')
      return []
    else:
      return Link(value[0], [convert_internal_link(value[1][0]), ""])

  return None

def mwiki_specials2(key, value, format, meta):
  global count, meta_infos, id_prefix
  if 'id-prefix' not in meta:
    id_prefix = 'ewiki-foo.'
  else:
    id_prefix = str(meta['id-prefix']['c'])
  ewiki.parse_redirects(str(meta['redirects']['c'].encode('utf-8')))
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

