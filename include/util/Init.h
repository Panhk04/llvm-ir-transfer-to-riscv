//
// Created by XuRuiyuan on 2023/8/1.
//

#ifndef FASTER_MEOW_INIT_H
#define FASTER_MEOW_INIT_H
#include "Type.h"

//frontend
BasicType *BasicType::I32_TYPE = new BasicType(DataType::I32);
BasicType *BasicType::I1_TYPE = new BasicType(DataType::I1);
BasicType *BasicType::F32_TYPE = new BasicType(DataType::F32);
int BasicBlock::bb_count = 0;
int BasicBlock::empty_bb_count = 0;

#endif //FASTER_MEOW_INIT_H
