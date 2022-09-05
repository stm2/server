---
title: Eressea Demo-Reporte
---
{% for demo in site.demos %}
  <h2>
    <a href="{{ demo.url }}">
      {{ demo.name }} - {{ demo.description }}
    </a>
  </h2>
  <p>CR: <a href="{{ demo.cr }}">{{ demo.cr }}</a></p>
  <p>{{ demo.content | markdownify }}</p>
{% endfor %}
