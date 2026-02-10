
openal_ver="1.23.1"
sdl2_ver="2.30.2"
wx_ver="3.2.1"
ffmpeg_ver="master-latest-win64-gpl-shared"
slang_ver="2025.19.1"

windows_openal_url="https://github.com/kcat/openal-soft/releases/download/${openal_ver}/openal-soft-${openal_ver}-bin.zip"
windows_sdl2_url="https://github.com/libsdl-org/SDL/releases/download/release-${sdl2_ver}/SDL2-devel-${sdl2_ver}-VC.zip"
windows_wx_hdrs_url="https://github.com/wxWidgets/wxWidgets/releases/download/v${wx_ver}/wxWidgets-${wx_ver}-headers.7z"
windows_wx_libs_url="https://github.com/wxWidgets/wxWidgets/releases/download/v${wx_ver}/wxMSW-${wx_ver}_vc14x_x64_Dev.7z"
windows_ffmpeg_url="https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-${ffmpeg_ver}.zip"
windows_slang_url="https://github.com/shader-slang/slang/releases/download/v${slang_ver}/slang-${slang_ver}-windows-x86_64.zip"

project_folder="$(pwd)"
dependency_folder="${project_folder}/src_dependency"

echo "Downloading dependencies..."

# Download required dependencies
wget $windows_sdl2_url -O "SDL2.zip"
wget $windows_openal_url -O "OPENAL.zip"
wget $windows_wx_hdrs_url -O "WX_HDRS.7z"
wget $windows_wx_libs_url -O "WX_LIBS.7z"
wget $windows_ffmpeg_url -O "FFMPEG.zip"
wget $windows_slang_url -O "SLANG.zip"

echo "Extracting dependencies..."

unzip "SDL2.zip" -d $dependency_folder
unzip "OPENAL.zip" -d $dependency_folder
7z x "WX_HDRS.7z" -o"${dependency_folder}/wxWidgets"
7z x "WX_LIBS.7z" -o"${dependency_folder}/wxWidgets"
unzip "FFMPEG.zip" -d ${dependency_folder}
unzip "SLANG.zip" -d "${dependency_folder}/slang"

# Generate project files
windows_openal_dir="${dependency_folder}/openal-soft-${openal_ver}-bin"
windows_sdl2_dir="${dependency_folder}/SDL2-${sdl2_ver}"
windows_ffmpeg_dir="${dependency_folder}/ffmpeg-${ffmpeg_ver}"

cp -a -rf "${windows_openal_dir}/." "${dependency_folder}/openal-soft"
cp -a -rf "${windows_sdl2_dir}/." "${dependency_folder}/SDL2"
cp -a -rf "${windows_ffmpeg_dir}/." "${dependency_folder}/ffmpeg"

rm -rf $windows_openal_dir
rm -rf $windows_sdl2_dir
rm -rf $windows_ffmpeg_dir

rm -f "SDL2.zip"
rm -f "OPENAL.zip"
rm -f "WX_HDRS.7z"
rm -f "WX_LIBS.7z"
rm -f "FFMPEG.zip"
rm -f "SLANG.zip"

echo "Completed"

#Set-Location -Path $project_folder

# & .\\premake5 vs2022

# Open solution
# & .\\build\\DriverSyndicate.sln
