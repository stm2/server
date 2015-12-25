#!/usr/bin/env python


"""
Pandoc filter to convert mediawiki redirects to text
"""

from pandocfilters import toJSONFilter, walk, Str, Para
from ewiki import elem
import types


DEBUG = False

count=0
target=None

def mwiki_redirect(key, value, format, meta):
  global target
  global count
  count += 1
  if DEBUG:
    print ".."+key+"."+str(value)

  if key == 'Link' and value[1][1] == 'wikilink':
    if DEBUG:
      print (key+"."+value[1][0])
    target=value[1][0]
    
  return None


def mwiki_redirect2(key, value, format, meta):
  global count
  if count > 0:
    count-=1
  if DEBUG:
    print (key+"."+str(count))
  if count == 0:
    basic = mwiki_redirect(key, value, format, meta)
    if basic == None:
      if DEBUG:
        print ("walk")
      elem(key, walk(value, mwiki_redirect, format, meta))
      return [Para([Str(target)])]
    else:
      return []

if __name__ == "__main__":
  toJSONFilter(mwiki_redirect2)
