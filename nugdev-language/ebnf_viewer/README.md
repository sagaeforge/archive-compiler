# EBNF Viewer

A graphical viewer for EBNF (Extended Backus-Naur Form) grammar files. This tool parses EBNF files and displays the grammar rules in a visual tree structure.

## Features

- Parse EBNF grammar files
- Visualize grammar rules as a tree structure
- Interactive navigation (zoom, pan)
- Support for all EBNF constructs (sequences, alternatives, repetitions, etc.)

## Requirements

- C++17 compatible compiler
- CMake 3.15 or higher
- SFML 2.5 or higher

## Building

1. Create a build directory:

```bash
mkdir build
cd build
```

2. Configure with CMake:

```bash
cmake ..
```

3. Build the project:

```bash
cmake --build .
```

## Usage

Run the viewer with an EBNF file as an argument:

```bash
./ebnf_viewer path/to/grammar.ebnf
```

### Controls

- Left mouse button: Pan the view
- Mouse wheel: Zoom in/out

## Example EBNF File

```
program = statement* ;
statement = declaration | expression ;
declaration = "let" identifier "=" expression ;
expression = number | identifier ;
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.
