# Contribute to OneBusAway for Pebble

This guide details how to use issues and pull requests (for new code) to improve OneBusAway for Pebble.

## Individual Contributor License Agreement (ICLA)

To ensure that the app source code remains fully open-source under a common license, we require that contributors sign an electronic ICLA before contributions can be merged.  When you submit a pull request, you'll be prompted by the [CLA Assistant](https://cla-assistant.io/) to sign the ICLA.

## Code of Conduct

This project adheres to the [Open Code of Conduct](http://todogroup.org/opencodeofconduct/#OneBusAway/conduct@onebusaway.org). By participating, you are expected to honor this code.

## Code Style and Template

See the main [OneBusAway Code Style Guide](https://github.com/OneBusAway/onebusaway/wiki/Code-Style).

This project follows the [Google C++ coding style guide](https://google.github.io/styleguide/cppguide.html), [Summarized by davidzchen here](https://gist.github.com/davidzchen/9187878).

## Closing policy for issues and pull requests

OneBusAway is a popular project and the capacity to deal with issues and pull requests is limited. Out of respect for our volunteers, issues and pull requests not in line with the guidelines listed in this document may be closed without notice.

Please treat our volunteers with courtesy and respect, it will go a long way towards getting your issue resolved.

Issues and pull requests should be in English and contain appropriate language for audiences of all ages.

## Issue tracker

The [issue tracker](https://github.com/OneBusAway/onebusaway-pebbletime/issues) is only for obvious bugs, misbehavior, & feature requests in the latest stable or development release of OneBusAway for Pebble. When submitting an issue please conform to the issue submission guidelines listed below. Not all issues will be addressed and your issue is more likely to be addressed if you submit a pull request which partially or fully addresses the issue.

### Issue tracker guidelines

**[Search](https://github.com/OneBusAway/onebusaway-pebbletime/search?q=&ref=cmdform&type=Issues)** for similar entries before submitting your own, there's a good chance somebody else had the same issue or feature request. Show your support with `:+1:` and/or join the discussion. Please submit issues in the following format (as the first post) and feature requests in a similar format:

1. **Summary:** Summarize your issue in one sentence (what goes wrong, what did you expect to happen)
1. **Steps to reproduce:** How can we reproduce the issue?
1. **Expected behavior:** What did you expect the app to do?
1. **Observed behavior:** What did you see instead?  Describe your issue in detail here.
1. **Possible fixes**: If you can, link to the line of code that might be responsible for the problem.

## Pull requests

We welcome pull requests with fixes and improvements to OneBusAway for Pebble code, tests, and/or documentation. The features we would really like a pull request for are [open issues with the enhancements label](https://github.com/OneBusAway/onebusaway-pebbletime/issues?labels=enhancement&page=1&state=open).

### Pull request guidelines

If you can, please submit a pull request with the fix or improvements including tests. If you don't know how to fix the issue but can write a test that exposes the issue we will accept that as well. In general bug fixes that include a regression test are merged quickly while new features without proper tests are least likely to receive timely feedback. The workflow to make a pull request is as follows:

1. Sign the [ICLA](https://docs.google.com/forms/d/12jV-ByyN186MuPotMvxJtNKtSaGGTnEHm8rXomM2bm4/viewform) (first time only)
1. Fork the project on GitHub
1. Create a feature branch
1. If you created new files, add the Apache license header to them. If you modified an existing file, add your name to the copyright attribution in the header.
1. Push the commit to your fork
1. Submit a pull request with a motive for your change and the method you used to achieve it
1. [Search for issues](https://github.com/OneBusAway/onebusaway-pebbletime/search?q=&ref=cmdform&type=Issues) related to your pull request and mention them in the pull request description or comments

We will accept pull requests if:

* The code has proper tests and all tests pass (or it is a test exposing a failure in existing code)
* It can be merged without problems (if not please use: `git rebase master`)
* It doesn't break any existing functionality
* It's quality code that conforms to standard style guides and best practices
* The description includes a motive for your change and the method you used to achieve it
* It is not a catch all pull request but rather fixes a specific issue or implements a specific feature
* It keeps the OneBusAway for Pebble code base clean and well structured
* We think other users will benefit from the same functionality
* If it makes changes to the UI the pull request should include screenshots
* It is a single commit (please use `git rebase -i` to squash commits)

## License

By contributing code to this project via pull requests, patches, or any other process, you are agreeing to license your contributions under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0.html).
