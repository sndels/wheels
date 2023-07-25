# wheels

Let's reinvent some!

This is kind of a stdlib-thingy where I try to replace things from the STL to learn how things work. It implements dynamic containers with abstract allocators to get a feel for an architecture that requires passing allocators around and not relying on magical global ones

Allocators include a general purpose TlsfAllocator, a LinearAllocator and a 'ScopedScratch' for temporary allocations.

The containers mirror their STL counterparts with missing and/or simplified interfaces where I didn't find a need to complicate things.

Used in [prosper](https://github.com/sndels/prosper), my Vulkan toy renderer, to get more insight through "real world" user code.
