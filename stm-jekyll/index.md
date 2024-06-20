---
# Feel free to add content and custom Front Matter to this file.
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

#layout:  special
---

Willkommen auf Solthars Eresseaseiten!

## Pages

<ul>
    {% for link in site.data.navigation.docs %}
    <li><a href="{{ site.baseurl }}{{ link.url }}">{{ link.title }}</a></li>
    {% endfor %}
</ul>

## Posts
{% for post in site.posts %}
  - [{{ post.title }}]({{ site.baseurl }}{{post.url}})
{% endfor %}
