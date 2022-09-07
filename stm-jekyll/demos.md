---
title: Eressea Demo-Reporte
---
{% for demo in site.demos %}
## [{{ demo.name }}{% if demo.description %} - {{ demo.description }}{% endif %}]({{ site.baseurl }}{{ demo.url }})

  {% if demo.crs.size == 1 %}
CR:
  {% endif -%}
  {% if demo.crs.size > 1 -%}
CRs:
  {% endif -%}
  {% for cr in demo.crs limit:1 -%}
[{{ cr }}](demos/{{ cr | replace: ".cr", ".html" }})
<span style="font-size:small;"><a href="demos/{{ cr }}">(download)</a></span>
  {% endfor -%}
  {% for cr in demo.crs offset:continue -%}
, [{{ cr }}](demos/{{ cr | replace: ".cr", ".html" }})
<span style="font-size:small;"><a href="demos/{{ cr }}">(download)</a></span>
  {% endfor %}

{{ demo.content | split: "." | first | markdownify }}...<br>

{% endfor %}
