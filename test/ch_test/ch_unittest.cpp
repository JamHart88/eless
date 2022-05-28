// Step 1. Include necessary header files such that the stuff your
// test logic needs is declared.
//
// Don't forget gtest.h, which declares the testing framework.


#include "less.hpp"

#include "ch_mocks.hpp"
#include "gmock/gmock.h" // Brings in gMock.
#include "gtest/gtest.h"

#include "ch.cpp"

using namespace ::testing;

// helper to copy a filename string
char* initfilename(const char* fname)
{
    int len = strlen(fname);
    char* filename = (char*)calloc(len + 1, sizeof(char));
    memcpy(filename, fname, len);
    return filename;
}


struct Mocks
    : public ::testing::NiceMock<ChMock> {
};

template <typename T>
struct ModuleTest : public ::testing::Test {
protected:
    ModuleTest()
    {
        moduleTestMocks.reset(new T);
    }
    virtual ~ModuleTest()
    {
        moduleTestMocks.reset();
    }
    void SetUp()
    {
    }
    void TearDown()
    {
    }
};

struct TestFixture : public ModuleTest<Mocks> {
};

// --------------------------------------------------------------
class ChUnitTestEmpty : public TestFixture {
public:
    ChUnitTestEmpty()
    {
      int autobuf = 0;
      logfile = -1;
      follow_mode = 0;       /* F cmd Follows file desc or file name? */
      curr_ino = 0;
      curr_dev = 0;
      screen_trashed = TRASHED;
      sigs = 0;
      namelogfile = NULL;
    }

    void TearDown()
    {
    }

    filestate *capturedFilestate = nullptr;

    void captureFilestate(void * obj){
      capturedFilestate = (filestate *)obj;
    }

};
// --------------------------------------------------------------

TEST_F(ChUnitTestEmpty, chflags)
{
  int res = ch_getflags();
  EXPECT_EQ(res, 0);
}

TEST_F(ChUnitTestEmpty, ch_init1)
{
  const int test_file_id = 10;
  const int test_flags = 0;

  using namespace ifile;
  Ifile * lclIfile = new Ifile(nullptr);

  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(2).
    WillOnce(Return(lclIfile)).
    WillOnce(Return(lclIfile));
  
  EXPECT_CALL(*lclIfile, getFilestate()).Times(1).
    WillOnce(Return(nullptr));
  
  // This will capture the filestate object being saved
  EXPECT_CALL(*lclIfile, setFilestate(_)).
    Times(1).
    WillOnce(DoAll(Invoke(this, &ChUnitTestEmpty::captureFilestate), Return()));

  ch_init(test_file_id, 
          test_flags);

  // TODO: Check buffers

  EXPECT_EQ(capturedFilestate->file,  test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags);

  delete lclIfile;
}


TEST_F(ChUnitTestEmpty, ch_init2)
{

  const int test_file_id = 10;
  const int test_flags = CH_CANSEEK | CH_HELPFILE;

  using namespace ifile;
  Ifile * lclIfile = new Ifile(nullptr);

  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(2).
    WillOnce(Return(lclIfile)).
    WillOnce(Return(lclIfile));
  
  EXPECT_CALL(*lclIfile, getFilestate()).Times(1).
    WillOnce(Return(nullptr));
  
  EXPECT_CALL(GetMock<ChMock>(), lseek(test_file_id, _, _)).
    Times(1).
    WillOnce(Return(BAD_LSEEK)); // is Seekable return - false
    
  // This will capture the filestate object being saved
  EXPECT_CALL(*lclIfile, setFilestate(_)).
    Times(1).
    WillOnce(DoAll(Invoke(this, &ChUnitTestEmpty::captureFilestate), Return()));
  
  ch_init(test_file_id,
          test_flags); 

  EXPECT_EQ(capturedFilestate->file, test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags & ~CH_CANSEEK);

  delete lclIfile;
}


TEST_F(ChUnitTestEmpty, ch_init3)
{

  const int test_file_id = 10;
  const int test_flags = CH_KEEPOPEN;

  using namespace ifile;
  Ifile * lclIfile = new Ifile(nullptr);

  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(2).
    WillOnce(Return(lclIfile)).
    WillOnce(Return(lclIfile));
  
  EXPECT_CALL(*lclIfile, getFilestate()).Times(1).
    WillOnce(Return(nullptr));
  
  // This will capture the filestate object being saved
  EXPECT_CALL(*lclIfile, setFilestate(_)).
    Times(1).
    WillOnce(DoAll(Invoke(this, &ChUnitTestEmpty::captureFilestate), Return()));

  ch_init(test_file_id, CH_KEEPOPEN); 

  EXPECT_EQ(capturedFilestate->file, test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags);

  delete lclIfile;
}

// --------------------------------------------------------------
class ChUnitTest1Buff : public TestFixture {
public:
    ifile::Ifile* lclIfile = nullptr;
    filestate* capturedFilestate = nullptr;

    void captureFilestate(void* obj)
    {
        capturedFilestate = (filestate*)obj;
    }

    const int default_file_id = 1;
    const int default_flags = 0;

    ChUnitTest1Buff()
    {
        log("Fixture constructor", false);
        int autobuf = 0;
        logfile = -1;
        follow_mode = 0;
        curr_ino = 0;
        curr_dev = 0;
        screen_trashed = TRASHED;
        sigs = 0;
        namelogfile = NULL;
        lclIfile = new ifile::Ifile(nullptr);
    }

    void SetUp()
    {

        log("Setup start", false);

        EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).Times(2).WillOnce(Return(lclIfile)).WillOnce(Return(lclIfile));
        EXPECT_CALL(*lclIfile, getFilestate()).Times(1).WillOnce(Return(nullptr));
        EXPECT_CALL(*lclIfile, setFilestate(_)).Times(1).WillOnce(DoAll(Invoke(this, &ChUnitTest1Buff::captureFilestate), Return()));

        ch_init(default_file_id, // f
            default_flags); // flags

        log("Setup end", false);
    }

    void TearDown()
    {
        log("TearDown", false);
        delete lclIfile;
    }
};
// --------------------------------------------------------------

TEST_F(ChUnitTest1Buff, ch_init4)
{
  using namespace ifile;

  const int test_file_id    = 9;
  const int test_flags      = CH_NODATA | CH_CANSEEK;
  const int test_file_size  = 9999;

  capturedFilestate->file  = -1; // Set to null - will change in test
  capturedFilestate->flags = test_flags;
  capturedFilestate->fsize = test_file_size;

  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(1).
    WillOnce(Return(lclIfile)); 

  EXPECT_CALL(*lclIfile, getFilestate()).Times(1).
    WillOnce(Return((void*)capturedFilestate));
  
  EXPECT_CALL(GetMock<ChMock>(), lseek(test_file_id, _, _)).
    Times(1).
    WillOnce(Return(1));

  EXPECT_CALL(GetMock<ChMock>(), filesize(9)).
    Times(1).
    WillOnce(Return(test_file_size));

  ch_init(test_file_id,   // f
          test_flags);    // flags

  EXPECT_EQ(capturedFilestate->file, test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags); 
  EXPECT_EQ(capturedFilestate->fsize, test_file_size);
}


// =================================================================
// Google Test can be run manually from the main() function
// or, it can be linked to the gtest_main library for an already
// set-up main() function primed to accept Google Test test cases.
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // use this to run a filter of tests in the file
    //::testing::GTEST_FLAG(filter) = "ChUnitTest1Buff*";

    return RUN_ALL_TESTS();
}
