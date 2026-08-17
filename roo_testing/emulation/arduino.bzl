"""Public build helpers for Arduino sketches running under roo_testing."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

_ARDUINO_MAIN = Label("//:arduino_main")
_IS_ARDUINO = Label("//roo_testing/platforms:is_arduino")
_INCOMPATIBLE = Label("@platforms//:incompatible")

def _sketch_wrapper_impl(ctx):
    output = ctx.actions.declare_file(ctx.label.name + ".cpp")
    ctx.actions.write(
        output = output,
        content = '#include "{}"\n'.format(ctx.file.sketch.basename),
    )
    return [DefaultInfo(files = depset([output]))]

_sketch_wrapper = rule(
    implementation = _sketch_wrapper_impl,
    attrs = {
        "sketch": attr.label(
            mandatory = True,
            allow_single_file = [".ino"],
        ),
    },
)

def _normalize_sketch(sketch):
    if type(sketch) != "string":
        fail("sketch must be a package-local .ino label string")
    path = sketch[1:] if sketch.startswith(":") else sketch
    if path.startswith("/") or path.startswith("@") or ":" in path:
        fail("sketch must be package-local, got: {}".format(sketch))
    if not path.endswith(".ino"):
        fail("sketch must name a .ino file, got: {}".format(sketch))
    return path

def _derive_include_dir(sketch_path):
    parts = sketch_path.split("/")
    return "." if len(parts) == 1 else "/".join(parts[:-1])

def _arduino_compatibility():
    return select({
        _IS_ARDUINO: [],
        "//conditions:default": [_INCOMPATIBLE],
    })

def roo_arduino_example(
        name,
        sketch,
        deps = [],
        srcs = [],
        include_dir = None,
        linkstatic = True,
        target_compatible_with = [],
        **kwargs):
    """Creates a native runnable cc_binary from a C++-valid Arduino sketch.

    The macro deliberately does not reproduce Arduino's automatic prototype
    generation. The `.ino` file must be valid when included as C++ and should
    include `Arduino.h` itself when it uses Arduino APIs.

    Args:
      name: Name of the resulting runnable `cc_binary` target.
      sketch: Package-local string label of one `.ino` source file.
      deps: Libraries needed while compiling and linking the sketch.
      srcs: Additional C/C++ sources compiled into the binary.
      include_dir: Optional package-relative directory containing the sketch.
        It is derived from `sketch` by default.
      linkstatic: Forwarded to `cc_binary`; true by default for host emulation.
      target_compatible_with: Additional compatibility constraints. The macro
        always makes the target incompatible outside an Arduino frontend.
      **kwargs: Remaining `cc_binary` attributes, such as `defines`, `copts`,
        `data`, `args`, `tags`, and `visibility`.
    """
    sketch_path = _normalize_sketch(sketch)
    if include_dir == None:
        include_dir = _derive_include_dir(sketch_path)

    wrapper_target = name + "__ino_wrapper"
    context_target = name + "__ino_context"
    arduino_compatibility = _arduino_compatibility()

    _sketch_wrapper(
        name = wrapper_target,
        sketch = sketch,
        target_compatible_with = arduino_compatibility,
    )

    cc_library(
        name = context_target,
        textual_hdrs = [sketch],
        includes = [include_dir],
        target_compatible_with = arduino_compatibility,
        deps = deps,
    )

    cc_binary(
        name = name,
        srcs = [":" + wrapper_target] + srcs,
        deps = [
            ":" + context_target,
            _ARDUINO_MAIN,
        ],
        linkstatic = linkstatic,
        target_compatible_with = target_compatible_with + arduino_compatibility,
        **kwargs
    )
