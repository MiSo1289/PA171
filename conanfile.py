from conans import ConanFile


class PA171(ConanFile):
    name = "PA171"
    version = "0.0.0"
    revision_mode = "scm"
    settings = ("os", "compiler", "arch", "build_type")
    exports_sources = (
        "src/*",
        "CMakeLists.txt",
    )
    requires = (
        "abseil/20210324.2",
        "catch2/2.13.7",
        "fmt/8.0.1",
        "lyra/1.5.1",
        "range-v3/0.11.0",
        "stb/cci.20210713",
    )
    default_options = {
        "boost:header_only": True,
    }
    generators = ("CMakeDeps", "CMakeToolchain")
