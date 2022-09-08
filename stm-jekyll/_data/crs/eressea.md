---
name: Eressea Example
description: An extensive example
crs:
  - 999-demo.cr
  - 999-nice.cr
  - 999-foe.cr
  - 1000-demo.cr
  - 1000-nice.cr
  - 1000-foe.cr
date: 2022-09-08
layout: crsvg
custom-javascript-list:
  - crstuff.js
---
An example with all sorts of stuff that could go on in an Eressea game{% for cr in page.crs %}
  - [{{ cr }}]({{ cr | replace: ".cr", ".html" }})
{% endfor %}
