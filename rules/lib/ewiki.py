#!/usr/bin/env python

import string, json, re, logging
import urllib as ul
import htmlentitydefs as entity
from xml.sax.saxutils import escape
import xml.etree.ElementTree as ET
from datetime import datetime

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


'''keep html tags in tags_json['keep'], remove tags in tags_json['remove'], html-escape all other tags'''
def remove_tags(filename, tags_json):
  global kept_tags, removed_tags
  kept_tags = json.loads(tags_json)["keep"]
  removed_tags = json.loads(tags_json)["remove"]
  with open(filename,'r') as mw_file:
    for line in mw_file:
      print tag_pattern.sub(tag_replace, line),

'''
template_filename: mediawiki import xml template
user: editing user name
user_id: numeric id, default None
comment: edit comment
pages: json-string of { "title":"filename" } entries of files to export
timestamp: like 2015-04-08T07:56:59Z, default None
minor: true for minor edit, default False
ns: namespace, default 0
page_id: default None
'''
def mediawiki_export(template_filename, user, comment, pages, timestamp=None, minor=False, ns=0, user_id=None):
  
  if timestamp == None:
    timestamp = datetime.utcnow().isoformat()

  page = """
  <page>
    <title>$title$</title>
    <ns>$ns$</ns>
<!--    <id>123</id> -->
    <revision>
<!--      <id>$id</id>-->
<!--      <parentid>5432</parentid>-->
      <timestamp>$timestamp$</timestamp>
      <contributor>
        <username>$user$</username>
        <id>$userid$</id>
      </contributor>
      $minor$
      <comment>$comment$</comment>
      <text xml:space="preserve" >$text$</text>
<!--      <sha1>ievbe98ixa3b11e89hxw1mgsno1odfd</sha1>-->
      <model>wikitext</model>
      <format>text/x-wiki</format>
    </revision>
  </page>
  """

  xml_content = ""
  for title, text_file in json.loads(pages).iteritems():
    with open(text_file, 'r') as mwfile:
      content = mwfile.read()
      for line in page.split('\n'):
        line = re.sub('\$title\$', title, line)
        line = re.sub('\$ns\$', str(ns), line)
        line = re.sub('\$timestamp\$', timestamp, line)
        line = re.sub('\$user\$', user, line)

        if user_id == None:
          line = re.sub('<id>\$userid\$</id>', '<!--<id>\$userid\$</id>-->', line)
        else:
          line = re.sub('\$userid\$', user_id, line)

        if minor:
          line = re.sub('\$minor\$', "<minor />", line)
        else:
          line = re.sub('\$minor\$', "", line)

        line = re.sub('\$comment\$', comment, line)

        line = re.sub('\$text\$', escape(content.decode('utf-8')), line)

        xml_content  += line + "\n"

      xml_content += "\n"

  with open(template_filename, 'r') as tfile:
    for line in tfile:
      print re.sub('\$pages\$', xml_content.encode('utf-8'), line),
