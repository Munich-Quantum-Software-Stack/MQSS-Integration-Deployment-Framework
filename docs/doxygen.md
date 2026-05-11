# API Documentation

The API documentation is generated from the public headers in `include/`
using Doxygen.

Generate it from the repository root:

```sh
doxygen Doxyfile
```

The generated HTML output is written to:

```text
docs/api/
```

Open it locally:

```sh
xdg-open docs/api/index.html
```

The generated files are not committed to Git.
