#ifndef IFILE_MOCKS_H
#define IFILE_MOCKS_H

#include "../mocks.hpp"
#include "ifile.hpp"
#include "utils.hpp"

#include <gtest/gtest.h>
#include "gmock/gmock.h"

class UtilsMock : public ModuleMock
{
public:
    MOCK_METHOD1(save, char*(const char*));
    MOCK_METHOD1(lrealpath, char *(const char *));
    MOCK_METHOD1(unmark, void (ifile::Ifile *));
    MOCK_METHOD1(mark_check_ifile, void (ifile::Ifile *));
};

#endif

