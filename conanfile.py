from conans import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


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
        "boost/1.77.0",
        "fmt/8.0.1",
        "lyra/1.5.1",
        "stb/cci.20210713",
    )
    default_options = {
        "boost:header_only": True,
    }

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.generate()
