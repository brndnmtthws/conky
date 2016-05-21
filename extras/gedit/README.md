# Gedit syntax highlighting

Note this is purely a highlighting **NOT** a syntax validator, the highlighting may not be 100% correct and not all arguments or keywords may be recognised properly. Feel free to change a regex if you have got a better one.

The syntax highlighting will automatically be applied to all files with `conky` in their name. eg. `my_config.conky` (unfortunately it also triggers for the conky.lang file itself, you should set it to XML manually)

 *  [Gedit Syntax Highlight documentation](https://developer.gnome.org/gtksourceview/stable/lang-reference.html)
 * [Regex Tutorial](http://www.rexegg.com/)
 * [Regex Testing](https://regex101.com/)

Developers: if you want to add a new keyword just add it (in order) to the list. In `keywordsConfig` for config keywords and in `keywordsText` for variables. If it has a number at the end, eg `color1` just use `color[0-9]{1}` if the number is mendatory or `color[0-9]?` if it is not.

If you want to add your own group of argument keywords you can look at the current definitions and copy them, should be self explainatory.

***

 for medit v1.1.1:
` /usr/share/medit/language-specs/conky.lang`

 for medit - older versions:
 `/usr/share/medit-1/language-specs/conky.lang`

 for gedit v2.x
` /usr/share/gtksourceview-2.0/language-specs/conky.lang`
 or (for single user)
` ~/.local/share/gtksourceview-2.0/language-specs/conky.lang`

 for gedit v3.x
` /usr/share/gtksourceview-3.0/language-specs/conky.lang`
 or (for single user)
` ~/.local/share/gtksourceview-3.0/language-specs/conky.lang`
