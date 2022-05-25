#include "gmock/gmock.h"
#include "mocks.hpp"


// This is the single tone that is filled in test fixture constructor
std::unique_ptr<ModuleMockBase> moduleTestMocks;

ModuleMockBase::~ModuleMockBase()
{
}