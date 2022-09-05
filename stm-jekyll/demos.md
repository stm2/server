---
title: Eressea Demo-Reporte
---
<h1>{{ $title }}</h1>
{% for demo in site.demos %}

## [{{ demo.name }}{% if demo.description %} - {{ demo.description }}{% endif %}]({{ site.baseurl }}{{ demo.url }})

  {% if demo.crs.size == 1 %}
CR: [{{ demo.crs[0] }}](demos/{{ demo.crs[0] }})
  {% endif %}
  {% if demo.crs.size > 1 %}
CRs:
    {% for cr in demo.crs limit:1 -%}
[{{ cr }}](demos/{{ cr }})
    {% endfor -%}
    {% for cr in demo.crs offset:continue -%}
, [{{ cr }}](demos/{{ cr }})
    {% endfor %}
  {% endif %}
{{ demo.content }}
{% endfor %}
