#!/usr/bin/env python
# coding: utf-8

import sys, json, ewiki


if len(sys.argv) > 1:
  with open(sys.argv[1], 'r') as file:
    files = json.load(file)
    result = []
    for [filename, title] in files:
      filename = filename.strip()
      title = title.strip()
      with open(filename, 'r') as f:
        content = json.load(f)
        content[1].insert(0, ewiki.json_header(title))
        if result == []:
          result = content
        else:
          result[1] += content[1]
          
    print json.dumps(result)
