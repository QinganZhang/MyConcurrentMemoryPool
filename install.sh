mkdir build
mkdir install
cd build 
rm -rf * 
cmake .. // -DCMAKE_INSTALL_PREFIX=
make -j8 
make install 

# CPP头文件搜索路径
# export CPLUS_INCLUDE_PATH= 
# 动态链接库搜索路径
# export LD_LIBRARI_PATH=
# 静态链接库搜索路径
# export LIBRARY_PATH=