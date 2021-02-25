#new_git_repository(
#    name = "googletest",
#    build_file = "gmock.BUILD",
#    remote = "https://github.com/google/googletest",
#    tag = "release-1.8.0",
#)

# local_repository(
#     name = "arduino",
#     path = "../arduino",
# )

new_local_repository(
   name = "fltk",
   path = "/home/dawidk/fltk-1.3.4-2",
   build_file = "fltk.BUILD",
)

#new_local_repository(
#    name = "main",
#    path = "../../",
#    build_file = "main.BUILD",
#)

new_local_repository(
    name = "system_libs",
    # pkg-config --variable=libdir x11
    path = "/usr/lib/x86_64-linux-gnu",
    build_file_content = """
cc_library(
    name = "x11",
    srcs = ["libX11.so", "libXext.so", "libdl.so", "libm.so", "libpthread.so"],
    linkstatic = 1,
    alwayslink = 1,
    visibility = ["//visibility:public"],
)
""",
)
