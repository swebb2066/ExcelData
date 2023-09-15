from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class Excel2csvConan(ConanFile):
    name = "Excel2csv"
    version = "0.0"
    author = "Stephen Webb swebb2066@gmail.com"
    url = "https://github.com/swebb2066/excel2csv"
    description = "Generate a comma separated values (CSV) file from .xls or .xlsx files"
    topics = ("Excel", "CSV")
    settings = "os", "compiler", "build_type", "arch"
    requires = (
        "log4cxx/1.1.0@",
        "boost/1.83.0@",
        "yaml-cpp/0.8.0@",
        )
    generators = ("CMakeDeps", "CMakeToolchain")

    def configure(self):
        self.options["log4cxx"].shared = True
        self.options["boost"].shared = False
        self.options["boost"].zlib = False
        self.options["boost"].bzip2 = False
        self.options["yaml-cpp"].shared = False

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
