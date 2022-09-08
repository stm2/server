---
name: Demo 001
description: A more complex CR
crs:
  - 334-42.cr
date: 2022-09-08
layout: crsvg
custom-javascript-list:
  - crstuff.js
---
An example with different types of terrains, ships, lighthouses, and a battle.
{% for cr in page.crs %}
  - [{{ cr }}]({{ cr | replace: ".cr", ".html" }})
{% endfor %}
