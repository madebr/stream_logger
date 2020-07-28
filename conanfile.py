#  Copyright (C) 2020 Maarten DB
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

from conans import CMake, ConanFile
import os


class StreamLoggerConan(ConanFile):
    name = "stream_logger"
    version = "master"
    settings = "os", "arch", "compiler", "build_type"
    exports_sources = "CMakeLists.txt", "*.cpp", "COPYING"
    exports = "COPYING"
    generators = "cmake"
    requires = (
        "boost/1.73.0",
    )

    _cmake = None

    def _get_cmake(self):
        if self._cmake:
            return self._cmake
        self._cmake = CMake(self)
        self._cmake.definitions["Boost_USE_DEBUG_LIBS"] = self.settings.build_type == "Debug"
        self._cmake.definitions["Boost_USE_STATIC_LIBS"] = not self.options["boost"].shared
        if self.settings.compiler == "Visual Studio":
            self._cmake.definitions["Boost_USE_MULTITHREADED"] = "MT" in str(self.settings.compiler.runtime)
        return self._cmake

    def build(self):
        cmake = self._get_cmake()
        cmake.configure()
        cmake.build()

    def install(self):
        cmake = self._get_cmake()
        cmake.install()

    def package_info(self):
        bin_path = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH environment variable: {}".format(bin_path))
        self.env_info.PATH.append(bin_path)