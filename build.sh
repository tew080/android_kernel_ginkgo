#!/bin/bash
#
# Compile script for ERROR kernel
# Copyright (C) 2020-2021 Adithya R.

SECONDS=0 # builtin bash timer
ZIPNAME="ERROR-Q1-ginkgo-KSU-Next+SuSFS-$(date '+%Y%m%d-%H%M').zip"
TC_DIR="/home/tew/kernel/clang-r547379"
GCC_64_DIR="/home/tew/kernel/aarch64-linux-android-4.9"
GCC_32_DIR="/home/tew/kernel/arm-linux-androideabi-4.9"
DEFCONFIG="vendor/ginkgo-perf_defconfig"

export PATH="${TC_DIR}/bin:${GCC_64_DIR}/bin:${GCC_32_DIR}/bin:/usr/bin:${PATH}"

if [[ $1 = "-r" || $1 = "--regen" ]]; then
make O=out ARCH=arm64 $DEFCONFIG savedefconfig
cp out/defconfig arch/arm64/configs/$DEFCONFIG
exit
fi

if [[ $1 = "-c" || $1 = "--clean" ]]; then
make clean
make mrproper
rm -rf out
rm -rf *.zip
fi

MAKE_PARAMS="O=out \
   ARCH=arm64 \
   LD_LIBRARY_PATH="${TC_DIR}/lib:${LD_LIBRARY_PATH}" \
   CC=clang \
   LD=ld.lld \
   AR=llvm-ar \
   AS=llvm-as \
   NM=llvm-nm \
   OBJCOPY=llvm-objcopy \
   OBJDUMP=llvm-objdump \
   STRIP=llvm-strip \
   CROSS_COMPILE=$GCC_64_DIR/bin/aarch64-linux-android- \
   CROSS_COMPILE_ARM32=$GCC_32_DIR/bin/arm-linux-androideabi- \
   CLANG_TRIPLE=aarch64-linux-gnu- \
   Image.gz-dtb dtbo.img"

mkdir -p out
make O=out ARCH=arm64 $DEFCONFIG

echo -e "\nStarting compilation...\n"
make -j$(nproc --all) $MAKE_PARAMS

kernel="out/arch/arm64/boot/Image.gz-dtb"
dtbo="out/arch/arm64/boot/dtbo.img"

if [ ! -f "$kernel" ] || [ ! -f "$dtbo" ]; then
	echo -e "\nCompilation failed!"
	exit 1
fi

rm -rf AnyKernel3/Image.gz-dtb
rm -rf AnyKernel3/dtbo.img

cp $kernel AnyKernel3
cp $dtbo AnyKernel3

cd AnyKernel3
zip -r9 "../$ZIPNAME" * -x .git README.md *placeholder
cd ..
echo -e "\nCompleted in $((SECONDS / 60)) minute(s) and $((SECONDS % 60)) second(s) !"
echo "Zip: $ZIPNAME"
