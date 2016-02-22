# see http://luajit.org/install.html for details
# there, a call like one of the following is recommended

if [ "x$NDK" == "x" ]; then
	NDK=/opt/android-ndk
fi
if [ ! -d "$NDK" ]; then
	echo 'NDK not found. Please set NDK environment variable and have it point to the NDK dir.'
	exit 1
fi

TARGET_ARCH=$1
TARGET_ABI=$2
LUAJIT_PATH=$3
INSTALL_DIR=$4

if [ ! -d "$LUAJIT_PATH" ]; then
	echo 'LuaJIT path not set. Example for this script: mk-luajit "/path/to/luajit-2.0" target-arch abi-number /install/dir'
	exit 1
fi

BUILD_ARCH=linux-$(uname -m)

if [[ ("$BUILD_ARCH" == "linux-i686") || ("$BUILD_ARCH" == "linux-i586") ]]; then
	BUILD_ARCH="linux-x86"
fi

DEST=$INSTALL_DIR/$TARGET_ARCH

case "$TARGET_ARCH" in
clean)
	make -C $LUAJIT_PATH clean
	;;
armeabi)
	# Android/ARM, armeabi (ARMv5TE soft-float), Android 2.2+ (Froyo)
	NDKABI=$TARGET_ABI
	NDKVER=$NDK/toolchains/arm-linux-androideabi-4.8
	NDKP=$NDKVER/prebuilt/$BUILD_ARCH/bin/arm-linux-androideabi-
	NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-arm"
	NDKARCH="-march=armv5te -mfloat-abi=softfp"
	rm -rf "$DEST"
	make -C $LUAJIT_PATH install HOST_CC="gcc -m32" CROSS=$NDKP TARGET_FLAGS="$NDKF $NDKARCH" DESTDIR="$DEST" PREFIX=
	;;
armeabi-v7a)
	# Android/ARM, armeabi-v7a (ARMv7 VFP), Android 4.0+ (ICS)
	NDKABI=$TARGET_ABI
	NDKVER=$NDK/toolchains/arm-linux-androideabi-4.8
	NDKP=$NDKVER/prebuilt/$BUILD_ARCH/bin/arm-linux-androideabi-
	NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-arm"
	NDKARCH="-march=armv7-a -mfloat-abi=softfp -Wl,--fix-cortex-a8"
	make -C $LUAJIT_PATH install HOST_CC="gcc -m32" CROSS=$NDKP TARGET_FLAGS="$NDKF $NDKARCH" DESTDIR="$DEST" PREFIX=
	;;
mips)
	# Android/MIPS, mips (MIPS32R1 hard-float), Android 4.0+ (ICS)
	NDKABI=$TARGET_ABI
	NDKVER=$NDK/toolchains/mipsel-linux-android-4.8
	NDKP=$NDKVER/prebuilt/$BUILD_ARCH/bin/mipsel-linux-android-
	NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-mips"
	make -C $LUAJIT_PATH install HOST_CC="gcc -m32" CROSS=$NDKP TARGET_FLAGS="$NDKF" DESTDIR="$DEST" PREFIX=
	;;
x86)
	# Android/x86, x86 (i686 SSE3), Android 4.0+ (ICS)
	NDKABI=$TARGET_ABI
	NDKVER=$NDK/toolchains/x86-4.6
	NDKP=$NDKVER/prebuilt/$BUILD_ARCH/bin/i686-linux-android-
	NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-x86"
	make -C $LUAJIT_PATH install HOST_CC="gcc -m32" CROSS=$NDKP TARGET_FLAGS="$NDKF" DESTDIR="$DEST" PREFIX=
	;;
*)
	echo 'specify one of "armeabi", "armeabi-v7a", "mips", "x86" or "clean" as first argument'
	exit 1
	;;
esac
