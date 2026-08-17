# SPDX-License-Identifier: GPL-2.0-or-later
function(openxhc_enable_warnings target)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion)
  elseif(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
  endif()
endfunction()
