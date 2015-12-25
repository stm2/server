#!/usr/bin/env python

import string, json, logging
import urllib as ul
import htmlentitydefs as entity
import xml.etree.ElementTree as ET

chars_to_keep = [c for c in string.ascii_letters + string.digits + "_.-"]


def nothing(k, v, f, m):
  return None

def elem(k, v):
  return {'t' : k, 'c': v}

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


def page_list(filename):
  tree = ET.parse(filename)
  root = tree.getroot()
  return [link.attrib['title'].encode('utf-8') for link in root.findall(".//ul[@class='mw-allpages-chunk']/li/a")]


redirects = {}

def parse_redirects(input):
  global redirects
  redirects = json.loads(input)
  for r in redirects:
      target = redirects[r].encode('utf-8')
  for r in redirects:
      target = redirects[r].encode('utf-8')
      ccc=0
      while target in redirects:
          target = redirects[target]
          if ccc > 999:
            logging.warn('redirect loop for ' + r +'?')
          ccc += 1
      redirects[r] = target

def find_redirect(target):
  if target in redirects:
    return redirects[target]
  return target
