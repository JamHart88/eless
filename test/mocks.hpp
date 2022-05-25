#ifndef MOCKS_H
#define MOCKS_H

#include <gtest/gtest.h>
#include "gmock/gmock.h"


struct ModuleMockBase
{
    virtual ~ModuleMockBase();
};

// 1. The ModuleMockBase inheritance must be virtual to allow storing mocks in the singleton.
// 2. The ReturnAfterHookSwitch inheritance must be non-virtual to allow different switch for every mock.
struct ModuleMock
    : public virtual ModuleMockBase
{
};

extern std::unique_ptr<ModuleMockBase> moduleTestMocks;


template<typename T>
static T& GetMock()
{
    auto ptr = dynamic_cast<T*>(moduleTestMocks.get());
    if (ptr == nullptr)
    {
        auto err = "The test does not provide mock of \"" + std::string(typeid(T).name()) + "\"";
        throw std::runtime_error(err.c_str());
    }
    return *ptr;
}


#endif
