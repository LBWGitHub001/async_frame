//
// Created by lbw on 25-2-20.
//

#ifndef MEMBLOCK_H
#define MEMBLOCK_H

#include <memory>


class MemBlockBase {
    MemBlockBase() = default;
    virtual ~MemBlockBase() = default;

    virtual void free() = 0;
    virtual void* malloc(size_t size) = 0;




};



#endif //MEMBLOCK_H
