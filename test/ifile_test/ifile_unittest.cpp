// Step 1. Include necessary header files such that the stuff your
// test logic needs is declared.
//
// Don't forget gtest.h, which declares the testing framework.

#include "ifile.cpp"
#include "less.hpp"

#include "ifile_mocks.hpp"
#include "gmock/gmock.h" // Brings in gMock.
#include "gtest/gtest.h"

using namespace ::testing;

namespace {

// this is used in utils
// int is_tty = true;

using namespace ifile;

struct Mocks
    : public ::testing::NiceMock<UtilsMock> {
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
        // std::cout << "----ModuleTest Setup()\n";
    }
    void TearDown()
    {
        // std::cout << "----ModuleTest TearDown()\n";
    }
};

char* initfilename(const char* fname)
{
    int len = strlen(fname);
    char* filename = (char*)calloc(len + 1, sizeof(char));
    memcpy(filename, fname, len);
    return filename;
}

struct TestFixture : public ModuleTest<Mocks> {
};

// --------------------------------------------------------------
class IfileUnitTestEmpty : public TestFixture {
public:
    IfileUnitTestEmpty()
    {
      previousIfile = nullptr;
      currentIfile = nullptr;
    }

    void TearDown()
    {
      // Remove all ifile objects
      Ifile * currIfile = getCurrentIfile();
      while (! (currIfile == nullptr)) {
        EXPECT_CALL(GetMock<UtilsMock>(), unmark(currIfile)).Times(1);
        deleteIfile(currIfile);
        currIfile = getCurrentIfile();
      }
    }
};
// --------------------------------------------------------------

TEST_F(IfileUnitTestEmpty, CreateAndDelete)
{
  //std::cout << "Start create and delete\n";

  char const * const fname = "test1.txt";

  char* filename1 = initfilename(fname);
  char* filename2 = initfilename(fname);

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).
    WillOnce(Return(filename1));
  
  EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
    Times(1).
    WillOnce(Return(filename2));

  EXPECT_CALL(GetMock<UtilsMock>(), mark_check_ifile(_));

  Ifile* newIfile = getIfile("test1.txt");

  //std::cout << "Myifile " << newIfile << " -> filename is : " << newIfile->getFilename() << "\n";

  EXPECT_NE(newIfile, nullptr);
  EXPECT_STREQ(newIfile->getFilename(), fname);
  EXPECT_EQ(newIfile->getOpened(), false);
  EXPECT_EQ(newIfile->getHoldCount(), 0);
  EXPECT_EQ(numIfiles(), 1);

  EXPECT_EQ(find(newIfile), 1); // index starts from 1

  // Did current ifile get set correctly?
  //Ifile * currentIfile = getCurrentIfile();
  EXPECT_EQ(currentIfile, newIfile);

  // What about oldIfile
  Ifile * oldIfile = getOldIfile();
  EXPECT_EQ(oldIfile, nullptr);

  // No need to free the filenames - done by ifile

  // Set the file state - to test that its freed as part of delete
  char * dummyFilestate = initfilename("DUMMTFILESTATE");
  newIfile->setFilestate(dummyFilestate);
  EXPECT_STREQ((char *)newIfile->getFilestate(), (char *)"DUMMTFILESTATE");

  EXPECT_CALL(GetMock<UtilsMock>(), unmark(newIfile)).Times(1);
  deleteIfile(newIfile);

  EXPECT_EQ(numIfiles(), 0);

  // Did current ifile get set correctly?
  EXPECT_EQ(getCurrentIfile(), nullptr);
  EXPECT_EQ(getOldIfile(), nullptr);
  EXPECT_EQ(numIfiles(), 0);

  //std::cout << "End\n";
}

TEST_F(IfileUnitTestEmpty, nextIfile_emptylist)
{
  Ifile * ret = nextIfile(nullptr);
  EXPECT_EQ(ret, nullptr);
}

TEST_F(IfileUnitTestEmpty, prevIfile_emptyList)
{
  Ifile * ret = prevIfile(nullptr);
  EXPECT_EQ(ret, nullptr);
}

// --------------------------------------------------------------
class IfileUnitTest : public TestFixture {
public:
    Ifile* savedTestIfilePtr = nullptr;
    const char* default_fname = "ifile1.txt";

    IfileUnitTest()
    {
    }

    void SetUp()
    {
        previousIfile = nullptr;
        currentIfile = nullptr;

        //std::cout << "---IfileUnitTest SetUp()\n";
        char* filename1 = initfilename(default_fname);
        char* filename2 = initfilename(default_fname);

        EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
          Times(1).
          WillOnce(Return(filename1));

        EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
          Times(1).
          WillOnce(Return(filename2));

        savedTestIfilePtr = getIfile(default_fname);

        //std::cout << "---IfileUnitTest SetUp() DONE\n";
    }

    void TearDown()
    {
        //std::cout << "----IfileUnitTest TearDown()\n";

        // delete any ifile objects that exist

        Ifile * currIfile = getCurrentIfile();
        while (! (currIfile == nullptr)) {
          EXPECT_CALL(GetMock<UtilsMock>(), unmark(currIfile)).Times(1);
          //std::cout << "TearDown - deleting " << currIfile << "\n";
          deleteIfile(currIfile);
          currIfile = getCurrentIfile();
        }

    }
};
// --------------------------------------------------------------

TEST_F(IfileUnitTest, FindIfile_part1)
{
    char* realsearchfilename = initfilename("t1.txt");

    // check valid at start
    EXPECT_EQ(numIfiles(), 1);

    EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
      Times(1).
      WillOnce(Return(realsearchfilename));

    Ifile* newIfile = findIfile(default_fname);

    // name should be unchanged
    EXPECT_NE(newIfile, nullptr);
    EXPECT_EQ(newIfile, savedTestIfilePtr);
    EXPECT_STREQ(newIfile->getFilename(), default_fname);
    EXPECT_EQ(numIfiles(), 1);
}

TEST_F(IfileUnitTest, DeleteIfile_nullptr)
{
    EXPECT_EQ(numIfiles(), 1);
    deleteIfile(nullptr);
    EXPECT_EQ(numIfiles(), 1);
}

TEST_F(IfileUnitTest, getCurrentIfile)
{
    Ifile * myIfile = getCurrentIfile();
    EXPECT_EQ(myIfile, savedTestIfilePtr);
    EXPECT_EQ(currentIfile, myIfile);
}

TEST_F(IfileUnitTest, setCurrentIfile_nullptr)
{
    setCurrentIfile(nullptr);
    EXPECT_EQ(getCurrentIfile(), nullptr);
    EXPECT_EQ(currentIfile, nullptr);

    // reset ifile so that TearDown works
    setCurrentIfile(savedTestIfilePtr);
}

TEST_F(IfileUnitTest, setCurrentIfile_saved)
{
    setCurrentIfile(savedTestIfilePtr);
    EXPECT_EQ(getCurrentIfile(), savedTestIfilePtr);
    EXPECT_EQ(currentIfile, savedTestIfilePtr);
}

TEST_F(IfileUnitTest, getOldIfile)
{
    Ifile * myIfile = getOldIfile();
    EXPECT_EQ(myIfile, nullptr);
    EXPECT_EQ(myIfile, previousIfile);
}

TEST_F(IfileUnitTest, setOldIfile_nullptr)
{
    setOldIfile(nullptr);
    EXPECT_EQ(getOldIfile(), nullptr);
    EXPECT_EQ(previousIfile, nullptr);
}

TEST_F(IfileUnitTest, setOldIfile_saved)
{
    setOldIfile(savedTestIfilePtr);
    Ifile * myIfile = getOldIfile();
    EXPECT_EQ(getOldIfile(), savedTestIfilePtr);
    EXPECT_EQ(previousIfile, getOldIfile());
}

TEST_F(IfileUnitTest, IfileCreate_add3MoreIfiles)
{
  //std::cout << "-------------------------------------\n";
  // 1 file already exists
  // 2nd file
  char const * fname2 = (char *)"test2.txt";

  char* file_1 = initfilename(fname2);
  char* file_2 = initfilename(fname2);

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).
    WillOnce(Return(file_1));

  EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
    Times(1).
    WillOnce(Return(file_2));

  Ifile* newIfile = getIfile(fname2);
  
  EXPECT_NE(newIfile, nullptr);
  EXPECT_STREQ(newIfile->getFilename(), fname2);
  EXPECT_EQ(numIfiles(), 2);
  EXPECT_EQ(find(newIfile), 2); // index starts from 1

  // Did current ifile get set correctly?
  Ifile * lclcurrentIfile = getCurrentIfile();
  EXPECT_EQ(lclcurrentIfile, newIfile);

  //std::cout << "-------------------------------------\n";
  // 3rd file
  char const * fname3 = (char *)"test3.txt";

  file_1 = initfilename(fname3);
  file_2 = initfilename(fname3);

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).
    WillOnce(Return(file_1));
    
  EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
    Times(1).
    WillOnce(Return(file_2));

  newIfile = getIfile(fname3);
  
  EXPECT_NE(newIfile, nullptr);
  EXPECT_STREQ(newIfile->getFilename(), fname3);
  EXPECT_EQ(numIfiles(), 3);
  EXPECT_EQ(find(newIfile), 3); // index starts from 1

  // Did current ifile get set correctly?
  lclcurrentIfile = getCurrentIfile();
  EXPECT_EQ(lclcurrentIfile, newIfile);



  //std::cout << "-------------------------------------\n";
  // 4th file
  char const * fname4 = (char *)"test4.txt";

  file_1 = initfilename(fname4);
  file_2 = initfilename(fname4);

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).
    WillOnce(Return(file_1));

  EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
    Times(1).
    WillOnce(Return(file_2));

  newIfile = getIfile(fname4);
  
  EXPECT_NE(newIfile, nullptr);
  EXPECT_STREQ(newIfile->getFilename(), fname4);
  EXPECT_EQ(numIfiles(), 4);
  EXPECT_EQ(find(newIfile), 4); // index starts from 1

  // Did current ifile get set correctly?
  lclcurrentIfile = getCurrentIfile();
  EXPECT_EQ(lclcurrentIfile, newIfile);
}

TEST_F(IfileUnitTest, CopyConstructor)
{

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).
    WillOnce(Return(initfilename(default_fname)));

  Ifile copiedIfile = *savedTestIfilePtr;
  EXPECT_STREQ(copiedIfile.getFilename(), savedTestIfilePtr->getFilename());

}

TEST_F(IfileUnitTest, OpEqualsConstructor)
{
  char const * const fname = "OpEqualsConstructorFile.txt";

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).
    WillOnce(Return(initfilename(fname)));
  
  //EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
  //  Times(1).
  //  WillOnce(Return(initfilename(fname)));

  Ifile copiedIfile(fname);

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).
    WillOnce(Return(initfilename(default_fname)));

  copiedIfile = *savedTestIfilePtr;

  EXPECT_STREQ(copiedIfile.getFilename(), savedTestIfilePtr->getFilename());

  EXPECT_EQ(*savedTestIfilePtr == copiedIfile, true);
  
}

TEST_F(IfileUnitTest, getPos_setPos) 
{

  scrpos res = savedTestIfilePtr->getPos();
  EXPECT_EQ(res.pos, -1);
  EXPECT_EQ(res.ln,  0);
  
  scrpos sp = {99, 21};

  savedTestIfilePtr->setPos(sp);

  res = savedTestIfilePtr->getPos();
  EXPECT_EQ(res.pos, 99);
  EXPECT_EQ(res.ln,  21);
}

TEST_F(IfileUnitTest, getOpened_setOpened) 
{
  EXPECT_EQ(savedTestIfilePtr->getOpened(), false); // default

  savedTestIfilePtr->setOpened(true);
 
  EXPECT_EQ(savedTestIfilePtr->getOpened(), true);

}

TEST_F(IfileUnitTest, getHold_setHold) 
{
  EXPECT_EQ(savedTestIfilePtr->getHoldCount(), 0); // default

  savedTestIfilePtr->setHold(2);
   
  EXPECT_EQ(savedTestIfilePtr->getHoldCount(), 2); 

}

TEST_F(IfileUnitTest, getAltPipe_setAltPipe) 
{
  char * altpipe = initfilename("ALTPIPE");

  EXPECT_EQ(savedTestIfilePtr->getAltpipe(), nullptr); // default

  savedTestIfilePtr->setAltpipe(altpipe);
   
  EXPECT_EQ(savedTestIfilePtr->getAltpipe(), altpipe); 

  free(altpipe);
}

TEST_F(IfileUnitTest, getAltfilename_setAltfilename) 
{
  char * altfilename = initfilename("ALTFILENAME");

  EXPECT_EQ(savedTestIfilePtr->getAltfilename(), nullptr); // default

  savedTestIfilePtr->setAltfilename(altfilename);
   
  EXPECT_EQ(savedTestIfilePtr->getAltfilename(), altfilename); 

  char * altfilename2 = initfilename("ALTFILENAME2");

  savedTestIfilePtr->setAltfilename(altfilename2);

  EXPECT_EQ(savedTestIfilePtr->getAltfilename(), altfilename2); 

  //free(altfilename2);
}

// --------------------------------------------------------------
class IfileUnitTest4Files : public TestFixture {
public:
    Ifile* file1Ptr = nullptr;
    Ifile* file2Ptr = nullptr;
    Ifile* file3Ptr = nullptr;
    Ifile* file4Ptr = nullptr;
    
    const char* f1 = "f1.txt";
    const char* f2 = "f2.txt";
    const char* f3 = "f3.txt";
    const char* f4 = "f4.txt";

    IfileUnitTest4Files()
    {
    }

    void SetUp()
    {
        //std::cout << "--- SetUp Start\n";

        previousIfile = nullptr;
        currentIfile = nullptr;

        //std::cout << "---IfileUnitTest4Files SetUp() === 1 ==== \n";

        EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
          Times(1).
          WillOnce(Return(initfilename(f1)));

        EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
           Times(1).
           WillOnce(Return(initfilename(f1)));

        file1Ptr = getIfile(f1);

        // --------------------------------------------------
        //std::cout << "---IfileUnitTest4Files SetUp() === 2 ==== \n";
        // 2nd file

        EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
          Times(1).
          WillOnce(Return(initfilename(f2)));

        EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
          Times(1).
          WillOnce(Return(initfilename(f2)));

        file2Ptr = getIfile(f2);
        
        EXPECT_STREQ(file2Ptr->getFilename(), f2);

        // --------------------------------------------------
        //std::cout << "---IfileUnitTest4Files SetUp() === 3 ==== \n";
        
        // 3rd file

        EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
          Times(1).
          WillOnce(Return(initfilename(f3)));

        EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
          Times(1).
          WillOnce(Return(initfilename(f3)));

        file3Ptr = getIfile(f3);
        
        EXPECT_STREQ(file3Ptr->getFilename(), f3);

        // --------------------------------------------------        
        //std::cout << "---IfileUnitTest4Files SetUp() === 4 ==== \n";

        // 4th file
        
        EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
          Times(1).          
          WillOnce(Return(initfilename(f4)));

        EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
          Times(1).
          WillOnce(Return(initfilename(f4)));

        file4Ptr = getIfile(f4);
        
        EXPECT_STREQ(file4Ptr->getFilename(), f4);

        //std::cout << "---IfileUnitTest4Files SetUp() === END ==== \n";
        //std::cout << "--- SetUp End\n";
    }

    void TearDown()
    {
        //std::cout << "--- TearDown start\n";
        Ifile * currIfile = fileList.front();
        while (! (currIfile == nullptr)) {
          EXPECT_CALL(GetMock<UtilsMock>(), unmark(currIfile)).Times(1);
          deleteIfile(currIfile);
          currIfile = nullptr;
          if (fileList.size() > 0)
            currIfile = fileList.front();
          
        }
        //std::cout << "--- TearDown end\n";

    }
};
// --------------------------------------------------------------

TEST_F(IfileUnitTest4Files, getIndexTests)
{
    //std::cout << "getIndexTests Start \n";
    EXPECT_EQ(getIndex(file1Ptr),1);
    EXPECT_EQ(getIndex(file2Ptr),2);
    EXPECT_EQ(getIndex(file3Ptr),3);
    EXPECT_EQ(getIndex(file4Ptr),4);
    EXPECT_EQ(getIndex(nullptr), -1);
}

TEST_F(IfileUnitTest4Files, find_nullptr)
{
  int index = find(nullptr);
  EXPECT_EQ(index, -1);

}

TEST_F(IfileUnitTest4Files, delete_first)
{   
    setCurrentIfile(file1Ptr);

    EXPECT_CALL(GetMock<UtilsMock>(), unmark(file1Ptr)).
      Times(1);
    
    deleteIfile(file1Ptr);
    
    EXPECT_EQ(numIfiles(), 3);
    
    // Did current ifile get set correctly?
    EXPECT_EQ(getCurrentIfile(), file2Ptr);
    EXPECT_EQ(getOldIfile(), nullptr);

    EXPECT_EQ(getIndex(file1Ptr), -1);
    EXPECT_EQ(getIndex(file2Ptr), 1);
    EXPECT_EQ(getIndex(file3Ptr), 2);
    EXPECT_EQ(getIndex(file4Ptr), 3);
}

TEST_F(IfileUnitTest4Files, delete_last)
{
    setCurrentIfile(file4Ptr);

    EXPECT_CALL(GetMock<UtilsMock>(), unmark(file4Ptr)).Times(1);
    
    deleteIfile(file4Ptr);  

    EXPECT_EQ(numIfiles(), 3);
    // Did current ifile get set correctly?
    EXPECT_EQ(getCurrentIfile(), file3Ptr);
    EXPECT_EQ(getOldIfile(), nullptr);

    EXPECT_EQ(getIndex(file1Ptr), 1);
    EXPECT_EQ(getIndex(file2Ptr), 2);
    EXPECT_EQ(getIndex(file3Ptr), 3);
    EXPECT_EQ(getIndex(file4Ptr), -1);
}

TEST_F(IfileUnitTest4Files, delete_middle)
{
    setCurrentIfile(file2Ptr);

    EXPECT_CALL(GetMock<UtilsMock>(), unmark(file2Ptr)).Times(1);
    
    deleteIfile(file2Ptr);  

    EXPECT_EQ(numIfiles(), 3);
    // Did current ifile get set correctly?
    EXPECT_EQ(getCurrentIfile(), file1Ptr);
    EXPECT_EQ(getOldIfile(), nullptr);

    EXPECT_EQ(getIndex(file1Ptr), 1);
    EXPECT_EQ(getIndex(file2Ptr), -1);
    EXPECT_EQ(getIndex(file3Ptr), 2);
    EXPECT_EQ(getIndex(file4Ptr), 3);
}

TEST_F(IfileUnitTest4Files, nextIfile_nullptr)
{
  Ifile * ret = nextIfile(nullptr);
  EXPECT_EQ(ret, file1Ptr);
}

TEST_F(IfileUnitTest4Files, nextIfile_1stptr)
{
  Ifile * ret = nextIfile(file1Ptr);
  EXPECT_EQ(ret, file2Ptr);
}

TEST_F(IfileUnitTest4Files, nextIfile_lastptr)
{
  Ifile * ret = nextIfile(file4Ptr);
  EXPECT_EQ(ret, nullptr);
}

TEST_F(IfileUnitTest4Files, prevIfile_nullptr)
{
  Ifile * ret = prevIfile(nullptr);
  EXPECT_EQ(ret, file4Ptr);
}

TEST_F(IfileUnitTest4Files, prevIfile_1stptr)
{
  Ifile * ret = prevIfile(file1Ptr);
  EXPECT_EQ(ret, nullptr);
}

TEST_F(IfileUnitTest4Files, prevIfile_lastptr)
{
  Ifile * ret = prevIfile(file4Ptr);
  EXPECT_EQ(ret, file3Ptr);
}

TEST_F(IfileUnitTest4Files, findIfile_changeOfFilename)
{

  char * shorterFname = initfilename("f.txt");

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).          
    WillOnce(Return(shorterFname));

  EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
    Times(1).
    WillOnce(Return(initfilename(f1)));

  Ifile * foundIfile = findIfile(shorterFname);

  EXPECT_EQ(foundIfile->getFilename(), shorterFname);
}

TEST_F(IfileUnitTest4Files, findIfile_createIfile)
{
  char const * const fname = "newfilename.txt";

  char * newfname1 = initfilename(fname);
  char * newfname2 = initfilename(fname);

  EXPECT_CALL(GetMock<UtilsMock>(), save(_)).
    Times(1).          
    WillOnce(Return(newfname1));

  EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
    Times(1).
    WillOnce(Return(newfname2));

  createIfile(fname);

  EXPECT_CALL(GetMock<UtilsMock>(), filename::lrealpath(_)).
    Times(1).
    WillOnce(Return(initfilename(fname)));

  Ifile * newIfile = getIfile(fname);

  EXPECT_STREQ(newIfile->getFilename(), fname);
  EXPECT_EQ(fileList.size(), 5);
}

// Tests Above cover all these with 100% coverage
//  currentIfile
//  previousIfile
//  getCurrentIfile
//  setCurrentIfile
//  getOldIfile
//  setOldIfile
//  numIfiles   -- see FindIfile
//  newIfile
//  deleteIfile -- see CreateAndDelete, CreateAndDelete
//  getIfile
//  getIndex
//  Ifile::Ifile
//  Ifile::Ifile(const Ifile& src)
//  Ifile& Ifile::operator=(const Ifile& src)
//  find
//  nextIfile
//  prevIfile
//  getOffIfile
//  findIfile
//  createIfile
//  Ifile::~Ifile()
//  const char* getFilename() { return this->h_filename; };
//  scrpos getPos() { return this->h_scrpos; };
//  bool getOpened() { return this->h_opened; };
//  int getHoldCount() { return this->h_hold; };
//  void* getFilestate() { return this->h_filestate; };
//  void* getAltpipe() { return this->h_altpipe; };
//  char* getAltfilename() { return this->h_altfilename; };
//  void setFilename(const char* newFilename);
//  void setPos(scrpos pos);
//  void setOpened(bool opened);
//  void setHold(int incr);
//  void setFilestate(void* filestate);
//  void setAltpipe(void* altpipePtr);
//  void setAltfilename(char* altfilename);
//  bool operator==(const Ifile& lhs, const Ifile& rhs)


} // ifile namespace

// Google Test can be run manually from the main() function
// or, it can be linked to the gtest_main library for an already
// set-up main() function primed to accept Google Test test cases.
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
