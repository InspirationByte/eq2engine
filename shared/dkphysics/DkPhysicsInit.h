#pragma once

namespace JPH
{
class JobSystem;
class TempAllocator;
}

JPH::JobSystem* GetJoltJobSystem();
JPH::TempAllocator* GetJoltTempAlloc();

void dkPhysicsLibInit();
void dkPhysicsLibShutdown();
