if [ ! -d "lib" ]; then 
	mkdir lib
fi
cd lib
rm -rf * 
cd .. 

if [ ! -d "install" ]; then 
	mkdir install
fi
cd install
rm -rf *
cd ..

if [ ! -d "build" ]; then 
	mkdir build
fi
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
