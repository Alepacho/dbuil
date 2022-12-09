# BUILD
BUILD is a small build automation program with the ability to compile only changed source files.

# Example

This is an example of how `build.json` file look like:
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

See other examples in `build.json` and `test` folder.

# How to use
Just type `./build -help` to see all available commands.

