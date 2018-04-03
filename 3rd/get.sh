wget -cN --directory-prefix=cpp https://github.com/catchorg/Catch2/releases/download/v2.2.0/catch.hpp
git clone https://github.com/ericniebler/range-v3.git cpp/range-v3
git clone https://github.com/ryanhaining/cppitertools cpp/itertools
wget -cN --directory-prefix=cpp https://github.com/fmtlib/fmt/releases/download/4.1.0/fmt-4.1.0.zip
cd cpp
unzip fmt-4.1.0.zip
