## DText Parser

A Ruby extension for parsing DText markup used by e621.net.
This gem uses the [Ragel state machine compiler](https://github.com/adrian-thurston/ragel) to generate efficient C++ parsing code for DText syntax.

### Setting up the development environment

If you wish to test your changes in a local instance of [e621ng](https://github.com/e621ng/e621ng), please refer to the "Local DText gem" section of that repo's README file.

1. Build the Docker container: `docker compose build`
2. Compile the parser: `docker compose run --rm rake compile`
3. Run tests: `docker compose run --rm rake test`
4. Build the source and pre-compiled gems: `docker compose run --rm rake native gem`

### Important: Do not commit `dtext.cpp` in pull requests

The `dtext.cpp` file is a generated artifact and should not be committed in development branches or pull requests.
It will be regenerated during the official release process.

### Creating a new release

Use the release script to properly bump the version number and compile the artifacts.

```bash
./scripts/release.sh [patch|minor|major]
```

Don't forget to update e621ng's Gemfile to include the new tag name.

### Ragel

If you're new to Ragel, these resources will help you understand the syntax and concepts:
- [Unofficial Ragel quickstart guide](https://github.com/calio/ragel-cheat-sheet)
- [Official Ragel documentation](http://www.colm.net/files/ragel/ragel-guide-6.10.pdf)
