# Contributing

Contributions to aMule are welcome. This page describes how to get involved.

## Ways to Contribute

- **Code**: Submit pull requests on [GitHub](https://github.com/amule-project/amule).
- **Bug reports**: Open an issue with the aMule version, operating system, and steps to reproduce.
- **Translations**: Update or add `.po` files in the `po/` directory. See [the translation guide](https://github.com/amule-project/amule/blob/master/docs/translations.md).
- **Documentation**: Improve or expand these docs.
- **Testing**: Test release candidates and report regressions.

## Pull Requests

1. Fork the repository and create a feature branch.
2. Follow the existing code style (C++17, wxWidgets conventions).
3. Keep changes focused; one logical change per PR.
4. Ensure the project builds without warnings on at least one supported platform.
5. Describe what the change does and why in the PR description.

## Reporting Bugs

When opening a bug report, please include:

- aMule version (`amule --version`)
- Operating system and version
- Steps to reproduce the issue
- Any relevant log output or crash traces

## Translations

aMule uses **GNU gettext** for internationalization.

```sh
# Regenerate the .pot template and merge into all .po files
./scripts/update-po.sh
```

To add a new language:

```sh
msginit -l xx_YY -i po/amule.pot -o po/xx_YY.po
# Add xx_YY to po/LINGUAS
```

Then translate the strings in `po/xx_YY.po` and open a pull request.
