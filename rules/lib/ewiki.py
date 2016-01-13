#!/usr/bin/env python

import string, json, re, logging
import urllib as ul
import htmlentitydefs as entity
import xml.etree.ElementTree as ET

chars_to_keep = [c for c in string.ascii_letters + string.digits + "_.-"]


def nothing(k, v, f, m):
  return None

def elem(k, v):
  return {'t' : k, 'c': v}

nullAttr = ("",[],[])

def url_encode(name):
  if type(name) == type(u''):
    return ul.quote(name.encode('utf-8'))
  else:
    return ul.quote(name)

def link_id(raw, prefix):
  try:
    if raw == None:
      return html_encode(prefix.lower())
    else:
      return (html_encode(prefix.lower())+"-"+html_encode(raw.lower()))
  except Exception as exc:
    raise Exception(raw, type(raw), prefix, type(prefix), exc)

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
      target = redirects[r]
      ccc=0
      while target in redirects:
          target = redirects[target]
          if ccc > 999:
            logging.warn('redirect loop for ' + r +'?')
            break;
          ccc += 1
      redirects[r] = target

def find_redirect(target):
  if target in redirects:
    return redirects[target]
  return target

def json_header(title, level=1):
  id = link_id("", title)
  header = { "t" : "Header", "c" : [level, [ id, [], []], [ { "t" : "Str", "c" : title } ] ] }
  return header

tag_pattern = re.compile('<(/{0,1}([A-Za-z][^ >]*)[^>]*)>')

kept_tags={}
removed_tags={}

def tag_replace(matchobj):
  global kept_tags, removed_tags
  if matchobj.group(2) in kept_tags:
    return matchobj.group(0)
  elif matchobj.group(2) in removed_tags:
    return ""
  
  return "&lt;"+matchobj.group(1)+"&gt;"


def remove_tags(filename, tags_json):
  global kept_tags, removed_tags
  kept_tags = json.loads(tags_json)["keep"]
  removed_tags = json.loads(tags_json)["remove"]
  with open(filename,'r') as mw_file:
    for line in mw_file:
      print tag_pattern.sub(tag_replace, line),
