
$openal_ver = '1.23.1'
$sdl2_ver = '2.30.2'
$wx_ver = '3.2.1'
$ffmpeg_ver = 'master-latest-win64-gpl-shared'

$windows_openal_url = 'https://openal-soft.org/openal-binaries/openal-soft-' + $openal_ver + '-bin.zip'
$windows_sdl2_url = 'https://github.com/libsdl-org/SDL/releases/download/release-' + $sdl2_ver + '/SDL2-devel-' + $sdl2_ver + '-VC.zip'
$windows_wx_hdrs_url = 'https://github.com/wxWidgets/wxWidgets/releases/download/v' +$wx_ver+ '/wxWidgets-' +$wx_ver+ '-headers.7z'
$windows_wx_libs_url = 'https://github.com/wxWidgets/wxWidgets/releases/download/v' +$wx_ver+ '/wxMSW-' +$wx_ver+ '_vc14x_x64_Dev.7z'
$windows_ffmpeg_url = 'https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-' +$ffmpeg_ver+ '.zip'

$project_folder = '.\\'
$dependency_folder = $project_folder + '\\src_dependency'

# Download required dependencies
Invoke-WebRequest -Uri $windows_sdl2_url -OutFile SDL2.zip
Expand-Archive SDL2.zip -DestinationPath $dependency_folder

Invoke-WebRequest -Uri $windows_openal_url -OutFile OPENAL.zip
Expand-Archive OPENAL.zip -DestinationPath $dependency_folder

Invoke-WebRequest -Uri $windows_wx_hdrs_url -OutFile WX_HDRS.7z
Expand-7Zip -ArchiveFileName WX_HDRS.7z -TargetPath ($dependency_folder + '\\wxWidgets')

Invoke-WebRequest -Uri $windows_wx_libs_url -OutFile WX_LIBS.7z
Expand-7Zip -ArchiveFileName WX_LIBS.7z -TargetPath ($dependency_folder + '\\wxWidgets')

Invoke-WebRequest -Uri $windows_ffmpeg_url -OutFile FFMPEG.zip
Expand-Archive FFMPEG.zip -DestinationPath $dependency_folder

# Generate project files
$windows_openal_dir = ('.\\src_dependency\\openal-soft-' + $openal_ver + '-bin')
$windows_sdl2_dir = ('.\\src_dependency\\SDL2-' + $sdl2_ver)
$windows_ffmpeg_dir = ('.\\src_dependency\\ffmpeg-' + $ffmpeg_ver)

Copy-Item -Path ($windows_openal_dir + '\\*') -Destination ($dependency_folder + '\\openal-soft') -Recurse -Force
Copy-Item -Path ($windows_sdl2_dir + '\\*') -Destination ($dependency_folder + '\\SDL2') -Recurse -Force
Copy-Item -Path ($windows_ffmpeg_dir + '\\*') -Destination ($dependency_folder + '\\ffmpeg') -Recurse -Force

Remove-Item $windows_openal_dir -Recurse
Remove-Item $windows_sdl2_dir -Recurse
Remove-Item $windows_ffmpeg_dir -Recurse

Remove-Item SDL2.zip
Remove-Item OPENAL.zip
Remove-Item WX_HDRS.7z
Remove-Item WX_LIBS.7z
Remove-Item FFMPEG.zip

Set-Location -Path $project_folder

# & .\\premake5 vs2022

# Open solution
# & .\\build\\DriverSyndicate.sln