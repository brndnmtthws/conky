# Conky docs

There are 3 YAML files which contain the documentation:

* [`variables.yaml`](variables.yaml): Documents each object/variable
* [`config_settings.yaml`](config_settings.yaml): Documents global configuration settings
* [`lua.yaml`](lua.yaml): Documents Conky's Lua API

The `desc` field within the docs can be formatted with markdown, however _do
not_ include headings within the `desc` fields, as it will mess up the man page
output. In markdown, headings begin with `#`.

## Updating docs

The man page is based on [`man.md.j2`](man.md.j2) which is a Jinja2 template.
The generated markdown is used to generate a final man page using
[`pandoc`](https://pandoc.org/). Generating the final man page is a 2 step
process:

1. Run `docgen.py` to process `man.md.j2`:
   ```console
   $ ./docgen.py man.md.j2 > man.md
   ```
2. Run `pandoc` to convert the markdown into a man page:
   ```console
   $ pandoc -f markdown -t man.md > conky.1
   ```

These steps are also part of the CMake build, and can be executed by configuring
the build with `-DBUILD_DOCS=ON`.
