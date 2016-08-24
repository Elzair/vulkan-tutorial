#.rst:
# FindGLSLangValidator
# -------
#
# The module defines the following variables:
#
# ``GLSLANGVALIDATOR_EXECUTABLE``
#   Path to glslangValidator
# ``GLSLangValidator_FOUND``, ``GIT_FOUND``
#   True if glslangValidator was found.
# ``GIT_VERSION_STRING``
#   The version of glslangValidator found.
#
# Example usage:
#
# .. code-block:: cmake
#
#    find_package(GLSLangValidator)
#    if(GLSLangValidator_FOUND)
#      message("GLSLangValidator found: ${GIT_EXECUTABLE}")
#    endif()

#==============================================================================
# Copyright (c) 2016, Philip Woods <elzairthesorcerer@gmail.com>
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#==============================================================================

find_program(
  GLSLANGVALIDATOR_EXECUTABLE
  NAMES "glslangValidator"
  DOC "glslang Validator"
)
mark_as_advanced(GLSLANGVALIDATOR_EXECUTABLE)

if(GLSLANGVALIDATOR_EXECUTABLE)
  execute_process(COMMAND ${GLSLANGVALIDATOR_EXECUTABLE} -v
                  OUTPUT_VARIABLE glslangValidator_version
                  ERROR_QUIET
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (glslangValidator_version MATCHES "^Glslang Version: SPIRV")
    string(REPLACE "Glslang Version: SPIRV" "" glslangValidator_version2 "${glslangValidator_version}")
    string(REPLACE " *" "" GLSLANGVALIDATOR_VERSION_STRING "${glslangValidator_version2}")
  endif()
  unset(glslangValidator_version)
  unset(glslangValidator_version2)
endif()

# Handle the QUIETLY and REQUIRED arguments and set GLSLangValidator_FOUND to TRUE if
# all listed variables are TRUE

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLSLangValidator
                                  REQUIRED_VARS GLSLANGVALIDATOR_EXECUTABLE
                                  VERSION_VAR GLSLANGVALIDATOR_VERSION_STRING)
