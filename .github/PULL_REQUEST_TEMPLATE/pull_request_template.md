**Descriptions**
* Describe the changes, why they were necessary, etc
* Describe how the changes will affect existing behaviour.
* Describe how you tested and validated your changes.
* Include any relevant screenshots/evidence demonstrating that the changes work and have been tested.

**Licenses**
* Any new source files should include a GPLv3 license header.
* Any contributed code must be GPLv3 licensed.

**Notes**
* Make sure your code is clean and if you have added a new features, some
  comments may be appreciated.  Make sure your commit messages are
  clear/information enough.
* If you added or altered settings and/or variables, make sure to
  update the documentation. Do not forget to mention default values or required
  build flags if this applies to your change.
* If you added a build flag, make sure to update the `static void print_version(void)` function with your new flag. If you added a new console command, make sure to update the `static void print_help(const char *prog_name)` function with your new command.
* Add tests for the code, where appropriate.

Please delete all descriptions, headers, notes, sections, etc too. Thank you.
