#!/bin/bash
###########################################################
# Builds parts of Equilibrium Engine 
# and Driver Syndicate itself.
#
# For Linux.
#
###########################################################

make -f coreLib.makefile
make -f prevLib.makefile

cd Core/
make -f eqCore.makefile
cd ../

cd MaterialSystem/
make -f eqMatSystem.makefile
cd ../

cd MaterialSystem/EngineShaders/
make -f eqBaseShaders.makefile
cd ../../

cd MaterialSystem/Renderers/
make -f eqNullRHI.makefile
make -f eqGLRHI.makefile
cd ../../

cd DriversGame/
make -f Game.makefile

echo "Completed operation!"
