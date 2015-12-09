# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"public" -I"public/core" -I"public/platform" -I"src_dependency/sdl2/include" 
Release_Include_Path=-I"public" -I"public/core" -I"public/platform" -I"src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D _DEBUG -D GCC_BUILD 
Release_Preprocessor_Definitions=-D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D CROSSLINK_LIB -D GCC_BUILD 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-O0 
Release_Compiler_Flags=-O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/public/Utils/Tokenizer.o gccDebug/public/VirtualStream.o gccDebug/public/Platform/Platform.o gccDebug/public/math/FixedMath.o gccDebug/public/math/FVector.o gccDebug/public/Math/Matrix.o gccDebug/public/math/QuadTree.o gccDebug/public/Math/Quaternion.o gccDebug/public/Math/Random.o gccDebug/public/Math/SIMD.o gccDebug/public/Math/Vector.o gccDebug/public/Math/Volume.o gccDebug/public/Utils/CRC32.o gccDebug/public/utils/eqjobs.o gccDebug/public/Utils/eqthread.o gccDebug/public/utils/eqwstring.o gccDebug/public/utils/KeyValues.o gccDebug/public/Utils/strtools.o gccDebug/public/Utils/eqtimer.o gccDebug/public/Utils/eqstring.o gccDebug/public/Network/c_udp.o gccDebug/public/Network/NetMessageBuffer.o gccDebug/public/Network/NetInterfaces.o gccDebug/public/Network/net_defs.o gccDebug/public/Imaging/ImageLoader.o 
	ar rcs gccDebug/libFrameworkLib.a gccDebug/public/Utils/Tokenizer.o gccDebug/public/VirtualStream.o gccDebug/public/Platform/Platform.o gccDebug/public/math/FixedMath.o gccDebug/public/math/FVector.o gccDebug/public/Math/Matrix.o gccDebug/public/math/QuadTree.o gccDebug/public/Math/Quaternion.o gccDebug/public/Math/Random.o gccDebug/public/Math/SIMD.o gccDebug/public/Math/Vector.o gccDebug/public/Math/Volume.o gccDebug/public/Utils/CRC32.o gccDebug/public/utils/eqjobs.o gccDebug/public/Utils/eqthread.o gccDebug/public/utils/eqwstring.o gccDebug/public/utils/KeyValues.o gccDebug/public/Utils/strtools.o gccDebug/public/Utils/eqtimer.o gccDebug/public/Utils/eqstring.o gccDebug/public/Network/c_udp.o gccDebug/public/Network/NetMessageBuffer.o gccDebug/public/Network/NetInterfaces.o gccDebug/public/Network/net_defs.o gccDebug/public/Imaging/ImageLoader.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file public/Utils/Tokenizer.cpp for the Debug configuration...
-include gccDebug/public/Utils/Tokenizer.d
gccDebug/public/Utils/Tokenizer.o: public/Utils/Tokenizer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Utils/Tokenizer.cpp $(Debug_Include_Path) -o gccDebug/public/Utils/Tokenizer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Utils/Tokenizer.cpp $(Debug_Include_Path) > gccDebug/public/Utils/Tokenizer.d

# Compiles file public/VirtualStream.cpp for the Debug configuration...
-include gccDebug/public/VirtualStream.d
gccDebug/public/VirtualStream.o: public/VirtualStream.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/VirtualStream.cpp $(Debug_Include_Path) -o gccDebug/public/VirtualStream.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/VirtualStream.cpp $(Debug_Include_Path) > gccDebug/public/VirtualStream.d

# Compiles file public/Platform/Platform.cpp for the Debug configuration...
-include gccDebug/public/Platform/Platform.d
gccDebug/public/Platform/Platform.o: public/Platform/Platform.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Platform/Platform.cpp $(Debug_Include_Path) -o gccDebug/public/Platform/Platform.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Platform/Platform.cpp $(Debug_Include_Path) > gccDebug/public/Platform/Platform.d

# Compiles file public/math/FixedMath.cpp for the Debug configuration...
-include gccDebug/public/math/FixedMath.d
gccDebug/public/math/FixedMath.o: public/math/FixedMath.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/math/FixedMath.cpp $(Debug_Include_Path) -o gccDebug/public/math/FixedMath.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/math/FixedMath.cpp $(Debug_Include_Path) > gccDebug/public/math/FixedMath.d

# Compiles file public/math/FVector.cpp for the Debug configuration...
-include gccDebug/public/math/FVector.d
gccDebug/public/math/FVector.o: public/math/FVector.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/math/FVector.cpp $(Debug_Include_Path) -o gccDebug/public/math/FVector.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/math/FVector.cpp $(Debug_Include_Path) > gccDebug/public/math/FVector.d

# Compiles file public/Math/Matrix.cpp for the Debug configuration...
-include gccDebug/public/Math/Matrix.d
gccDebug/public/Math/Matrix.o: public/Math/Matrix.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Math/Matrix.cpp $(Debug_Include_Path) -o gccDebug/public/Math/Matrix.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Math/Matrix.cpp $(Debug_Include_Path) > gccDebug/public/Math/Matrix.d

# Compiles file public/math/QuadTree.cpp for the Debug configuration...
-include gccDebug/public/math/QuadTree.d
gccDebug/public/math/QuadTree.o: public/math/QuadTree.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/math/QuadTree.cpp $(Debug_Include_Path) -o gccDebug/public/math/QuadTree.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/math/QuadTree.cpp $(Debug_Include_Path) > gccDebug/public/math/QuadTree.d

# Compiles file public/Math/Quaternion.cpp for the Debug configuration...
-include gccDebug/public/Math/Quaternion.d
gccDebug/public/Math/Quaternion.o: public/Math/Quaternion.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Math/Quaternion.cpp $(Debug_Include_Path) -o gccDebug/public/Math/Quaternion.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Math/Quaternion.cpp $(Debug_Include_Path) > gccDebug/public/Math/Quaternion.d

# Compiles file public/Math/Random.cpp for the Debug configuration...
-include gccDebug/public/Math/Random.d
gccDebug/public/Math/Random.o: public/Math/Random.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Math/Random.cpp $(Debug_Include_Path) -o gccDebug/public/Math/Random.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Math/Random.cpp $(Debug_Include_Path) > gccDebug/public/Math/Random.d

# Compiles file public/Math/SIMD.cpp for the Debug configuration...
-include gccDebug/public/Math/SIMD.d
gccDebug/public/Math/SIMD.o: public/Math/SIMD.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Math/SIMD.cpp $(Debug_Include_Path) -o gccDebug/public/Math/SIMD.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Math/SIMD.cpp $(Debug_Include_Path) > gccDebug/public/Math/SIMD.d

# Compiles file public/Math/Vector.cpp for the Debug configuration...
-include gccDebug/public/Math/Vector.d
gccDebug/public/Math/Vector.o: public/Math/Vector.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Math/Vector.cpp $(Debug_Include_Path) -o gccDebug/public/Math/Vector.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Math/Vector.cpp $(Debug_Include_Path) > gccDebug/public/Math/Vector.d

# Compiles file public/Math/Volume.cpp for the Debug configuration...
-include gccDebug/public/Math/Volume.d
gccDebug/public/Math/Volume.o: public/Math/Volume.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Math/Volume.cpp $(Debug_Include_Path) -o gccDebug/public/Math/Volume.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Math/Volume.cpp $(Debug_Include_Path) > gccDebug/public/Math/Volume.d

# Compiles file public/Utils/CRC32.cpp for the Debug configuration...
-include gccDebug/public/Utils/CRC32.d
gccDebug/public/Utils/CRC32.o: public/Utils/CRC32.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Utils/CRC32.cpp $(Debug_Include_Path) -o gccDebug/public/Utils/CRC32.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Utils/CRC32.cpp $(Debug_Include_Path) > gccDebug/public/Utils/CRC32.d

# Compiles file public/utils/eqjobs.cpp for the Debug configuration...
-include gccDebug/public/utils/eqjobs.d
gccDebug/public/utils/eqjobs.o: public/utils/eqjobs.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/utils/eqjobs.cpp $(Debug_Include_Path) -o gccDebug/public/utils/eqjobs.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/utils/eqjobs.cpp $(Debug_Include_Path) > gccDebug/public/utils/eqjobs.d

# Compiles file public/Utils/eqthread.cpp for the Debug configuration...
-include gccDebug/public/Utils/eqthread.d
gccDebug/public/Utils/eqthread.o: public/Utils/eqthread.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Utils/eqthread.cpp $(Debug_Include_Path) -o gccDebug/public/Utils/eqthread.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Utils/eqthread.cpp $(Debug_Include_Path) > gccDebug/public/Utils/eqthread.d

# Compiles file public/utils/eqwstring.cpp for the Debug configuration...
-include gccDebug/public/utils/eqwstring.d
gccDebug/public/utils/eqwstring.o: public/utils/eqwstring.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/utils/eqwstring.cpp $(Debug_Include_Path) -o gccDebug/public/utils/eqwstring.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/utils/eqwstring.cpp $(Debug_Include_Path) > gccDebug/public/utils/eqwstring.d

# Compiles file public/utils/KeyValues.cpp for the Debug configuration...
-include gccDebug/public/utils/KeyValues.d
gccDebug/public/utils/KeyValues.o: public/utils/KeyValues.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/utils/KeyValues.cpp $(Debug_Include_Path) -o gccDebug/public/utils/KeyValues.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/utils/KeyValues.cpp $(Debug_Include_Path) > gccDebug/public/utils/KeyValues.d

# Compiles file public/Utils/strtools.cpp for the Debug configuration...
-include gccDebug/public/Utils/strtools.d
gccDebug/public/Utils/strtools.o: public/Utils/strtools.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Utils/strtools.cpp $(Debug_Include_Path) -o gccDebug/public/Utils/strtools.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Utils/strtools.cpp $(Debug_Include_Path) > gccDebug/public/Utils/strtools.d

# Compiles file public/Utils/eqtimer.cpp for the Debug configuration...
-include gccDebug/public/Utils/eqtimer.d
gccDebug/public/Utils/eqtimer.o: public/Utils/eqtimer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Utils/eqtimer.cpp $(Debug_Include_Path) -o gccDebug/public/Utils/eqtimer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Utils/eqtimer.cpp $(Debug_Include_Path) > gccDebug/public/Utils/eqtimer.d

# Compiles file public/Utils/eqstring.cpp for the Debug configuration...
-include gccDebug/public/Utils/eqstring.d
gccDebug/public/Utils/eqstring.o: public/Utils/eqstring.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Utils/eqstring.cpp $(Debug_Include_Path) -o gccDebug/public/Utils/eqstring.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Utils/eqstring.cpp $(Debug_Include_Path) > gccDebug/public/Utils/eqstring.d

# Compiles file public/Network/c_udp.cpp for the Debug configuration...
-include gccDebug/public/Network/c_udp.d
gccDebug/public/Network/c_udp.o: public/Network/c_udp.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Network/c_udp.cpp $(Debug_Include_Path) -o gccDebug/public/Network/c_udp.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Network/c_udp.cpp $(Debug_Include_Path) > gccDebug/public/Network/c_udp.d

# Compiles file public/Network/NetMessageBuffer.cpp for the Debug configuration...
-include gccDebug/public/Network/NetMessageBuffer.d
gccDebug/public/Network/NetMessageBuffer.o: public/Network/NetMessageBuffer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Network/NetMessageBuffer.cpp $(Debug_Include_Path) -o gccDebug/public/Network/NetMessageBuffer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Network/NetMessageBuffer.cpp $(Debug_Include_Path) > gccDebug/public/Network/NetMessageBuffer.d

# Compiles file public/Network/NetInterfaces.cpp for the Debug configuration...
-include gccDebug/public/Network/NetInterfaces.d
gccDebug/public/Network/NetInterfaces.o: public/Network/NetInterfaces.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Network/NetInterfaces.cpp $(Debug_Include_Path) -o gccDebug/public/Network/NetInterfaces.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Network/NetInterfaces.cpp $(Debug_Include_Path) > gccDebug/public/Network/NetInterfaces.d

# Compiles file public/Network/net_defs.cpp for the Debug configuration...
-include gccDebug/public/Network/net_defs.d
gccDebug/public/Network/net_defs.o: public/Network/net_defs.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Network/net_defs.cpp $(Debug_Include_Path) -o gccDebug/public/Network/net_defs.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Network/net_defs.cpp $(Debug_Include_Path) > gccDebug/public/Network/net_defs.d

# Compiles file public/Imaging/ImageLoader.cpp for the Debug configuration...
-include gccDebug/public/Imaging/ImageLoader.d
gccDebug/public/Imaging/ImageLoader.o: public/Imaging/ImageLoader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/Imaging/ImageLoader.cpp $(Debug_Include_Path) -o gccDebug/public/Imaging/ImageLoader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/Imaging/ImageLoader.cpp $(Debug_Include_Path) > gccDebug/public/Imaging/ImageLoader.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/public/Utils/Tokenizer.o gccRelease/public/VirtualStream.o gccRelease/public/Platform/Platform.o gccRelease/public/math/FixedMath.o gccRelease/public/math/FVector.o gccRelease/public/Math/Matrix.o gccRelease/public/math/QuadTree.o gccRelease/public/Math/Quaternion.o gccRelease/public/Math/Random.o gccRelease/public/Math/SIMD.o gccRelease/public/Math/Vector.o gccRelease/public/Math/Volume.o gccRelease/public/Utils/CRC32.o gccRelease/public/utils/eqjobs.o gccRelease/public/Utils/eqthread.o gccRelease/public/utils/eqwstring.o gccRelease/public/utils/KeyValues.o gccRelease/public/Utils/strtools.o gccRelease/public/Utils/eqtimer.o gccRelease/public/Utils/eqstring.o gccRelease/public/Network/c_udp.o gccRelease/public/Network/NetMessageBuffer.o gccRelease/public/Network/NetInterfaces.o gccRelease/public/Network/net_defs.o gccRelease/public/Imaging/ImageLoader.o 
	ar rcs gccRelease/libFrameworkLib.a gccRelease/public/Utils/Tokenizer.o gccRelease/public/VirtualStream.o gccRelease/public/Platform/Platform.o gccRelease/public/math/FixedMath.o gccRelease/public/math/FVector.o gccRelease/public/Math/Matrix.o gccRelease/public/math/QuadTree.o gccRelease/public/Math/Quaternion.o gccRelease/public/Math/Random.o gccRelease/public/Math/SIMD.o gccRelease/public/Math/Vector.o gccRelease/public/Math/Volume.o gccRelease/public/Utils/CRC32.o gccRelease/public/utils/eqjobs.o gccRelease/public/Utils/eqthread.o gccRelease/public/utils/eqwstring.o gccRelease/public/utils/KeyValues.o gccRelease/public/Utils/strtools.o gccRelease/public/Utils/eqtimer.o gccRelease/public/Utils/eqstring.o gccRelease/public/Network/c_udp.o gccRelease/public/Network/NetMessageBuffer.o gccRelease/public/Network/NetInterfaces.o gccRelease/public/Network/net_defs.o gccRelease/public/Imaging/ImageLoader.o  $(Release_Implicitly_Linked_Objects)

# Compiles file public/Utils/Tokenizer.cpp for the Release configuration...
-include gccRelease/public/Utils/Tokenizer.d
gccRelease/public/Utils/Tokenizer.o: public/Utils/Tokenizer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Utils/Tokenizer.cpp $(Release_Include_Path) -o gccRelease/public/Utils/Tokenizer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Utils/Tokenizer.cpp $(Release_Include_Path) > gccRelease/public/Utils/Tokenizer.d

# Compiles file public/VirtualStream.cpp for the Release configuration...
-include gccRelease/public/VirtualStream.d
gccRelease/public/VirtualStream.o: public/VirtualStream.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/VirtualStream.cpp $(Release_Include_Path) -o gccRelease/public/VirtualStream.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/VirtualStream.cpp $(Release_Include_Path) > gccRelease/public/VirtualStream.d

# Compiles file public/Platform/Platform.cpp for the Release configuration...
-include gccRelease/public/Platform/Platform.d
gccRelease/public/Platform/Platform.o: public/Platform/Platform.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Platform/Platform.cpp $(Release_Include_Path) -o gccRelease/public/Platform/Platform.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Platform/Platform.cpp $(Release_Include_Path) > gccRelease/public/Platform/Platform.d

# Compiles file public/math/FixedMath.cpp for the Release configuration...
-include gccRelease/public/math/FixedMath.d
gccRelease/public/math/FixedMath.o: public/math/FixedMath.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/math/FixedMath.cpp $(Release_Include_Path) -o gccRelease/public/math/FixedMath.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/math/FixedMath.cpp $(Release_Include_Path) > gccRelease/public/math/FixedMath.d

# Compiles file public/math/FVector.cpp for the Release configuration...
-include gccRelease/public/math/FVector.d
gccRelease/public/math/FVector.o: public/math/FVector.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/math/FVector.cpp $(Release_Include_Path) -o gccRelease/public/math/FVector.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/math/FVector.cpp $(Release_Include_Path) > gccRelease/public/math/FVector.d

# Compiles file public/Math/Matrix.cpp for the Release configuration...
-include gccRelease/public/Math/Matrix.d
gccRelease/public/Math/Matrix.o: public/Math/Matrix.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Math/Matrix.cpp $(Release_Include_Path) -o gccRelease/public/Math/Matrix.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Math/Matrix.cpp $(Release_Include_Path) > gccRelease/public/Math/Matrix.d

# Compiles file public/math/QuadTree.cpp for the Release configuration...
-include gccRelease/public/math/QuadTree.d
gccRelease/public/math/QuadTree.o: public/math/QuadTree.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/math/QuadTree.cpp $(Release_Include_Path) -o gccRelease/public/math/QuadTree.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/math/QuadTree.cpp $(Release_Include_Path) > gccRelease/public/math/QuadTree.d

# Compiles file public/Math/Quaternion.cpp for the Release configuration...
-include gccRelease/public/Math/Quaternion.d
gccRelease/public/Math/Quaternion.o: public/Math/Quaternion.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Math/Quaternion.cpp $(Release_Include_Path) -o gccRelease/public/Math/Quaternion.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Math/Quaternion.cpp $(Release_Include_Path) > gccRelease/public/Math/Quaternion.d

# Compiles file public/Math/Random.cpp for the Release configuration...
-include gccRelease/public/Math/Random.d
gccRelease/public/Math/Random.o: public/Math/Random.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Math/Random.cpp $(Release_Include_Path) -o gccRelease/public/Math/Random.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Math/Random.cpp $(Release_Include_Path) > gccRelease/public/Math/Random.d

# Compiles file public/Math/SIMD.cpp for the Release configuration...
-include gccRelease/public/Math/SIMD.d
gccRelease/public/Math/SIMD.o: public/Math/SIMD.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Math/SIMD.cpp $(Release_Include_Path) -o gccRelease/public/Math/SIMD.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Math/SIMD.cpp $(Release_Include_Path) > gccRelease/public/Math/SIMD.d

# Compiles file public/Math/Vector.cpp for the Release configuration...
-include gccRelease/public/Math/Vector.d
gccRelease/public/Math/Vector.o: public/Math/Vector.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Math/Vector.cpp $(Release_Include_Path) -o gccRelease/public/Math/Vector.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Math/Vector.cpp $(Release_Include_Path) > gccRelease/public/Math/Vector.d

# Compiles file public/Math/Volume.cpp for the Release configuration...
-include gccRelease/public/Math/Volume.d
gccRelease/public/Math/Volume.o: public/Math/Volume.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Math/Volume.cpp $(Release_Include_Path) -o gccRelease/public/Math/Volume.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Math/Volume.cpp $(Release_Include_Path) > gccRelease/public/Math/Volume.d

# Compiles file public/Utils/CRC32.cpp for the Release configuration...
-include gccRelease/public/Utils/CRC32.d
gccRelease/public/Utils/CRC32.o: public/Utils/CRC32.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Utils/CRC32.cpp $(Release_Include_Path) -o gccRelease/public/Utils/CRC32.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Utils/CRC32.cpp $(Release_Include_Path) > gccRelease/public/Utils/CRC32.d

# Compiles file public/utils/eqjobs.cpp for the Release configuration...
-include gccRelease/public/utils/eqjobs.d
gccRelease/public/utils/eqjobs.o: public/utils/eqjobs.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/utils/eqjobs.cpp $(Release_Include_Path) -o gccRelease/public/utils/eqjobs.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/utils/eqjobs.cpp $(Release_Include_Path) > gccRelease/public/utils/eqjobs.d

# Compiles file public/Utils/eqthread.cpp for the Release configuration...
-include gccRelease/public/Utils/eqthread.d
gccRelease/public/Utils/eqthread.o: public/Utils/eqthread.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Utils/eqthread.cpp $(Release_Include_Path) -o gccRelease/public/Utils/eqthread.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Utils/eqthread.cpp $(Release_Include_Path) > gccRelease/public/Utils/eqthread.d

# Compiles file public/utils/eqwstring.cpp for the Release configuration...
-include gccRelease/public/utils/eqwstring.d
gccRelease/public/utils/eqwstring.o: public/utils/eqwstring.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/utils/eqwstring.cpp $(Release_Include_Path) -o gccRelease/public/utils/eqwstring.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/utils/eqwstring.cpp $(Release_Include_Path) > gccRelease/public/utils/eqwstring.d

# Compiles file public/utils/KeyValues.cpp for the Release configuration...
-include gccRelease/public/utils/KeyValues.d
gccRelease/public/utils/KeyValues.o: public/utils/KeyValues.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/utils/KeyValues.cpp $(Release_Include_Path) -o gccRelease/public/utils/KeyValues.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/utils/KeyValues.cpp $(Release_Include_Path) > gccRelease/public/utils/KeyValues.d

# Compiles file public/Utils/strtools.cpp for the Release configuration...
-include gccRelease/public/Utils/strtools.d
gccRelease/public/Utils/strtools.o: public/Utils/strtools.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Utils/strtools.cpp $(Release_Include_Path) -o gccRelease/public/Utils/strtools.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Utils/strtools.cpp $(Release_Include_Path) > gccRelease/public/Utils/strtools.d

# Compiles file public/Utils/eqtimer.cpp for the Release configuration...
-include gccRelease/public/Utils/eqtimer.d
gccRelease/public/Utils/eqtimer.o: public/Utils/eqtimer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Utils/eqtimer.cpp $(Release_Include_Path) -o gccRelease/public/Utils/eqtimer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Utils/eqtimer.cpp $(Release_Include_Path) > gccRelease/public/Utils/eqtimer.d

# Compiles file public/Utils/eqstring.cpp for the Release configuration...
-include gccRelease/public/Utils/eqstring.d
gccRelease/public/Utils/eqstring.o: public/Utils/eqstring.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Utils/eqstring.cpp $(Release_Include_Path) -o gccRelease/public/Utils/eqstring.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Utils/eqstring.cpp $(Release_Include_Path) > gccRelease/public/Utils/eqstring.d

# Compiles file public/Network/c_udp.cpp for the Release configuration...
-include gccRelease/public/Network/c_udp.d
gccRelease/public/Network/c_udp.o: public/Network/c_udp.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Network/c_udp.cpp $(Release_Include_Path) -o gccRelease/public/Network/c_udp.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Network/c_udp.cpp $(Release_Include_Path) > gccRelease/public/Network/c_udp.d

# Compiles file public/Network/NetMessageBuffer.cpp for the Release configuration...
-include gccRelease/public/Network/NetMessageBuffer.d
gccRelease/public/Network/NetMessageBuffer.o: public/Network/NetMessageBuffer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Network/NetMessageBuffer.cpp $(Release_Include_Path) -o gccRelease/public/Network/NetMessageBuffer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Network/NetMessageBuffer.cpp $(Release_Include_Path) > gccRelease/public/Network/NetMessageBuffer.d

# Compiles file public/Network/NetInterfaces.cpp for the Release configuration...
-include gccRelease/public/Network/NetInterfaces.d
gccRelease/public/Network/NetInterfaces.o: public/Network/NetInterfaces.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Network/NetInterfaces.cpp $(Release_Include_Path) -o gccRelease/public/Network/NetInterfaces.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Network/NetInterfaces.cpp $(Release_Include_Path) > gccRelease/public/Network/NetInterfaces.d

# Compiles file public/Network/net_defs.cpp for the Release configuration...
-include gccRelease/public/Network/net_defs.d
gccRelease/public/Network/net_defs.o: public/Network/net_defs.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Network/net_defs.cpp $(Release_Include_Path) -o gccRelease/public/Network/net_defs.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Network/net_defs.cpp $(Release_Include_Path) > gccRelease/public/Network/net_defs.d

# Compiles file public/Imaging/ImageLoader.cpp for the Release configuration...
-include gccRelease/public/Imaging/ImageLoader.d
gccRelease/public/Imaging/ImageLoader.o: public/Imaging/ImageLoader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/Imaging/ImageLoader.cpp $(Release_Include_Path) -o gccRelease/public/Imaging/ImageLoader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/Imaging/ImageLoader.cpp $(Release_Include_Path) > gccRelease/public/Imaging/ImageLoader.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug/source
	mkdir -p gccRelease/source

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccDebug/*.o
	rm -f gccDebug/*.d
	rm -f gccDebug/*.a
	rm -f gccDebug/*.so
	rm -f gccDebug/*.dll
	rm -f gccDebug/*.exe
	rm -f gccRelease/*.o
	rm -f gccRelease/*.d
	rm -f gccRelease/*.a
	rm -f gccRelease/*.so
	rm -f gccRelease/*.dll
	rm -f gccRelease/*.exe

