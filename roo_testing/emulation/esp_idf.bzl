"""Public build helpers for ESP-IDF applications running under roo_testing."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

_ESP_IDF_MAIN = Label("//:esp_idf_main")
_IDF = Label("//roo_testing/platforms:idf")

def roo_esp_idf_example(
        name,
        srcs,
        deps = [],
        linkstatic = True,
        target_compatible_with = [],
        **kwargs):
    """Creates a host-runnable binary from an ESP-IDF `app_main` application.

    Args:
      name: Name of the resulting runnable `cc_binary` target.
      srcs: C/C++ sources, one of which defines `extern "C" void app_main()`.
      deps: Libraries needed while compiling and linking the application.
      linkstatic: Forwarded to `cc_binary`; true by default for host emulation.
      target_compatible_with: Additional compatibility constraints. The macro
        always makes the target incompatible outside the ESP-IDF frontend.
      **kwargs: Remaining `cc_binary` attributes, such as `defines`, `copts`,
        `data`, `args`, `tags`, and `visibility`.
    """
    cc_binary(
        name = name,
        srcs = srcs,
        deps = deps + [_ESP_IDF_MAIN],
        linkstatic = linkstatic,
        target_compatible_with = target_compatible_with + [_IDF],
        **kwargs
    )
