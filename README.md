# SCANIVALVE MPS PROTOCOL

This single-header library exposes the internal data structures used by Scanivalve MPS devices. This can be included in projects that aim to interface with these scanners for data acquisition or device control and communication. The library is written in C to allow for use in C, C++, Fortran (using ISO_C_BINDINGS) or C# (using P/Invoke functions as needed).


## Including the interface in a project

The interface is a single-header file which can simply be copied to a directory in your projects include path.


### Including as git submodule

To keep up to date with development on the MPS protocol, include the directory as a git submodule. 

```bash
# add as git submodule
git submodule add https://github.com/csooriyakumaran/scanivalve-mps-protocol external/scanivalve-mps-protocol

# [Optiona] checkout specific tag (matching scanivalve firmware)
cd external/scanivalve-mps-protocol
git fetch --tags

git checkout <tag_name>
#e.g. for firmware version v4.01
git checkout v4.01

# stage and commit the changes in the parent repository
cd <project-root>
git add external/scanivalve-mps-protocol
git commit -m "Add submodule at specific tag <tag_name>"

```

### CMake

```cmake 
    add_subdirectory(external/scanivalve-mps-protocol)
    target_link_libraries(your_app PRIVATE scanivalve-mps-protocol)
```
