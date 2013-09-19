## Introduction
The Tizen Extensions Crosswalk (t-e-c) implements the
Tizen Web APIs (https://developer.tizen.org/documentation/web-api-reference)
on top of the Crosswalk app runtime (https://crosswalk-project.org/) and
its Extensions framework.

This is an open source project started by the Intel Open Source Technology Center
(http://www.01.org).

## Documents
Check out our [Wiki](https://github.com/crosswalk-project/tizen-extensions-crosswalk/wiki).

## Coding Style
JavaScript: We use Google's Closure Linter for checking our JavaScript coding sytle.
It is used in --strict mode but with max_line_length set to 100 columns.
Please ALWAYS run tools/js_lint.sh before submitting patches.

C++: We use cpplint. Please ALWAYS run tools/check-coding-style before submitting patches.

## License
This project's code uses the BSD license, see our `LICENSE` file.
