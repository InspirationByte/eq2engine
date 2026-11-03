
$openal_ver = '1.23.1'
$sdl2_ver = '2.30.2'

$slang_ver = '2025.19.1'

$windows_openal_url = 'https://github.com/kcat/openal-soft/releases/download/' + $openal_ver + '/openal-soft-' + $openal_ver + '-bin.zip'
$windows_sdl2_url = 'https://github.com/libsdl-org/SDL/releases/download/release-' + $sdl2_ver + '/SDL2-devel-' + $sdl2_ver + '-VC.zip'
$windows_slang_url = 'https://github.com/shader-slang/slang/releases/download/v' +$slang_ver+ '/slang-' +$slang_ver+ '-windows-x86_64.zip'

$project_folder = '.\\'
$dependency_folder = $project_folder + '\\src_dependency'

# Download required dependencies
Invoke-WebRequest -Uri $windows_sdl2_url -OutFile SDL2.zip
Invoke-WebRequest -Uri $windows_openal_url -OutFile OPENAL.zip
Invoke-WebRequest -Uri $windows_slang_url -OutFile SLANG.zip

Expand-Archive SDL2.zip -DestinationPath $dependency_folder
Expand-Archive OPENAL.zip -DestinationPath $dependency_folder
Expand-Archive SLANG.zip -DestinationPath ($dependency_folder + '\\slang')

# Generate project files
$windows_openal_dir = ('.\\src_dependency\\openal-soft-' + $openal_ver + '-bin')
$windows_sdl2_dir = ('.\\src_dependency\\SDL2-' + $sdl2_ver)

Copy-Item -Path ($windows_openal_dir + '\\*') -Destination ($dependency_folder + '\\openal-soft') -Recurse -Force
Copy-Item -Path ($windows_sdl2_dir + '\\*') -Destination ($dependency_folder + '\\SDL2') -Recurse -Force

Remove-Item $windows_openal_dir -Recurse
Remove-Item $windows_sdl2_dir -Recurse

Remove-Item SDL2.zip
Remove-Item OPENAL.zip
Remove-Item SLANG.zip

Set-Location -Path $project_folder

# & .\\premake5 vs2022

# Open solution
# & .\\build\\DriverSyndicate.sln
