# Gedit syntax highlighting

Note: this highlights based on syntax and does **NOT** attempt to validate arguments or keywords. The syntax highlighting is unlikely to be 100% accurate and is open to improvement.

The syntax highlighting will automatically be applied to all files with `conky` in their name. eg. `my_config.conky` (unfortunately it also triggers for the `conky.lang` file itself, you should set it to XML manually)

 * [`gtksourceview` Syntax Highlight documentation][1]
 * [Regex Tutorial](http://www.rexegg.com/)
 * [Regex Testing](https://regex101.com/)

Developers: The main context (`id="conkyrc"`) is where gedit begins. This main context then references other sub-contexts. Each context can apply styles to itself, sub-strings from its regexs, or its contents (in the case of `<start><end>` "container" contexts). If you are ever confused by something, try searching for XML attributes in the [`gtksourceview` docs][1]. If you find a particularly complex regex, try using the Regex Tester linked above, and bear in mind that `gtksourceview` adds some extra regex syntax (i.e. `\%[ ... ]` and `\%{ ... }`).

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

[1]: https://developer.gnome.org/gtksourceview/stable/lang-reference.html
