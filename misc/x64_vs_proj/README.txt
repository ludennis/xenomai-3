files generated with the following commands:
  1. mkdir x64_vs_proj
  2. cd x64_vs_proj
  3. cmake ../ -G "Visual Studio 15 2017 Win64" -T "host=x64"

- The directory specified by cmake is where "CMakeLists.txt" exist
- The option "-G" is for cmake generator to generate particular target from
  a "CMakeLists.txt"
- The option "-T" specifies the target platform, so does the "Win64"
