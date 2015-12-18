#!/usr/bin/env python

import string
import urllib as ul
import htmlentitydefs as entity

chars_to_keep = [c for c in string.ascii_letters + string.digits + "_.-"]

def url_encode(name):
  return ul.quote(name)


def link_id(raw, prefix):
  global chars_to_keep

  def replace(a):
    if a in chars_to_keep:
      return a.lower()
    return "_"

  prefix = ''.join(map(replace, prefix))
  if not prefix.startswith("ewiki-"):
    prefix = "ewiki-" + prefix

  return prefix + ''.join(map(replace, raw))

def html_encode(text):
   t = u''
   for i in text:
     if ord(i) in entity.codepoint2name:
       name = entity.codepoint2name.get(ord(i))
       t += "&" + name + ";"
     else:
       t += i
   return t
