# Contributing

Contributions are welcome from anyone.

To report bugs or issues, open [new issues](https://github.com/brndnmtthws/conky/issues/new).
To submit code changes, open [new pull requests](https://github.com/brndnmtthws/conky/compare).

## Guidelines

Patches submitted in issues, email, or elsewhere may be ignored. When submitting PRs, please:

- Describe the changes, why they were necessary, etc
- Describe how the changes affect existing behaviour
- Describe how you tested and validated your changes
- Include unit tests when appropriate
- Include any relevant screenshots/evidence demonstrating that the changes work and have been tested
- Any new source files should include a GPLv3 license header
- Any contributed code must be GPLv3 licensed
- Always leave the code better than you found it
- PRs with failed checks may be ignored or closed; please make sure
  the build and checks pass if possible (and notify someone when the build
  system is not working)

## Coding Style

Code should be formatted using `clang-format`. By configuring Conky with `cmake -DCHECK_CODE_QUALITY=ON`, you will be able to run `make clang-format` to automatically format code.

If code in your PR is not formatted according to [`.clang-format`](.clang-format), the checks will not pass.

## Unit Testing

Unit tests are a relatively new addition to Conky, and most existing code does not have tests. Conky uses the [Catch2](https://github.com/catchorg/Catch2) unit testing framework.

If you are adding new functions or methods, please consider adding unit tests for that code. Additionally, if you'd like to add tests for existing code, that would be a welcome contribution.
