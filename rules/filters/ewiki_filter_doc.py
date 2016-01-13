#!/usr/bin/env python

"""
Pandoc filter to convert <noinclude> elements to <div class='noinclude'>
"""

from pandocfilters import toJSONFilter, RawInline, RawBlock, Link, walk, Para
import re

next_pattern = re.compile('\{\{weiter\|(.*)\}\}')
category_pattern = re.compile('Kategorie:(.*)')


count = 0
meta_infos = None

def meta_info(info):
    global meta_infos
    if meta_infos:
      meta_infos.append(RawBlock('mediawiki', info))
    else:
      meta_infos = [RawBlock('mediawiki', info)]


def mwiki_specials(key, value, format, meta):
  if key == 'RawInline' and value[0] == 'html' and value[1] == '<noinclude>':
    return RawInline('html', '<div class="mw-noinclude">')
  if key == 'RawInline' and value[0] == 'html' and value[1] == '</noinclude>':
    return RawInline('html', '</div>')

  if key == 'RawInline' and value[0] == 'mediawiki':
    m = next_pattern.match(value[1])
    if m:
      meta_info(value[1])
      return []

  if key == 'Link' and value[2][1] == 'wikilink':
    m = category_pattern.match(value[2][0])
    if m:
      meta_info('{{Kategorie|' + m.group(1) + '}}')
      return []

  return None

def nothing(k, v, f, m):
  return None

def mwiki_specials2(key, value, format, meta):
  global count, meta_infos
  if count == 0 and key == 'Para':
      count += 1
      subtree = Para(walk(value, mwiki_specials, format, meta))
      count -= 1
      if meta_infos:
        subtree = [subtree]
        subtree.extend(meta_infos)
      return subtree

if __name__ == "__main__":
  toJSONFilter(mwiki_specials2)

