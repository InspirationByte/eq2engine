# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../public/platform" -I"../../src_dependency/sdl2/include" -I"../../public/materialsystem/renderers" 
Release_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../public/platform" -I"../../src_dependency/sdl2/include" -I"../../public/materialsystem/renderers" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lgdi32 -lshell32 -lwinmm -lcomctl32  -Wl,--end-group
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D USE_SIMD -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D USE_SIMD -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-fPIC -O0 -g 
Release_Compiler_Flags=-fPIC -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/../../public/BaseShader.o gccDebug/ShadersMain.o gccDebug/BaseDecal.o gccDebug/BaseSingle.o gccDebug/BaseVertexTransDecal4.o gccDebug/BaseVertexTransition4.o gccDebug/BleachBypass.o gccDebug/Blur.o gccDebug/DeferredPointlight.o gccDebug/DeferredSpotLight.o gccDebug/DeferredSunLight.o gccDebug/DepthWriteLighting.o gccDebug/ErrorShader.o gccDebug/FlashlightReflector.o gccDebug/HDRBloom.o gccDebug/HDRBlur.o gccDebug/BaseParticle.o gccDebug/shaders/BaseUnlit.o gccDebug/Shaders/DeferredAmbient.o gccDebug/shaders/InternalShaders.o gccDebug/ShadersOverride.o gccDebug/shaders/SkyBox.o gccDebug/SkinnedModel.o gccDebug/TestShader.o gccDebug/Water.o gccDebug/../../public/ViewParams.o 
	g++ -fPIC -shared -Wl,-soname,libEqBaseShaders.so -o gccDebug/libEqBaseShaders.so gccDebug/../../public/BaseShader.o gccDebug/ShadersMain.o gccDebug/BaseDecal.o gccDebug/BaseSingle.o gccDebug/BaseVertexTransDecal4.o gccDebug/BaseVertexTransition4.o gccDebug/BleachBypass.o gccDebug/Blur.o gccDebug/DeferredPointlight.o gccDebug/DeferredSpotLight.o gccDebug/DeferredSunLight.o gccDebug/DepthWriteLighting.o gccDebug/ErrorShader.o gccDebug/FlashlightReflector.o gccDebug/HDRBloom.o gccDebug/HDRBlur.o gccDebug/BaseParticle.o gccDebug/shaders/BaseUnlit.o gccDebug/Shaders/DeferredAmbient.o gccDebug/shaders/InternalShaders.o gccDebug/ShadersOverride.o gccDebug/shaders/SkyBox.o gccDebug/SkinnedModel.o gccDebug/TestShader.o gccDebug/Water.o gccDebug/../../public/ViewParams.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file ../../public/BaseShader.cpp for the Debug configuration...
-include gccDebug/../../public/BaseShader.d
gccDebug/../../public/BaseShader.o: ../../public/BaseShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/BaseShader.cpp $(Debug_Include_Path) -o gccDebug/../../public/BaseShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/BaseShader.cpp $(Debug_Include_Path) > gccDebug/../../public/BaseShader.d

# Compiles file ShadersMain.cpp for the Debug configuration...
-include gccDebug/ShadersMain.d
gccDebug/ShadersMain.o: ShadersMain.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ShadersMain.cpp $(Debug_Include_Path) -o gccDebug/ShadersMain.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ShadersMain.cpp $(Debug_Include_Path) > gccDebug/ShadersMain.d

# Compiles file BaseDecal.cpp for the Debug configuration...
-include gccDebug/BaseDecal.d
gccDebug/BaseDecal.o: BaseDecal.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseDecal.cpp $(Debug_Include_Path) -o gccDebug/BaseDecal.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseDecal.cpp $(Debug_Include_Path) > gccDebug/BaseDecal.d

# Compiles file BaseSingle.cpp for the Debug configuration...
-include gccDebug/BaseSingle.d
gccDebug/BaseSingle.o: BaseSingle.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseSingle.cpp $(Debug_Include_Path) -o gccDebug/BaseSingle.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseSingle.cpp $(Debug_Include_Path) > gccDebug/BaseSingle.d

# Compiles file BaseVertexTransDecal4.cpp for the Debug configuration...
-include gccDebug/BaseVertexTransDecal4.d
gccDebug/BaseVertexTransDecal4.o: BaseVertexTransDecal4.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseVertexTransDecal4.cpp $(Debug_Include_Path) -o gccDebug/BaseVertexTransDecal4.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseVertexTransDecal4.cpp $(Debug_Include_Path) > gccDebug/BaseVertexTransDecal4.d

# Compiles file BaseVertexTransition4.cpp for the Debug configuration...
-include gccDebug/BaseVertexTransition4.d
gccDebug/BaseVertexTransition4.o: BaseVertexTransition4.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseVertexTransition4.cpp $(Debug_Include_Path) -o gccDebug/BaseVertexTransition4.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseVertexTransition4.cpp $(Debug_Include_Path) > gccDebug/BaseVertexTransition4.d

# Compiles file BleachBypass.cpp for the Debug configuration...
-include gccDebug/BleachBypass.d
gccDebug/BleachBypass.o: BleachBypass.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BleachBypass.cpp $(Debug_Include_Path) -o gccDebug/BleachBypass.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BleachBypass.cpp $(Debug_Include_Path) > gccDebug/BleachBypass.d

# Compiles file Blur.cpp for the Debug configuration...
-include gccDebug/Blur.d
gccDebug/Blur.o: Blur.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Blur.cpp $(Debug_Include_Path) -o gccDebug/Blur.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Blur.cpp $(Debug_Include_Path) > gccDebug/Blur.d

# Compiles file DeferredPointlight.cpp for the Debug configuration...
-include gccDebug/DeferredPointlight.d
gccDebug/DeferredPointlight.o: DeferredPointlight.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DeferredPointlight.cpp $(Debug_Include_Path) -o gccDebug/DeferredPointlight.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DeferredPointlight.cpp $(Debug_Include_Path) > gccDebug/DeferredPointlight.d

# Compiles file DeferredSpotLight.cpp for the Debug configuration...
-include gccDebug/DeferredSpotLight.d
gccDebug/DeferredSpotLight.o: DeferredSpotLight.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DeferredSpotLight.cpp $(Debug_Include_Path) -o gccDebug/DeferredSpotLight.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DeferredSpotLight.cpp $(Debug_Include_Path) > gccDebug/DeferredSpotLight.d

# Compiles file DeferredSunLight.cpp for the Debug configuration...
-include gccDebug/DeferredSunLight.d
gccDebug/DeferredSunLight.o: DeferredSunLight.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DeferredSunLight.cpp $(Debug_Include_Path) -o gccDebug/DeferredSunLight.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DeferredSunLight.cpp $(Debug_Include_Path) > gccDebug/DeferredSunLight.d

# Compiles file DepthWriteLighting.cpp for the Debug configuration...
-include gccDebug/DepthWriteLighting.d
gccDebug/DepthWriteLighting.o: DepthWriteLighting.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DepthWriteLighting.cpp $(Debug_Include_Path) -o gccDebug/DepthWriteLighting.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DepthWriteLighting.cpp $(Debug_Include_Path) > gccDebug/DepthWriteLighting.d

# Compiles file ErrorShader.cpp for the Debug configuration...
-include gccDebug/ErrorShader.d
gccDebug/ErrorShader.o: ErrorShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ErrorShader.cpp $(Debug_Include_Path) -o gccDebug/ErrorShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ErrorShader.cpp $(Debug_Include_Path) > gccDebug/ErrorShader.d

# Compiles file FlashlightReflector.cpp for the Debug configuration...
-include gccDebug/FlashlightReflector.d
gccDebug/FlashlightReflector.o: FlashlightReflector.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c FlashlightReflector.cpp $(Debug_Include_Path) -o gccDebug/FlashlightReflector.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM FlashlightReflector.cpp $(Debug_Include_Path) > gccDebug/FlashlightReflector.d

# Compiles file HDRBloom.cpp for the Debug configuration...
-include gccDebug/HDRBloom.d
gccDebug/HDRBloom.o: HDRBloom.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c HDRBloom.cpp $(Debug_Include_Path) -o gccDebug/HDRBloom.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM HDRBloom.cpp $(Debug_Include_Path) > gccDebug/HDRBloom.d

# Compiles file HDRBlur.cpp for the Debug configuration...
-include gccDebug/HDRBlur.d
gccDebug/HDRBlur.o: HDRBlur.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c HDRBlur.cpp $(Debug_Include_Path) -o gccDebug/HDRBlur.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM HDRBlur.cpp $(Debug_Include_Path) > gccDebug/HDRBlur.d

# Compiles file BaseParticle.cpp for the Debug configuration...
-include gccDebug/BaseParticle.d
gccDebug/BaseParticle.o: BaseParticle.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseParticle.cpp $(Debug_Include_Path) -o gccDebug/BaseParticle.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseParticle.cpp $(Debug_Include_Path) > gccDebug/BaseParticle.d

# Compiles file shaders/BaseUnlit.cpp for the Debug configuration...
-include gccDebug/shaders/BaseUnlit.d
gccDebug/shaders/BaseUnlit.o: shaders/BaseUnlit.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c shaders/BaseUnlit.cpp $(Debug_Include_Path) -o gccDebug/shaders/BaseUnlit.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM shaders/BaseUnlit.cpp $(Debug_Include_Path) > gccDebug/shaders/BaseUnlit.d

# Compiles file Shaders/DeferredAmbient.cpp for the Debug configuration...
-include gccDebug/Shaders/DeferredAmbient.d
gccDebug/Shaders/DeferredAmbient.o: Shaders/DeferredAmbient.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Shaders/DeferredAmbient.cpp $(Debug_Include_Path) -o gccDebug/Shaders/DeferredAmbient.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Shaders/DeferredAmbient.cpp $(Debug_Include_Path) > gccDebug/Shaders/DeferredAmbient.d

# Compiles file shaders/InternalShaders.cpp for the Debug configuration...
-include gccDebug/shaders/InternalShaders.d
gccDebug/shaders/InternalShaders.o: shaders/InternalShaders.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c shaders/InternalShaders.cpp $(Debug_Include_Path) -o gccDebug/shaders/InternalShaders.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM shaders/InternalShaders.cpp $(Debug_Include_Path) > gccDebug/shaders/InternalShaders.d

# Compiles file ShadersOverride.cpp for the Debug configuration...
-include gccDebug/ShadersOverride.d
gccDebug/ShadersOverride.o: ShadersOverride.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ShadersOverride.cpp $(Debug_Include_Path) -o gccDebug/ShadersOverride.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ShadersOverride.cpp $(Debug_Include_Path) > gccDebug/ShadersOverride.d

# Compiles file shaders/SkyBox.cpp for the Debug configuration...
-include gccDebug/shaders/SkyBox.d
gccDebug/shaders/SkyBox.o: shaders/SkyBox.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c shaders/SkyBox.cpp $(Debug_Include_Path) -o gccDebug/shaders/SkyBox.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM shaders/SkyBox.cpp $(Debug_Include_Path) > gccDebug/shaders/SkyBox.d

# Compiles file SkinnedModel.cpp for the Debug configuration...
-include gccDebug/SkinnedModel.d
gccDebug/SkinnedModel.o: SkinnedModel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SkinnedModel.cpp $(Debug_Include_Path) -o gccDebug/SkinnedModel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SkinnedModel.cpp $(Debug_Include_Path) > gccDebug/SkinnedModel.d

# Compiles file TestShader.cpp for the Debug configuration...
-include gccDebug/TestShader.d
gccDebug/TestShader.o: TestShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c TestShader.cpp $(Debug_Include_Path) -o gccDebug/TestShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM TestShader.cpp $(Debug_Include_Path) > gccDebug/TestShader.d

# Compiles file Water.cpp for the Debug configuration...
-include gccDebug/Water.d
gccDebug/Water.o: Water.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Water.cpp $(Debug_Include_Path) -o gccDebug/Water.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Water.cpp $(Debug_Include_Path) > gccDebug/Water.d

# Compiles file ../../public/ViewParams.cpp for the Debug configuration...
-include gccDebug/../../public/ViewParams.d
gccDebug/../../public/ViewParams.o: ../../public/ViewParams.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/ViewParams.cpp $(Debug_Include_Path) -o gccDebug/../../public/ViewParams.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/ViewParams.cpp $(Debug_Include_Path) > gccDebug/../../public/ViewParams.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../public/BaseShader.o gccRelease/ShadersMain.o gccRelease/BaseDecal.o gccRelease/BaseSingle.o gccRelease/BaseVertexTransDecal4.o gccRelease/BaseVertexTransition4.o gccRelease/BleachBypass.o gccRelease/Blur.o gccRelease/DeferredPointlight.o gccRelease/DeferredSpotLight.o gccRelease/DeferredSunLight.o gccRelease/DepthWriteLighting.o gccRelease/ErrorShader.o gccRelease/FlashlightReflector.o gccRelease/HDRBloom.o gccRelease/HDRBlur.o gccRelease/BaseParticle.o gccRelease/shaders/BaseUnlit.o gccRelease/Shaders/DeferredAmbient.o gccRelease/shaders/InternalShaders.o gccRelease/ShadersOverride.o gccRelease/shaders/SkyBox.o gccRelease/SkinnedModel.o gccRelease/TestShader.o gccRelease/Water.o gccRelease/../../public/ViewParams.o 
	g++ -fPIC -shared -Wl,-soname,libEqBaseShaders.so -o gccRelease/libEqBaseShaders.so gccRelease/../../public/BaseShader.o gccRelease/ShadersMain.o gccRelease/BaseDecal.o gccRelease/BaseSingle.o gccRelease/BaseVertexTransDecal4.o gccRelease/BaseVertexTransition4.o gccRelease/BleachBypass.o gccRelease/Blur.o gccRelease/DeferredPointlight.o gccRelease/DeferredSpotLight.o gccRelease/DeferredSunLight.o gccRelease/DepthWriteLighting.o gccRelease/ErrorShader.o gccRelease/FlashlightReflector.o gccRelease/HDRBloom.o gccRelease/HDRBlur.o gccRelease/BaseParticle.o gccRelease/shaders/BaseUnlit.o gccRelease/Shaders/DeferredAmbient.o gccRelease/shaders/InternalShaders.o gccRelease/ShadersOverride.o gccRelease/shaders/SkyBox.o gccRelease/SkinnedModel.o gccRelease/TestShader.o gccRelease/Water.o gccRelease/../../public/ViewParams.o  $(Release_Implicitly_Linked_Objects)

# Compiles file ../../public/BaseShader.cpp for the Release configuration...
-include gccRelease/../../public/BaseShader.d
gccRelease/../../public/BaseShader.o: ../../public/BaseShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/BaseShader.cpp $(Release_Include_Path) -o gccRelease/../../public/BaseShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/BaseShader.cpp $(Release_Include_Path) > gccRelease/../../public/BaseShader.d

# Compiles file ShadersMain.cpp for the Release configuration...
-include gccRelease/ShadersMain.d
gccRelease/ShadersMain.o: ShadersMain.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ShadersMain.cpp $(Release_Include_Path) -o gccRelease/ShadersMain.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ShadersMain.cpp $(Release_Include_Path) > gccRelease/ShadersMain.d

# Compiles file BaseDecal.cpp for the Release configuration...
-include gccRelease/BaseDecal.d
gccRelease/BaseDecal.o: BaseDecal.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseDecal.cpp $(Release_Include_Path) -o gccRelease/BaseDecal.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseDecal.cpp $(Release_Include_Path) > gccRelease/BaseDecal.d

# Compiles file BaseSingle.cpp for the Release configuration...
-include gccRelease/BaseSingle.d
gccRelease/BaseSingle.o: BaseSingle.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseSingle.cpp $(Release_Include_Path) -o gccRelease/BaseSingle.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseSingle.cpp $(Release_Include_Path) > gccRelease/BaseSingle.d

# Compiles file BaseVertexTransDecal4.cpp for the Release configuration...
-include gccRelease/BaseVertexTransDecal4.d
gccRelease/BaseVertexTransDecal4.o: BaseVertexTransDecal4.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseVertexTransDecal4.cpp $(Release_Include_Path) -o gccRelease/BaseVertexTransDecal4.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseVertexTransDecal4.cpp $(Release_Include_Path) > gccRelease/BaseVertexTransDecal4.d

# Compiles file BaseVertexTransition4.cpp for the Release configuration...
-include gccRelease/BaseVertexTransition4.d
gccRelease/BaseVertexTransition4.o: BaseVertexTransition4.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseVertexTransition4.cpp $(Release_Include_Path) -o gccRelease/BaseVertexTransition4.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseVertexTransition4.cpp $(Release_Include_Path) > gccRelease/BaseVertexTransition4.d

# Compiles file BleachBypass.cpp for the Release configuration...
-include gccRelease/BleachBypass.d
gccRelease/BleachBypass.o: BleachBypass.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BleachBypass.cpp $(Release_Include_Path) -o gccRelease/BleachBypass.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BleachBypass.cpp $(Release_Include_Path) > gccRelease/BleachBypass.d

# Compiles file Blur.cpp for the Release configuration...
-include gccRelease/Blur.d
gccRelease/Blur.o: Blur.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Blur.cpp $(Release_Include_Path) -o gccRelease/Blur.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Blur.cpp $(Release_Include_Path) > gccRelease/Blur.d

# Compiles file DeferredPointlight.cpp for the Release configuration...
-include gccRelease/DeferredPointlight.d
gccRelease/DeferredPointlight.o: DeferredPointlight.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DeferredPointlight.cpp $(Release_Include_Path) -o gccRelease/DeferredPointlight.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DeferredPointlight.cpp $(Release_Include_Path) > gccRelease/DeferredPointlight.d

# Compiles file DeferredSpotLight.cpp for the Release configuration...
-include gccRelease/DeferredSpotLight.d
gccRelease/DeferredSpotLight.o: DeferredSpotLight.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DeferredSpotLight.cpp $(Release_Include_Path) -o gccRelease/DeferredSpotLight.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DeferredSpotLight.cpp $(Release_Include_Path) > gccRelease/DeferredSpotLight.d

# Compiles file DeferredSunLight.cpp for the Release configuration...
-include gccRelease/DeferredSunLight.d
gccRelease/DeferredSunLight.o: DeferredSunLight.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DeferredSunLight.cpp $(Release_Include_Path) -o gccRelease/DeferredSunLight.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DeferredSunLight.cpp $(Release_Include_Path) > gccRelease/DeferredSunLight.d

# Compiles file DepthWriteLighting.cpp for the Release configuration...
-include gccRelease/DepthWriteLighting.d
gccRelease/DepthWriteLighting.o: DepthWriteLighting.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DepthWriteLighting.cpp $(Release_Include_Path) -o gccRelease/DepthWriteLighting.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DepthWriteLighting.cpp $(Release_Include_Path) > gccRelease/DepthWriteLighting.d

# Compiles file ErrorShader.cpp for the Release configuration...
-include gccRelease/ErrorShader.d
gccRelease/ErrorShader.o: ErrorShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ErrorShader.cpp $(Release_Include_Path) -o gccRelease/ErrorShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ErrorShader.cpp $(Release_Include_Path) > gccRelease/ErrorShader.d

# Compiles file FlashlightReflector.cpp for the Release configuration...
-include gccRelease/FlashlightReflector.d
gccRelease/FlashlightReflector.o: FlashlightReflector.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c FlashlightReflector.cpp $(Release_Include_Path) -o gccRelease/FlashlightReflector.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM FlashlightReflector.cpp $(Release_Include_Path) > gccRelease/FlashlightReflector.d

# Compiles file HDRBloom.cpp for the Release configuration...
-include gccRelease/HDRBloom.d
gccRelease/HDRBloom.o: HDRBloom.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c HDRBloom.cpp $(Release_Include_Path) -o gccRelease/HDRBloom.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM HDRBloom.cpp $(Release_Include_Path) > gccRelease/HDRBloom.d

# Compiles file HDRBlur.cpp for the Release configuration...
-include gccRelease/HDRBlur.d
gccRelease/HDRBlur.o: HDRBlur.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c HDRBlur.cpp $(Release_Include_Path) -o gccRelease/HDRBlur.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM HDRBlur.cpp $(Release_Include_Path) > gccRelease/HDRBlur.d

# Compiles file BaseParticle.cpp for the Release configuration...
-include gccRelease/BaseParticle.d
gccRelease/BaseParticle.o: BaseParticle.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseParticle.cpp $(Release_Include_Path) -o gccRelease/BaseParticle.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseParticle.cpp $(Release_Include_Path) > gccRelease/BaseParticle.d

# Compiles file shaders/BaseUnlit.cpp for the Release configuration...
-include gccRelease/shaders/BaseUnlit.d
gccRelease/shaders/BaseUnlit.o: shaders/BaseUnlit.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c shaders/BaseUnlit.cpp $(Release_Include_Path) -o gccRelease/shaders/BaseUnlit.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM shaders/BaseUnlit.cpp $(Release_Include_Path) > gccRelease/shaders/BaseUnlit.d

# Compiles file Shaders/DeferredAmbient.cpp for the Release configuration...
-include gccRelease/Shaders/DeferredAmbient.d
gccRelease/Shaders/DeferredAmbient.o: Shaders/DeferredAmbient.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Shaders/DeferredAmbient.cpp $(Release_Include_Path) -o gccRelease/Shaders/DeferredAmbient.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Shaders/DeferredAmbient.cpp $(Release_Include_Path) > gccRelease/Shaders/DeferredAmbient.d

# Compiles file shaders/InternalShaders.cpp for the Release configuration...
-include gccRelease/shaders/InternalShaders.d
gccRelease/shaders/InternalShaders.o: shaders/InternalShaders.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c shaders/InternalShaders.cpp $(Release_Include_Path) -o gccRelease/shaders/InternalShaders.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM shaders/InternalShaders.cpp $(Release_Include_Path) > gccRelease/shaders/InternalShaders.d

# Compiles file ShadersOverride.cpp for the Release configuration...
-include gccRelease/ShadersOverride.d
gccRelease/ShadersOverride.o: ShadersOverride.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ShadersOverride.cpp $(Release_Include_Path) -o gccRelease/ShadersOverride.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ShadersOverride.cpp $(Release_Include_Path) > gccRelease/ShadersOverride.d

# Compiles file shaders/SkyBox.cpp for the Release configuration...
-include gccRelease/shaders/SkyBox.d
gccRelease/shaders/SkyBox.o: shaders/SkyBox.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c shaders/SkyBox.cpp $(Release_Include_Path) -o gccRelease/shaders/SkyBox.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM shaders/SkyBox.cpp $(Release_Include_Path) > gccRelease/shaders/SkyBox.d

# Compiles file SkinnedModel.cpp for the Release configuration...
-include gccRelease/SkinnedModel.d
gccRelease/SkinnedModel.o: SkinnedModel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SkinnedModel.cpp $(Release_Include_Path) -o gccRelease/SkinnedModel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SkinnedModel.cpp $(Release_Include_Path) > gccRelease/SkinnedModel.d

# Compiles file TestShader.cpp for the Release configuration...
-include gccRelease/TestShader.d
gccRelease/TestShader.o: TestShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c TestShader.cpp $(Release_Include_Path) -o gccRelease/TestShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM TestShader.cpp $(Release_Include_Path) > gccRelease/TestShader.d

# Compiles file Water.cpp for the Release configuration...
-include gccRelease/Water.d
gccRelease/Water.o: Water.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Water.cpp $(Release_Include_Path) -o gccRelease/Water.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Water.cpp $(Release_Include_Path) > gccRelease/Water.d

# Compiles file ../../public/ViewParams.cpp for the Release configuration...
-include gccRelease/../../public/ViewParams.d
gccRelease/../../public/ViewParams.o: ../../public/ViewParams.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/ViewParams.cpp $(Release_Include_Path) -o gccRelease/../../public/ViewParams.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/ViewParams.cpp $(Release_Include_Path) > gccRelease/../../public/ViewParams.d

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

