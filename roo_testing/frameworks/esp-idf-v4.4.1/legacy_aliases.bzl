"""Compatibility helpers for labels from the former ESP-IDF 4.4.1 tree."""


def legacy_idf_aliases(names):
    """Redirects former component targets to the current host ESP-IDF facade."""
    for target_name in names:
        native.alias(
            name = target_name,
            actual = "//roo_testing/frameworks/esp-idf:core",
            visibility = ["//visibility:public"],
        )
