# DBUIL
DBUIL is a small build automation program. It's like Make, but uses json instead of Makefile.

# Example

This is an example of how `dbuil.json` file look like:
```json
{
  "variables" : {
    "message": "Hello World!"
  },
  "targets": [
    {
      "name": "test",
      "description": "Sample Text",
      "commands": [ "echo ${message}" ]
    }
  ]
}
```

See other examples in `dbuil.json` and `test` folder.

# How to use
Just type `./dbuil -help` to see all available commands.

